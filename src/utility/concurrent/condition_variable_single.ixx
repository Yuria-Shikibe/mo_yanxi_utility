//
// Created by Matrix on 2024/11/2.
//

export module mo_yanxi.condition_variable_single;

import mo_yanxi.utility;
import std;

namespace mo_yanxi{
	export
	class condition_variable_single{
		std::counting_semaphore<> s_{std::ptrdiff_t{0}};

	public:
		condition_variable_single() noexcept = default;

		void notify_one() noexcept{
			s_.release();
		}

		void wait(std::unique_lock<std::mutex>& lock) noexcept{
			lock.unlock();
			s_.acquire();
			lock.lock();
		}

		template <std::predicate<> Pred>
		void wait(std::unique_lock<std::mutex>& lock, Pred pred) noexcept{
			while(!pred()){
				wait(lock);
			}
		}

		template <class Rep, class Period>
		std::cv_status wait_for(
			std::unique_lock<std::mutex>& lock,
			const std::chrono::duration<Rep, Period>& rel_time){
			lock.unlock();
			auto st = s_.try_acquire_for(rel_time);
			lock.lock();

			return st ? std::cv_status::no_timeout : std::cv_status::timeout;
		}

		template <class Clock, class Duration>
		std::cv_status wait_until(
			std::unique_lock<std::mutex>& lock,
			const std::chrono::time_point<Clock, Duration>& abs_time){
			lock.unlock();
			auto st = s_.try_acquire_until(abs_time);
			lock.lock();

			return st ? std::cv_status::no_timeout : std::cv_status::timeout;
		}

		template <std::predicate<> Pred, class Clock, class Duration>
		bool wait_until(
			std::unique_lock<std::mutex>& lock,
			const std::chrono::time_point<Clock, Duration>& abs_time,
			Pred pred
		){

			while (!pred()) {
				if (this->wait_until(lock, abs_time) == std::cv_status::timeout) {
					return pred();
				}
			}

			return true;
		}

		template <std::predicate<> Pred, class Rep, class Period>
		bool wait_for(
			std::unique_lock<std::mutex>& lock,
			const std::chrono::duration<Rep, Period>& rel_time,
			Pred pred
		){
			return this->wait_until(lock, mo_yanxi::to_absolute_time(rel_time), mo_yanxi::pass_fn(pred));
		}
	};
}
