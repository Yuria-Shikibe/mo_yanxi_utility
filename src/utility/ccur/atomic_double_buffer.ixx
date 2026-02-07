

export module mo_yanxi.concurrent.atomic_double_buffer;

import std;

namespace mo_yanxi::ccur{
	export
	template <typename T>
	struct atomic_double_buffer{
		using value_type = T;
	private:
		using atomic_value_type = unsigned;
		static constexpr unsigned index_bit = 0b1;
		static constexpr unsigned load_bit = 0b10;
		static constexpr unsigned store_bit = 0b100;

		std::atomic<atomic_value_type> flags{};
		std::array<value_type, 2> buffer{};

	public:
		template <std::invocable<value_type&> Modifier>
		void modify(Modifier modifier) noexcept(std::is_nothrow_invocable_v<Modifier, value_type&>){

			//mark modify is wanted
			const auto cur = flags.fetch_or(store_bit, std::memory_order_acquire);

			const auto last_idx = cur & index_bit;
			const auto next_idx = last_idx ^ index_bit;

			if constexpr (noexcept(std::is_nothrow_invocable_v<Modifier, value_type&>)){
				std::invoke(std::move(modifier), buffer[next_idx]);
			}else{
				try {
					std::invoke(std::move(modifier), buffer[next_idx]);
				} catch (...) {
					flags.fetch_and(~store_bit, std::memory_order_release);
					flags.notify_one();
					throw;
				}
			}


			auto expected = last_idx | store_bit;
			while(true){
				// 尝试原子地 移除store_bit 并 翻转index
				// 只有当 flags 严格等于 (last_idx | store_bit) 时才成功
				if(flags.compare_exchange_weak(expected, next_idx,
					std::memory_order_release,
					std::memory_order_relaxed)){
					flags.notify_one();
					break; // 提交成功
				}

				// 如果 CAS 失败，expected 会被更新为当前 flags 的值。

				// 检查失败原因：是否是因为有消费者在读取 (load_bit 被设置)？
				if(expected & load_bit){
					// 确实有消费者在读，我们需要等待消费者离开。
					// 此时 expected 已经包含了 load_bit，我们直接 wait 这个状态。
					// 注意：wait 需要值严格相等才阻塞，所以这里直接用更新后的 expected 是对的。
					flags.wait(expected, std::memory_order_relaxed);

					// 唤醒后，消费者应该已经清除 load_bit。
					// 此时我们需要重置 expected，准备下一次 CAS 尝试。
					// 我们的目标仍然是从 (store | index) -> (next_idx)
					// 所以重置 expected 为仅仅包含 store 和 index
					// (注意：这里要小心 ABA，但在这个简单模型中，index 不变，store 也不变，
					// 唯有 load_bit 会变，所以重新构造 expected 是安全的)
					expected = last_idx | store_bit;
				} else{
					// 如果 CAS 失败不是因为 load_bit (可能是 spurious failure)，继续循环尝试
				}
			}
		}

		void store(value_type&& value) noexcept(std::is_nothrow_move_assignable_v<value_type>){
			this->modify([&](value_type& t){
				t = std::move(value);
			});
		}

		void store(const value_type& value) noexcept(std::is_nothrow_copy_assignable_v<value_type>){
			this->modify([&](value_type& t){
				t = value;
			});
		}

		template <std::invocable<value_type&> Loader>
		void load(Loader loader) noexcept(std::is_nothrow_invocable_v<Loader, value_type&>){
			const auto cur = flags.fetch_or(load_bit, std::memory_order_acquire);
			const auto last_idx = cur & index_bit;

			if constexpr(noexcept(std::is_nothrow_invocable_v<Loader, value_type&>)){
				std::invoke(std::move(loader), buffer[last_idx]);
			} else{
				try{
					std::invoke(std::move(loader), buffer[last_idx]);
				} catch(...){
					flags.fetch_and(~load_bit, std::memory_order_release); //quit loading
					flags.notify_one(); //notify store is allowed
					throw;
				}
			}

			flags.fetch_and(~load_bit, std::memory_order_release); //quit loading
			flags.notify_one(); //notify store is allowed
		}

		template <std::invocable<value_type&> Loader>
		void load_latest(Loader loader) noexcept(std::is_nothrow_invocable_v<Loader, value_type&>){
			const auto cur = flags.fetch_or(load_bit, std::memory_order_acquire);

			if(cur & store_bit){
				//is storing
				flags.fetch_and(~load_bit, std::memory_order_release); //quit loading
				flags.notify_one(); //notify store is allowed
			}

			auto expected = cur & ~store_bit;
			while(true){ //wait until no one is storing and get the latest data
				if(flags.compare_exchange_weak(expected , expected | load_bit, std::memory_order_acquire, std::memory_order_relaxed)){
					break;
				}

				if(expected & store_bit){
					flags.wait(expected, std::memory_order_relaxed);

					expected = expected & ~store_bit;
				}
			}

			const auto last_idx = expected & index_bit;

			if constexpr(noexcept(std::is_nothrow_invocable_v<Loader, value_type&>)){
				std::invoke(std::move(loader), buffer[last_idx]);
			} else{
				try{
					std::invoke(std::move(loader), buffer[last_idx]);
				} catch(...){
					flags.fetch_and(~load_bit, std::memory_order_release); //quit loading
					flags.notify_one(); //notify store is allowed
					throw;
				}
			}

			flags.fetch_and(~load_bit, std::memory_order_release); //quit loading
			flags.notify_one(); //notify store is allowed
		}
	};

	export
	template <typename T>
	struct atomic_double_buffer_no_propagate : atomic_double_buffer<T>{
		atomic_double_buffer_no_propagate(const atomic_double_buffer_no_propagate& other) noexcept{
		}

		atomic_double_buffer_no_propagate(atomic_double_buffer_no_propagate&& other) noexcept {
		}

		atomic_double_buffer_no_propagate& operator=(const atomic_double_buffer_no_propagate& other) noexcept {
			return *this;
		}

		atomic_double_buffer_no_propagate& operator=(atomic_double_buffer_no_propagate&& other) noexcept{
			return *this;
		}
	};
}