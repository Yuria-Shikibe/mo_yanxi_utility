import std;
import mo_yanxi.byte_pool;

using namespace mo_yanxi;

// --- Correctness Tests ---

void test_trivial_ops() {
    std::println("Testing Trivial Operations (int)...");
    byte_pool pool;
    auto borrow = pool.borrow<int>(10);

    // Write
    for (int i = 0; i < 10; ++i) borrow[i] = i;

    // Read
    for (int i = 0; i < 10; ++i) {
        if (borrow[i] != i) {
            std::println(stderr, "Mismatch at index {}", i);
            std::terminate();
        }
    }

    // Resize larger (preserve)
    borrow.resize<true>(20);
    for (int i = 0; i < 10; ++i) {
        if (borrow[i] != i) {
            std::println(stderr, "Mismatch after grow at index {}", i);
            std::terminate();
        }
    }

    // Fill new area
    for (int i = 10; i < 20; ++i) borrow[i] = i;

    // Resize smaller
    borrow.resize<true>(5);
    for (int i = 0; i < 5; ++i) {
         if (borrow[i] != i) {
            std::println(stderr, "Mismatch after shrink at index {}", i);
            std::terminate();
        }
    }

    std::println("  PASS");
}

struct LifeCycle {
    static int constructed;
    static int destructed;
    int val;

    LifeCycle() : val(0) { ++constructed; }
    LifeCycle(int v) : val(v) { ++constructed; }
    ~LifeCycle() { ++destructed; }
    LifeCycle(const LifeCycle& o) : val(o.val) { ++constructed; }
    LifeCycle(LifeCycle&& o) noexcept : val(o.val) { ++constructed; o.val = -1; }
    LifeCycle& operator=(const LifeCycle&) = default;
    LifeCycle& operator=(LifeCycle&& o) noexcept {
        val = o.val;
        o.val = -1;
        return *this;
    }

    static void reset() { constructed = 0; destructed = 0; }
};

int LifeCycle::constructed = 0;
int LifeCycle::destructed = 0;

void test_nontrivial_ops() {
    std::println("Testing Non-Trivial Operations (LifeCycle)...");
    LifeCycle::reset();

    {
        byte_pool pool;
        auto borrow = pool.borrow<LifeCycle>(10);

        if (LifeCycle::constructed != 10) {
            std::println(stderr, "Construction count mismatch. Expected 10, got {}", LifeCycle::constructed);
            std::terminate();
        }

        // Resize grow
        // Note: byte_pool allocates min 512 bytes. LifeCycle is small (4 bytes).
        // 10 -> 20 elements fits in capacity (128 elements).
        // So resize happens in-place.
        // It constructs 10 new elements (indices 10..19).
        // Total constructed = 10 (initial) + 10 (growth) = 20.
        borrow.resize<true>(20);

        if (LifeCycle::constructed != 20) {
             std::println(stderr, "After resize constructed mismatch. Expected 20, got {}", LifeCycle::constructed);
             std::terminate();
        }
    }

    // Scope exit: destructs 20 elements.
    if (LifeCycle::destructed != 20) {
         std::println(stderr, "Destruction count mismatch. Expected 20, got {}", LifeCycle::destructed);
         std::terminate();
    }
     std::println("  PASS");
}

void test_bucket_reuse() {
    std::println("Testing Bucket Reuse...");
    byte_pool pool;
    void* ptr1 = nullptr;

    {
        auto b1 = pool.borrow<int>(100);
        ptr1 = b1.data();
    } // b1 retired, buffer goes to bucket

    {
        auto b2 = pool.borrow<int>(100);
        if (b2.data() != ptr1) {
             std::println(stderr, "Buffer not reused!");
        } else {
             std::println("  Reuse confirmed");
        }
    }
     std::println("  PASS");
}


// --- Benchmark ---

void run_benchmark() {
    std::println("Running Benchmarks...");

    constexpr int ITERATIONS = 1'000'000;

    // 1. Small Allocations (64 bytes)
    {
        auto start = std::chrono::high_resolution_clock::now();
        byte_pool pool;
        for(int i=0; i<ITERATIONS; ++i) {
            auto b = pool.borrow<std::byte>(64);
            b[0] = std::byte{1}; // touch
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::println("  byte_pool small alloc (64B) x 1M: {} ms", dur);
    }

    {
        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<ITERATIONS; ++i) {
            std::vector<std::byte> v(64);
            v[0] = std::byte{1};
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::println("  std::vector small alloc (64B) x 1M: {} ms", dur);
    }

     // 2. Large Allocations (1MB) - Fewer iterations
     constexpr int LARGE_ITER = 1000;
     constexpr int LARGE_SIZE = 1024 * 1024;

     {
        auto start = std::chrono::high_resolution_clock::now();
        byte_pool pool;
        for(int i=0; i<LARGE_ITER; ++i) {
            auto b = pool.borrow<std::byte>(LARGE_SIZE);
            b[0] = std::byte{1};
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::println("  byte_pool large alloc (1MB) x 1k: {} ms", dur);
    }

    {
        auto start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<LARGE_ITER; ++i) {
            std::vector<std::byte> v(LARGE_SIZE);
            v[0] = std::byte{1};
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::println("  std::vector large alloc (1MB) x 1k: {} ms", dur);
    }
}

int main() {
    try {
        test_trivial_ops();
        test_nontrivial_ops();
        test_bucket_reuse();
        run_benchmark();
    } catch (const std::exception& e) {
        std::println(stderr, "Exception: {}", e.what());
        return 1;
    }
    return 0;
}
