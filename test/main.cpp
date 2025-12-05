#include <cstdio>


import std;
import mo_yanxi.open_addr_hash_map;
// 保留你原有的其他 import
// import mo_yanxi.math.vector2;
// ...

using namespace std::chrono;

// ================= 配置参数 =================
constexpr unsigned DATA_SIZE = 1'000'000'0;
constexpr unsigned TEST_ROUNDS = 1000'000; // 正确性测试的操作次数
constexpr unsigned EMPTY_KEY = static_cast<unsigned>(-1);

// ================= 辅助工具 =================

// 计时器
class ScopedTimer {
    std::string_view name;
    time_point<high_resolution_clock> start;
public:
    ScopedTimer(std::string_view name) : name(name), start(high_resolution_clock::now()) {}
    ~ScopedTimer() {
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(end - start).count();
        std::println("{:<25} 耗时: {:>6} ms", name, duration);
    }
};

// 随机数生成器 (排除 EMPTY_KEY)
unsigned get_random_key(std::mt19937& rng) {
    std::uniform_int_distribution<unsigned> dist(0, std::numeric_limits<unsigned>::max() - 2);
    return dist(rng);
}

// ================= 正确性测试核心逻辑 =================

// 检查自定义 Map 是否与标准 Map 内容完全一致
template<typename StdMap, typename CustomMap>
void check_consistency(const StdMap& std_map, CustomMap& custom_map, int step) {
    // 1. 检查大小
    if (std_map.size() != custom_map.size()) {
        std::println(stderr, "错误: 大小不匹配! Step: {}, Std: {}, Custom: {}",
            step, std_map.size(), custom_map.size());
        std::abort();
    }

    // 2. 正向检查: 遍历标准 Map，确保 Custom Map 中都有且值正确
    for (const auto& [k, v] : std_map) {
        auto it = custom_map.find(k);
        if (it == custom_map.end()) {
            std::println(stderr, "错误: 丢失 Key {}! Step: {}", k, step);
            std::abort();
        }
        if (it->second != v) {
            std::println(stderr, "错误: Value 不匹配 Key {}! Std: {}, Custom: {}", k, v, it->second);
            std::abort();
        }
    }

    // 3. 反向检查: 遍历 Custom Map，确保没有多余的“幽灵元素”
    for (const auto& pair : custom_map) {
        if (std_map.find(pair.first) == std_map.end()) {
            std::println(stderr, "错误: 存在多余 Key {}! Step: {}", pair.first, step);
            std::abort();
        }
    }
}

void verify_correctness() {
    std::println("=== 开始正确性验证 (Fuzz Testing) ===");

    // 对照组
    std::map<unsigned, unsigned> std_ref;
    // 测试组
    mo_yanxi::fixed_open_hash_map<unsigned, unsigned, EMPTY_KEY> custom_map;

    std::mt19937 rng(12345); // 固定种子以便复现
    std::uniform_int_distribution<int> op_dist(0, 100);

    {
        ScopedTimer t("Correctness Check");
        for (int i = 0; i < TEST_ROUNDS; ++i) {
            unsigned key = get_random_key(rng);
            int op = op_dist(rng);

            if (op < 60) {
                // [60% 概率] 插入 或 更新
                unsigned val = rng();
                std_ref[key] = val;
                custom_map[key] = val;
            }
            else if (op < 85) {
                // [25% 概率] 删除
                // 注意：需要测试删除不存在的key和存在的key
                size_t n1 = std_ref.erase(key);
                size_t n2 = custom_map.erase(key);

                if (n1 != n2) {
                    std::println(stderr, "错误: erase 返回值不一致! Key: {}, Std: {}, Custom: {}", key, n1, n2);
                    std::abort();
                }
            }
            else {
                // [15% 概率] 查找验证
                bool has_std = std_ref.contains(key);
                bool has_custom = custom_map.contains(key);
                if (has_std != has_custom) {
                    std::println(stderr, "错误: contains 结果不一致! Key: {}", key);
                    std::abort();
                }
            }

            // 每隔一定步数进行全量比对 (太频繁会影响测试速度)
            if (i % 1000 == 0) {
                check_consistency(std_ref, custom_map, i);
            }
        }
        // 最后进行一次终极比对
        check_consistency(std_ref, custom_map, TEST_ROUNDS);
    }

    std::println("PASSED: 所有正确性测试通过！\n");
}

// ================= 性能基准测试 (原代码) =================

std::vector<unsigned> generate_random_keys(unsigned count) {
    std::vector<unsigned> keys;
    keys.reserve(count);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<unsigned> dist(0, std::numeric_limits<unsigned>::max() - 2);
    for (unsigned i = 0; i < count; ++i) {
        keys.push_back(dist(rng));
    }
    return keys;
}

void benchmark_std(const std::vector<unsigned>& keys) {
    std::println("=== std::unordered_map Benchmark ===");
    std::unordered_map<unsigned, unsigned> map;

    {
        ScopedTimer t("Insert");
        for (unsigned k : keys) map[k] = k;
    }
    {
        ScopedTimer t("Find");
        volatile unsigned sink = 0;
        for (unsigned k : keys) {
            auto it = map.find(k);
            if (it != map.end()) sink += it->second;
        }
    }
    {
        ScopedTimer t("Iterate");
        volatile long long sum = 0;
        for (auto& [k, v] : map) sum += v;
    }
    {
        ScopedTimer t("Erase (Half)");
        size_t half_size = keys.size() / 2;
        for (size_t i = 0; i < half_size; ++i) map.erase(keys[i]);
    }
    std::println("最终大小: {}", map.size());
}

void benchmark_custom(const std::vector<unsigned>& keys) {
    std::println("\n=== mo_yanxi::fixed_open_hash_map Benchmark ===");
    mo_yanxi::fixed_open_hash_map<unsigned, unsigned, EMPTY_KEY> map;

    {
        ScopedTimer t("Insert");
        for (unsigned k : keys) map[k] = k;
    }
    {
        ScopedTimer t("Find");
        volatile unsigned sink = 0;
        for (unsigned k : keys) {
            auto it = map.find(k);
            if (it != map.end()) sink += it->second;
        }
    }
    {
        ScopedTimer t("Iterate");
        volatile long long sum = 0;
        for (auto&& [k, v] : map) sum += v;
    }
    {
        ScopedTimer t("Erase (Half)");
        size_t half_size = keys.size() / 2;
        for (size_t i = 0; i < half_size; ++i) map.erase(keys[i]);
    }
    std::println("最终大小: {}", map.size());
}

int main() {
    // 1. 先运行正确性测试
    verify_correctness();

    // 2. 如果通过，再运行性能测试
    std::println("准备性能测试数据: {} 个随机整数...", DATA_SIZE);
    auto keys = generate_random_keys(DATA_SIZE);

    benchmark_std(keys);
    benchmark_custom(keys);

    return 0;
}