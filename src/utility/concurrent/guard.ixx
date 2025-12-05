//
// Created by Matrix on 2025/11/20.
//

export module mo_yanxi.concurrent.guard;

import std;

namespace mo_yanxi::ccur{

export
template <std::ptrdiff_t I>
struct semaphore_acq_guard{
	std::counting_semaphore<I>* semaphore;

	[[nodiscard]] semaphore_acq_guard() = default;

	[[nodiscard]] explicit(false) semaphore_acq_guard(std::counting_semaphore<I>* semaphore) noexcept
	: semaphore(semaphore){
		if(semaphore)semaphore->acquire();
	}

	[[nodiscard]] explicit(false) semaphore_acq_guard(std::counting_semaphore<I>& semaphore) noexcept
		: semaphore(&semaphore){
		semaphore.acquire();
	}

	[[nodiscard]] explicit(false) semaphore_acq_guard(std::counting_semaphore<I>& semaphore, std::adopt_lock_t) noexcept
		: semaphore(&semaphore){
	}

	~semaphore_acq_guard(){
		if(semaphore)semaphore->release();
	}

	semaphore_acq_guard(const semaphore_acq_guard& other) = delete;

	semaphore_acq_guard(semaphore_acq_guard&& other) noexcept
	: semaphore{std::exchange(other.semaphore, {})}{
	}

	semaphore_acq_guard& operator=(const semaphore_acq_guard& other) = delete;

	semaphore_acq_guard& operator=(semaphore_acq_guard&& other) noexcept{
		if(this == &other) return *this;
		if(semaphore)semaphore->release();
		semaphore = std::exchange(other.semaphore, {});
		return *this;
	}
};


}