module;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.allocator_aware_unique_ptr;

import std;
namespace mo_yanxi{
export
template <typename T, typename Alloc>
class allocator_aware_unique_ptr{
public:
	// 类型别名
	using pointer = T*;
	using element_type = T;
	// 自动重绑定分配器，确保 Alloc 是针对 T 的
	//TODO 同stl一样确保它们的type一致？
	using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

private:
	ADAPTED_NO_UNIQUE_ADDRESS allocator_type alloc_;
	pointer ptr_ = nullptr;

	using traits = std::allocator_traits<allocator_type>;

public:
	// =========================================================================
	// 构造函数
	// =========================================================================

	// 默认构造
	constexpr allocator_aware_unique_ptr() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
	: alloc_(), ptr_(nullptr){
	}

	constexpr allocator_aware_unique_ptr(
		std::nullptr_t) noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
	: alloc_(), ptr_(nullptr){
	}

	constexpr allocator_aware_unique_ptr(pointer p, const allocator_type& alloc) noexcept
	: alloc_(alloc), ptr_(p){
	}

	constexpr allocator_aware_unique_ptr(pointer p, allocator_type&& alloc) noexcept
	: alloc_(std::move(alloc)), ptr_(p){
	}

	// 禁止拷贝
	allocator_aware_unique_ptr(const allocator_aware_unique_ptr&) = delete;
	allocator_aware_unique_ptr& operator=(const allocator_aware_unique_ptr&) = delete;

	// 移动构造
	// 总是移动分配器
	constexpr allocator_aware_unique_ptr(
		allocator_aware_unique_ptr&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
	: alloc_(std::move(other.alloc_)), ptr_(std::exchange(other.ptr_, nullptr)){
	}


	~allocator_aware_unique_ptr(){
		if(ptr_ != nullptr){
			traits::destroy(alloc_, ptr_);
			traits::deallocate(alloc_, ptr_, 1);
		}
	}

	constexpr allocator_aware_unique_ptr& operator=(
		allocator_aware_unique_ptr&& other) noexcept(std::is_nothrow_move_assignable_v<allocator_type>){
		if(this != &other){
			reset(); // 先释放当前资源
			alloc_ = std::move(other.alloc_); // 传播分配器 (POCCA 语义在这里是强制的)
			ptr_ = std::exchange(other.ptr_, nullptr);
		}
		return *this;
	}

	template <typename U, typename UAlloc>
		requires (std::convertible_to<U*, T*> && std::convertible_to<UAlloc, allocator_type>)
	constexpr allocator_aware_unique_ptr& operator=(allocator_aware_unique_ptr<U, UAlloc>&& other) noexcept{
		reset();
		alloc_ = std::move(other.get_allocator());
		ptr_ = other.release();
		return *this;
	}

	constexpr allocator_aware_unique_ptr& operator=(std::nullptr_t) noexcept{
		reset();
		return *this;
	}

	constexpr pointer release() noexcept{
		return std::exchange(ptr_, nullptr);
	}

	constexpr void reset(pointer p = nullptr) noexcept{
		if(ptr_ != nullptr){
			// 销毁对象
			traits::destroy(alloc_, ptr_);
			// 释放内存 (假设只分配了1个对象)
			traits::deallocate(alloc_, ptr_, 1);
		}
		ptr_ = p;
	}

	// 交换
	constexpr void swap(allocator_aware_unique_ptr& other) noexcept{
		using std::swap;
		swap(alloc_, other.alloc_);
		swap(ptr_, other.ptr_);
	}

	[[nodiscard]] constexpr pointer get() const noexcept{
		return ptr_;
	}

	[[nodiscard]] constexpr allocator_type& get_allocator() noexcept{
		return alloc_;
	}

	[[nodiscard]] constexpr const allocator_type& get_allocator() const noexcept{
		return alloc_;
	}

	explicit constexpr operator bool() const noexcept{
		return ptr_ != nullptr;
	}

	[[nodiscard]] constexpr std::add_lvalue_reference_t<T> operator
	*() const noexcept(noexcept(*std::declval<pointer>())){
		return *ptr_;
	}

	[[nodiscard]] constexpr pointer operator->() const noexcept{
		return ptr_;
	}

	// =========================================================================
	// 比较运算符 (C++20 spaceship)
	// =========================================================================

	friend constexpr bool operator==(const allocator_aware_unique_ptr& lhs,
		const allocator_aware_unique_ptr& rhs) noexcept{
		return lhs.get() == rhs.get();
	}

	friend constexpr bool operator==(const allocator_aware_unique_ptr& lhs, std::nullptr_t) noexcept{
		return lhs.get() == nullptr;
	}

	// 支持与其他指针比较
	friend constexpr auto operator<=>(const allocator_aware_unique_ptr& lhs,
		const allocator_aware_unique_ptr& rhs) noexcept{
		return std::compare_three_way{}(lhs.get(), rhs.get());
	}
};

export
template <typename T, typename Alloc>
constexpr void swap(allocator_aware_unique_ptr<T, Alloc>& lhs, allocator_aware_unique_ptr<T, Alloc>& rhs) noexcept{
	lhs.swap(rhs);
}

// =========================================================================
// 辅助构造函数 (Factory) - 强烈推荐使用此方法
// =========================================================================
export
template <typename T, typename Alloc, typename... Args>
[[nodiscard]] constexpr auto make_allocate_aware_unique(const Alloc& alloc, Args&&... args){
	// Rebind 分配器
	using AllocType = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;
	AllocType a(alloc);

	T* p = std::allocator_traits<AllocType>::allocate(a, 1);

	try{
		if constexpr(std::constructible_from<T, const AllocType&, Args&&...>){
			std::allocator_traits<AllocType>::construct(a, p, a, std::forward<Args>(args)...);
		} else if constexpr(std::constructible_from<T, Args&&..., const AllocType&>){
			std::allocator_traits<AllocType>::construct(a, p, std::forward<Args>(args)..., a);
		} else{
			std::allocator_traits<AllocType>::construct(a, p, std::forward<Args>(args)...);
		}
	} catch(...){
		std::allocator_traits<AllocType>::deallocate(a, p, 1);
		throw;
	}

	return allocator_aware_unique_ptr<T, AllocType>(p, std::move(a));
}

export
template <typename T, typename Alloc>
class allocator_aware_poly_unique_ptr{
public:
	using pointer = T*;
	using element_type = T;
	// 确保分配器是针对 T 的 (Base type)
	using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

