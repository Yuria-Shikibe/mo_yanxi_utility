export module mo_yanxi.ccur.atomic_shared_mutex;

import std;

namespace mo_yanxi::ccur{
	/**
	 * @brief Reader takes the privilege, does not guarantee the writer can capture the lock.
	 *
	 * Used for fragment read and large write operations.
	 *
	 * 1.75~2 times faster(throughput/read operation) than std::shared_mutex in such case.
	 *
	 * 3~4 times lesser in write operations.
	 *
	 * @warning ONLY SINGLE WRITER is permitted!
	 */
	export
	struct atomic_shared_mutex{
	private:
		using counter_type = std::size_t;
		static constexpr counter_type write_flag = std::numeric_limits<counter_type>::max();

		std::atomic<counter_type> state_{0};

	public:
		void lock() noexcept{
			counter_type expected = 0;
			while(!state_.compare_exchange_weak(expected, write_flag,
			                                    std::memory_order_acquire,
			                                    std::memory_order_relaxed)){
				// state_.wait(expected, std::memory_order_relaxed);
				expected = 0;
			}
		}

		bool try_lock() noexcept{
			counter_type expected = 0;
			return state_.compare_exchange_strong(expected, write_flag,
			                                      std::memory_order_acquire,
			                                      std::memory_order_relaxed);
		}

		void unlock() noexcept{
			state_.store(0, std::memory_order_release);
		}

		void lock_shared() noexcept{
			counter_type prev = state_.load(std::memory_order_relaxed);

			while(true){
				if(prev == write_flag){ //Spin
					std::this_thread::yield();
					prev = state_.load(std::memory_order_relaxed);
				}else if(state_.compare_exchange_weak(
					prev, prev + 1,
					std::memory_order_acquire,
					std::memory_order_relaxed)){
					break;
				}
			}
		}

		/**
		 * @brief downgrade the write to reader
		 * @return true if successful, basically used for assertion
		 * @warning Must call after lock() and be in the same thread!
		 */
		bool downgrade() noexcept{
			auto val = write_flag;
			return state_.compare_exchange_strong(val, 1, std::memory_order_relaxed, std::memory_order_relaxed);
		}

		bool try_lock_shared() noexcept{
			counter_type prev = state_.load(std::memory_order_relaxed);
			if(prev == write_flag){
				return false;
			}

			return state_.compare_exchange_strong(prev, prev + 1,
			                                      std::memory_order_acquire,
			                                      std::memory_order_relaxed);
		}

		void unlock_shared() noexcept{
			const auto prev = state_.fetch_sub(1, std::memory_order_release);

			// 如果原来的值是 1，说明这是最后一个读者，现在状态变为了 0
			// 此时必须唤醒可能正在等待的写者
			if(prev == 1){
				state_.notify_one();
			}
		}
	};

	export struct shared_lock{
	private:
		atomic_shared_mutex* mutex_{nullptr};

	public:
		shared_lock() noexcept = default;

		// 构造时获取共享锁
		explicit shared_lock(atomic_shared_mutex& mtx) noexcept
			: mutex_(&mtx){
			mutex_->lock_shared();
		}

		// 领养锁 (假设当前线程已经持有锁)
		shared_lock(atomic_shared_mutex& mtx, std::adopt_lock_t) noexcept
			: mutex_(&mtx){
		}

		// 析构时释放
		~shared_lock() noexcept{
			if(mutex_){
				mutex_->unlock_shared();
			}
		}

		// 拷贝构造：复制指针并增加引用计数 (获取锁)
		shared_lock(const shared_lock& other) noexcept
			: mutex_(other.mutex_){
			if(mutex_){
				mutex_->lock_shared();
			}
		}

		// 拷贝赋值：释放旧的，获取新的
		shared_lock& operator=(const shared_lock& other) noexcept{
			if(this == &other) return *this;

			if(mutex_){
				mutex_->unlock_shared();
			}

			mutex_ = other.mutex_;

			if(mutex_){
				mutex_->lock_shared();
			}
			return *this;
		}

		shared_lock(shared_lock&& other) noexcept
			: mutex_(std::exchange(other.mutex_, nullptr)){
		}

		shared_lock& operator=(shared_lock&& other) noexcept{
			if(this == &other) return *this;

			if(mutex_){
				mutex_->unlock_shared();
			}

			mutex_ = std::exchange(other.mutex_, nullptr);
			return *this;
		}

		void lock() const noexcept{
			if(mutex_) mutex_->lock_shared();
		}

		void unlock() noexcept{
			if(mutex_){
				mutex_->unlock_shared();
				mutex_ = nullptr;
			}
		}

		[[nodiscard]] bool owns_lock() const noexcept{
			return mutex_ != nullptr;
		}

		[[nodiscard]] explicit operator bool() const noexcept{
			return owns_lock();
		}
	};
}
