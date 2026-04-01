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
	using allocator_type = typename container_type::allocator_type;
	using value_type = T;

	// 使用 is_nothrow_copy_constructible_v 推导 noexcept
	explicit mpsc_double_buffer(const container_type& init_container)
		noexcept(std::is_nothrow_copy_constructible_v<container_type>)
		requires (std::copy_constructible<T>)
		: buffers_{init_container, init_container}{
	}

	// 使用 is_nothrow_constructible_v 推导 noexcept
	explicit mpsc_double_buffer(const allocator_type& alloc)
		noexcept(std::is_nothrow_constructible_v<container_type, const allocator_type&>)
		: buffers_{container_type{alloc}, container_type{alloc}}{
	}

	[[nodiscard]] mpsc_double_buffer() = default;

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

	// 使用 declval 嗅探底层 push_back 的异常特性
	void push(const T& item) noexcept(noexcept(std::declval<container_type&>().push_back(item))){
		std::uint32_t idx = lock();
		try{
			buffers_[idx].push_back(item);
		}catch(...){
			unlock(); // 如果抛出异常，说明没有成功存入数据，只释放锁，不设置 HAS_DATA_BIT
			throw;
		}

		unlock_and_set_data();
	}

	// 使用 declval 嗅探底层 move push_back 的异常特性
	void push(T&& item) noexcept(noexcept(std::declval<container_type&>().push_back(std::move(item)))){
		std::uint32_t idx = lock();
		try{
			buffers_[idx].push_back(std::move(item));
		}catch(...){
			unlock();
			throw;
		}

		unlock_and_set_data();
	}

	// 修复了拼写错误 empalce -> emplace，并补充了 try-catch 和 noexcept 推导
	template <typename ...Args>
		requires std::constructible_from<T, Args&&...>
	void emplace(Args&& ...args) noexcept(noexcept(std::declval<container_type&>().emplace_back(std::forward<Args>(args)...))){
		std::uint32_t idx = lock();
		try{
			buffers_[idx].emplace_back(std::forward<Args>(args)...);
		}catch(...){
			unlock();
			throw;
		}

		unlock_and_set_data();
	}

	// 引入万能引用并使用 is_nothrow_invocable_v 探测可调用对象的异常特性，增加 try-catch
	template <std::invocable<container_type&> Fn>
	void modify(Fn&& fn) noexcept(std::is_nothrow_invocable_v<Fn, container_type&>){
		std::uint32_t idx = lock();
		if constexpr(std::is_nothrow_invocable_v<Fn, container_type&>){
			std::invoke(std::forward<Fn>(fn), buffers_[idx]);
		} else{
			try{
				std::invoke(std::forward<Fn>(fn), buffers_[idx]);
			} catch(...){
				unlock();
				throw;
			}
		}
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

	// 为了允许在 size() const 方法中加解锁，state_ 需要被标记为 mutable
	alignas(std::hardware_destructive_interference_size) mutable std::atomic<std::uint32_t> state_;

	static constexpr std::uint32_t INDEX_BIT = 1 << 0;
	static constexpr std::uint32_t LOCK_BIT = 1 << 1;
	static constexpr std::uint32_t HAS_DATA_BIT = 1 << 2;

	std::uint32_t active_index() const noexcept{
		return state_.load(std::memory_order_relaxed) & INDEX_BIT;
	}

	std::uint32_t active_index_relaxed() const noexcept{
		return state_.load(std::memory_order_relaxed) & INDEX_BIT;
	}

	// C++20 优化的 wait/notify 锁机制，为了支持 size() const 标记为 const
	std::uint32_t lock() const noexcept{
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

	void unlock() const noexcept{
		// 持有锁时，其他线程不会修改 state_，直接 load + store 即可
		std::uint32_t current = state_.load(std::memory_order_relaxed);
		state_.store(current & ~LOCK_BIT, std::memory_order_release);
		state_.notify_one();
	}

	void unlock_and_set_data() const noexcept{
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