	// 【关键点】删除器类型定义
	// 我们需要传入 allocator 来进行正确的 deallocate，因此不能只是 void(*)(void*)
	// 这里使用 T* 而不是 void* 以保持一定的类型安全性，但在内部实现中会转换
	using deleter_type = void(*)(T*, allocator_type&) noexcept;

private:
	ADAPTED_NO_UNIQUE_ADDRESS allocator_type alloc_;
	pointer ptr_ = nullptr;
	deleter_type deleter_ = nullptr; // 存储类型擦除后的删除函数

	using traits = std::allocator_traits<allocator_type>;

public:
	// =========================================================================
	// 构造函数
	// =========================================================================

	constexpr allocator_aware_poly_unique_ptr() noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
	: alloc_(), ptr_(nullptr), deleter_(nullptr){
	}

	constexpr allocator_aware_poly_unique_ptr(
		std::nullptr_t) noexcept(std::is_nothrow_default_constructible_v<allocator_type>)
	: alloc_(), ptr_(nullptr), deleter_(nullptr){
	}

	// 必须提供 deleter，因为无法推导多态类型的销毁逻辑
	constexpr allocator_aware_poly_unique_ptr(pointer p, const allocator_type& alloc, deleter_type d) noexcept
	: alloc_(alloc), ptr_(p), deleter_(d){
	}

	constexpr allocator_aware_poly_unique_ptr(pointer p, allocator_type&& alloc, deleter_type d) noexcept
	: alloc_(std::move(alloc)), ptr_(p), deleter_(d){
	}

	// 禁止拷贝
	allocator_aware_poly_unique_ptr(const allocator_aware_poly_unique_ptr&) = delete;
	allocator_aware_poly_unique_ptr& operator=(const allocator_aware_poly_unique_ptr&) = delete;

	// 移动构造
	constexpr allocator_aware_poly_unique_ptr(
		allocator_aware_poly_unique_ptr&& other) noexcept(std::is_nothrow_move_constructible_v<allocator_type>)
	: alloc_(std::move(other.alloc_))
	, ptr_(std::exchange(other.ptr_, nullptr))
	, deleter_(std::exchange(other.deleter_, nullptr)){
	}

	// 析构函数
	~allocator_aware_poly_unique_ptr(){
		reset();
	}

	// 移动赋值
	constexpr allocator_aware_poly_unique_ptr& operator=(
		allocator_aware_poly_unique_ptr&& other) noexcept(std::is_nothrow_move_assignable_v<allocator_type>){
		if(this != &other){
			reset();
			alloc_ = std::move(other.alloc_);
			ptr_ = std::exchange(other.ptr_, nullptr);
			deleter_ = std::exchange(other.deleter_, nullptr);
		}
		return *this;
	}

	constexpr allocator_aware_poly_unique_ptr& operator=(std::nullptr_t) noexcept{
		reset();
		return *this;
	}

	// =========================================================================
	// 资源管理
	// =========================================================================

