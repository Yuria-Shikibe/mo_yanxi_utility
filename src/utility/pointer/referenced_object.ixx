module;

#include <cassert>
#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.referenced_ptr;

import std;

namespace mo_yanxi{

export struct no_deletion_on_ref_count_to_zero{};

template <typename T>
constexpr bool no_delete_on_drop = std::is_same_v<T, no_deletion_on_ref_count_to_zero>;

export
template <typename T, typename D = std::default_delete<T>>
struct referenced_ptr;

template <typename T, typename D>
struct referenced_ptr{
	template <typename Ty, typename Dy>
	friend struct referenced_ptr;

	using element_type = std::remove_const_t<T>;
	using pointer = T*;

	[[nodiscard]] constexpr referenced_ptr() = default;

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object) : object{object}{
		if(this->object) incr();
	}

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object, D&& deleter) : object{object}, deleter{std::move(deleter)}{
		if(this->object) incr();
	}

	[[nodiscard]] constexpr explicit(false) referenced_ptr(pointer object, const D& deleter) : object{object}, deleter{deleter}{
		if(this->object) incr();
	}

	template <typename... Args>
		requires (std::constructible_from<T, Args...>)
	[[nodiscard]] explicit constexpr referenced_ptr(std::in_place_t, Args&&... args) : referenced_ptr{
			new element_type(std::forward<Args>(args)...)
		}{
	}

	template <std::derived_from<T> Ty>
		requires (std::is_const_v<T> == std::is_const_v<Ty>)
	explicit(false) referenced_ptr(const referenced_ptr<Ty>& other) : referenced_ptr{other.get()}{}

	template <std::derived_from<T> Ty>
		requires (std::is_const_v<T> == std::is_const_v<Ty>)
	explicit(false) referenced_ptr(referenced_ptr<Ty>&& other) : object{std::exchange(other.object, {})}{
	}


private:
	void delete_elem(pointer t) noexcept {
		std::invoke(deleter, t);
	}

	void decr() noexcept;

	void incr() noexcept;

public:
	explicit constexpr operator bool() const noexcept{
		return object != nullptr;
	}

	constexpr T& operator*() const noexcept{
		assert(object != nullptr);
		return *object;
	}

	constexpr T* operator->() const noexcept{
		return object;
	}

	constexpr T* get() const noexcept{
		return object;
	}

	constexpr void reset(T* ptr) noexcept{
		if(object){
			decr();
		}

		object = ptr;
		if(object) incr();
	}

	constexpr void reset(std::nullptr_t = nullptr) noexcept{
		if(object){
			decr();
		}

		object = nullptr;
	}

	constexpr ~referenced_ptr() noexcept{
		if(object){
			decr();
		}
	}

	constexpr referenced_ptr(const referenced_ptr& other) noexcept
		: object{other.object}{
		if(object) incr();
	}

	constexpr referenced_ptr(referenced_ptr&& other) noexcept
		: object{std::exchange(other.object, {})}{
	}

	constexpr referenced_ptr& operator=(const referenced_ptr& other){
		if(this == &other) return *this;
		if(this->object == other.object) return *this;

		this->reset(other.object);

		return *this;
	}

	constexpr referenced_ptr& operator=(referenced_ptr&& other) noexcept{
		if(this == &other) return *this;
		this->reset(other.object);

		return *this;
	}

	constexpr bool operator==(std::nullptr_t) const noexcept{
		return object == nullptr;
	}

	constexpr bool operator==(const referenced_ptr&) const noexcept = default;


private:
	T* object{};
	ADAPTED_NO_UNIQUE_ADDRESS D deleter{};
};


export
template <typename T, typename D>
constexpr bool operator==(const T* p, const referenced_ptr<T, D>& self) noexcept{
	return p == self.get();
}

export
template <typename T, typename D>
constexpr bool operator==(const referenced_ptr<T, D>& self, const T* p) noexcept{
	return self.get() == p;
}

/**
 * @brief Specify that lifetime of a style is NOT automatically managed by reference count.
 */
constexpr inline std::size_t persistent_spec = std::dynamic_extent;

namespace tags{
export
struct persistent_tag_t{
};

export constexpr inline persistent_tag_t persistent{};

}


export
/**
 * @warning
 * The move of ref count is only designed to use with external lifetime manage.
 * If it is used with default deleter, may cause dangling like errors.
 */
struct referenced_object{

private:
	std::size_t reference_count_{};

public:
	[[nodiscard]] constexpr referenced_object() = default;

	constexpr referenced_object(const referenced_object& other) noexcept{
	}

	constexpr referenced_object& operator=(const referenced_object& other){
		return *this;
	}

	constexpr referenced_object(referenced_object&& other) noexcept
		: reference_count_{std::exchange(other.reference_count_, {})}{
	}

