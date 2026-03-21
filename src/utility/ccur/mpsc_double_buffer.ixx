export module mo_yanxi.concurrent.mpsc_double_buffer;

import std;

namespace mo_yanxi::ccur{
template <typename C, typename T>
concept double_buffer_container = requires(C c, T val){
	{ c.push_back(val) };
	{ c.push_back(std::move(val)) };
	{ c.clear() };
	std::ranges::sized_range<const C&>;
	{ c.empty() } -> std::convertible_to<bool>;
} && std::ranges::range<C> && std::swappable<C>;

export
template <typename T, double_buffer_container<T> Container = std::vector<T>>
class mpsc_double_buffer{
	static_assert(std::is_object_v<T>);

public:
	using container_type = Container;
	using value_type = T;

	explicit mpsc_double_buffer(const container_type& init_container = container_type())
		: buffers_{init_container, init_container}{
	}


	container_type* fetch() noexcept{
		if((state_.load(std::memory_order_acquire) & HAS_DATA_BIT) == 0){
			return nullptr;
		}

		// 这里的 active_index_relaxed() 依然保留，因为我们需要在加锁前清理 inactive buffer
		std::uint32_t current_idx = active_index_relaxed();
		std::uint32_t inactive_idx = 1 - current_idx;
		buffers_[inactive_idx].clear();

		// 加锁，这里不需要使用返回的 index，因为消费者自己独占控制了 index 的翻转
		lock();

		std::uint32_t current_state = state_.load(std::memory_order_relaxed);
		std::uint32_t new_state = (current_state ^ INDEX_BIT) & ~(HAS_DATA_BIT | LOCK_BIT);
		state_.store(new_state, std::memory_order_release);
		state_.notify_one();

		return std::addressof(buffers_[current_idx]);
	}

	void push(const T& item){
		std::uint32_t idx = lock();
		buffers_[idx].push_back(item);
		unlock_and_set_data();
	}

	void push(T&& item){
		std::uint32_t idx = lock();
		buffers_[idx].push_back(std::move(item));
		unlock_and_set_data();
	}

	template <std::invocable<container_type&> Fn>
	void modify(Fn fn) noexcept(std::is_nothrow_invocable_v<Fn, container_type&>){
		std::uint32_t idx = lock();
		std::invoke(fn, buffers_[idx]);
		unlock_and_set_data();
	}

	std::size_t size() const noexcept{
		std::uint32_t idx = lock();
		const std::size_t s = std::ranges::size(buffers_[idx]);
		unlock();
		return s;
	}

private:
	alignas(std::hardware_destructive_interference_size) container_type buffers_[2];
	alignas(std::hardware_destructive_interference_size) std::atomic<std::uint32_t> state_;

	static constexpr std::uint32_t INDEX_BIT = 1 << 0;
	static constexpr std::uint32_t LOCK_BIT = 1 << 1;
	static constexpr std::uint32_t HAS_DATA_BIT = 1 << 2;

	std::uint32_t active_index() const noexcept{
		return state_.load(std::memory_order_relaxed) & INDEX_BIT;
	}

	std::uint32_t active_index_relaxed() const noexcept{
		return state_.load(std::memory_order_relaxed) & INDEX_BIT;
	}

	// C++20 优化的 wait/notify 锁机制
	std::uint32_t lock() noexcept{
		std::uint32_t current = state_.load(std::memory_order_relaxed);
		while(true){
			if((current & LOCK_BIT) == 0){
				// 尝试加锁
				if(state_.compare_exchange_weak(current, current | LOCK_BIT,
					std::memory_order_acquire,
					std::memory_order_relaxed)){
					// 加锁成功！此时 current 就是加锁瞬间的完整状态
					// 直接顺手提取并返回 active_index
					return current & INDEX_BIT;
				}
			} else{
				state_.wait(current, std::memory_order_relaxed);
				current = state_.load(std::memory_order_relaxed);
			}
		}
	}

	void unlock() noexcept{
		// 持有锁时，其他线程不会修改 state_，直接 load + store 即可
		std::uint32_t current = state_.load(std::memory_order_relaxed);
		state_.store(current & ~LOCK_BIT, std::memory_order_release);
		state_.notify_one();
	}

	void unlock_and_set_data() noexcept{
		std::uint32_t current = state_.load(std::memory_order_relaxed);
		// 直接覆盖，省去 do-while CAS 循环
		std::uint32_t new_state = (current & ~LOCK_BIT) | HAS_DATA_BIT;
		state_.store(new_state, std::memory_order_release);
		state_.notify_one();
	}
};


export
template <typename T, double_buffer_container<T> C>
struct mpsc_double_buffer_no_propagate : mpsc_double_buffer<T, C> {
	using mpsc_double_buffer<T, C>::mpsc_double_buffer;

	mpsc_double_buffer_no_propagate() = default;
	mpsc_double_buffer_no_propagate(const mpsc_double_buffer_no_propagate& other) noexcept {}
	mpsc_double_buffer_no_propagate(mpsc_double_buffer_no_propagate&& other) noexcept {}
	mpsc_double_buffer_no_propagate& operator=(const mpsc_double_buffer_no_propagate& other) noexcept {
		return *this;
	}
	mpsc_double_buffer_no_propagate& operator=(mpsc_double_buffer_no_propagate&& other) noexcept {
		return *this;
	}
};
}