	constexpr pointer release() noexcept{
		deleter_ = nullptr;
		return std::exchange(ptr_, nullptr);
	}

	// 这里的 reset 只允许 nullptr，不允许重置为新的裸指针
	// 因为重置为裸指针需要同时提供对应的 deleter，防止逻辑错误
	constexpr void reset(std::nullptr_t = nullptr) noexcept{
		if(ptr_ != nullptr && deleter_ != nullptr){
			// 调用类型擦除的删除器，传入 ptr 和 alloc
			deleter_(ptr_, alloc_);
		}
		ptr_ = nullptr;
		deleter_ = nullptr;
	}

	constexpr void swap(allocator_aware_poly_unique_ptr& other) noexcept{
		using std::swap;
		swap(alloc_, other.alloc_);
		swap(ptr_, other.ptr_);
		swap(deleter_, other.deleter_);
	}

	// =========================================================================
	// 观察器
	// =========================================================================

	[[nodiscard]] constexpr pointer get() const noexcept{ return ptr_; }
	[[nodiscard]] constexpr allocator_type& get_allocator() noexcept{ return alloc_; }
	[[nodiscard]] constexpr const allocator_type& get_allocator() const noexcept{ return alloc_; }

	explicit constexpr operator bool() const noexcept{ return ptr_ != nullptr; }

	[[nodiscard]] constexpr std::add_lvalue_reference_t<T> operator
	*() const noexcept(noexcept(*std::declval<pointer>())){
		return *ptr_;
	}

	[[nodiscard]] constexpr pointer operator->() const noexcept{
		return ptr_;
	}

	// ... (此处省略 comparison operators，实现逻辑与原版一致) ...
};


// 内部使用的静态删除器生成模板
template <typename Concrete, typename Base, typename Alloc>
void poly_deleter_impl(Base* ptr, Alloc& alloc) noexcept{
	// 1. 还原具体类型指针
	Concrete* p = static_cast<Concrete*>(ptr);

	// 2. Rebind 分配器到具体类型
	using ConcreteAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Concrete>;
	ConcreteAlloc ca(alloc); // 从 BaseAlloc 构造 ConcreteAlloc (状态传递)

	// 3. 销毁对象
	std::allocator_traits<ConcreteAlloc>::destroy(ca, p);

	// 4. 释放内存 (使用正确的大小 sizeof(Concrete))
	std::allocator_traits<ConcreteAlloc>::deallocate(ca, p, 1);
}


export
template <typename T, typename Alloc>
constexpr void swap(allocator_aware_poly_unique_ptr<T, Alloc>& lhs,
	allocator_aware_poly_unique_ptr<T, Alloc>& rhs) noexcept{
	lhs.swap(rhs);
}

// =========================================================================
// 核心：多态 Factory 函数
// =========================================================================

/*
 * Factory 函数
 * Concrete: 实际创建的子类类型
 * Base:     unique_ptr 持有的基类类型
 * Alloc:    传入的分配器类型
 */
export
template <typename Concrete, typename Base, typename Alloc, typename... Args>
	requires std::derived_from<Concrete, Base>
[[nodiscard]] constexpr auto make_allocate_aware_poly_unique(const Alloc& alloc, Args&&... args){
	// 1. 准备具体类型的分配器
	using ConcreteAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Concrete>;
	ConcreteAlloc ca(alloc);

	// 2. 分配内存
	Concrete* p = std::allocator_traits<ConcreteAlloc>::allocate(ca, 1);

	// 3. 构造对象 (异常安全处理)
	try{
		if constexpr(std::constructible_from<Concrete, const ConcreteAlloc&, Args&&...>){
			std::allocator_traits<ConcreteAlloc>::construct(ca, p, ca, std::forward<Args>(args)...);
		} else if constexpr(std::constructible_from<Concrete, Args&&..., const ConcreteAlloc&>){
			std::allocator_traits<ConcreteAlloc>::construct(ca, p, std::forward<Args>(args)..., ca);
		} else{
			std::allocator_traits<ConcreteAlloc>::construct(ca, p, std::forward<Args>(args)...);
		}
	} catch(...){
		std::allocator_traits<ConcreteAlloc>::deallocate(ca, p, 1);
		throw;
	}

	// 4. 准备 Base 类型的分配器用于存储在 unique_ptr 中
	using BaseAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Base>;

	// 5. 获取类型擦除的删除器函数指针
	// 这里实例化了一个针对 Concrete 类型的静态函数
	auto deleter_func = &poly_deleter_impl<Concrete, Base, BaseAlloc>;

	// 6. 返回多态指针
	return allocator_aware_poly_unique_ptr<Base, BaseAlloc>(p, BaseAlloc(alloc), deleter_func);
}
}