	constexpr referenced_object& operator=(referenced_object&& other) noexcept{
		if(this == &other) return *this;
		reference_count_ = std::exchange(other.reference_count_, {});
		return *this;
	}

protected:

	[[nodiscard]] constexpr std::size_t ref_count() const noexcept{
		return reference_count_;
	}

	[[nodiscard]] constexpr bool droppable() const noexcept{
		return reference_count_ == 0;
	}

	/**
	 * @brief take reference
	 */
	constexpr void ref_incr() noexcept{
		++reference_count_;
	}

	/**
	 * @brief drop referebce
	 * @return true if should be destructed
	 */
	constexpr bool ref_decr() noexcept{
		assert(reference_count_ != 0);
		--reference_count_;
		return reference_count_ == 0;
	}


	template <typename T, typename D>
	friend struct referenced_ptr;
};

export
/**
 * @warning Must work with external lifetime manage
 * does not propagate reference count on copy/move
 */
struct referenced_object_atomic_nonpropagation{

private:
	std::atomic_size_t reference_count_{};

public:
	[[nodiscard]] referenced_object_atomic_nonpropagation() = default;

	referenced_object_atomic_nonpropagation(const referenced_object_atomic_nonpropagation& other) noexcept{}

	referenced_object_atomic_nonpropagation& operator=(const referenced_object_atomic_nonpropagation& other) noexcept {
		return *this;
	}

	referenced_object_atomic_nonpropagation(referenced_object_atomic_nonpropagation&& other) noexcept {
	}

	referenced_object_atomic_nonpropagation& operator=(referenced_object_atomic_nonpropagation&& other) noexcept{
		return *this;
	}

protected:

	[[nodiscard]] std::size_t ref_count() const noexcept{
		return reference_count_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] bool droppable() const noexcept{
		return reference_count_.load(std::memory_order_acquire) == 0;
	}

	/**
	 * @brief take reference
	 */
	void ref_incr() noexcept{
		reference_count_.fetch_add(1, std::memory_order_relaxed);
	}

	/**
	 * @brief drop referebce
	 * @return true if should be destructed
	 */
	bool ref_decr() noexcept{
		auto last = reference_count_.fetch_sub(1, std::memory_order_release);
		assert(last != 0);
		return last == 1;
	}


	template <typename T, typename D>
	friend struct referenced_ptr;
};

export
struct referenced_object_persistable{

private:
	mutable std::size_t reference_count_{};

public:
	[[nodiscard]] constexpr referenced_object_persistable() = default;

	[[nodiscard]] constexpr explicit referenced_object_persistable(tags::persistent_tag_t)
		: reference_count_(persistent_spec){
	}

	constexpr referenced_object_persistable(const referenced_object_persistable& other) noexcept{
	}

	constexpr referenced_object_persistable& operator=(const referenced_object_persistable& other){
		return *this;
	}

	constexpr referenced_object_persistable(referenced_object_persistable&& other) noexcept
		: reference_count_{}{
	}

	constexpr referenced_object_persistable& operator=(referenced_object_persistable&& other) noexcept{
		if(this == &other) return *this;
		return *this;
	}

protected:

	[[nodiscard]] constexpr std::size_t ref_count() const noexcept{
		return reference_count_;
	}

	[[nodiscard]] constexpr bool droppable() const noexcept{
		return reference_count_ == 0;
	}

	/**
	 * @brief take reference
	 */
	constexpr void ref_incr() const noexcept{
		if(is_persistent())return;
		++reference_count_;
		assert(reference_count_ != persistent_spec && "yes i know this should be impossible, just in case");
	}

	/**
	 * @brief drop referebce
	 * @return true if should be destructed
	 */
	constexpr bool ref_decr() const noexcept{
		if(is_persistent())return false;
		--reference_count_;
		return reference_count_ == 0;
	}

	[[nodiscard]] constexpr bool is_persistent() const noexcept{
		return reference_count_ == persistent_spec;
	}

	template <typename T, typename D>
	friend struct referenced_ptr;
};

template <typename T, typename D>
void referenced_ptr<T, D>::decr() noexcept{
	if constexpr(!no_delete_on_drop<D>){
		if(object->ref_decr()){
			this->delete_elem(object);
		}
	} else{
		assert(object->ref_count() > 0);
		object->ref_decr();
	}
}

template <typename T, typename D>
void referenced_ptr<T, D>::incr() noexcept{
	object->ref_incr();
}
}

template <typename T, typename D>
struct std::hash<mo_yanxi::referenced_ptr<T, D>>{
	static std::size_t operator()(const mo_yanxi::referenced_ptr<T, D>& ptr) noexcept{
		static constexpr std::hash<T*> hasher{};
		return hasher(ptr.get());
	}
};