export module mo_yanxi.meta_programming;

export import mo_yanxi.tuple_manipulate;
import std;

namespace mo_yanxi{
export
struct pred_always{
	template <typename... Args>
	constexpr static bool operator()(Args&&... args) noexcept{
		return (void)(((void)args), ...), true;
	}
};

export
struct pred_never{
	template <typename... Args>
	constexpr static bool operator()(Args&&... args) noexcept{
		return (void)(((void)args), ...), false;
	}
};

export
template <typename From, typename To>
struct copy_const : std::type_identity<To>{
};

export
template <typename From, typename To>
struct copy_const<const From, To> : std::type_identity<const To>{
};

export
template <typename From, typename To>
using copy_const_t = typename copy_const<From, To>::type;

export
template <typename From, typename To>
struct copy_qualifier : std::type_identity<To>{
};

#define ArrangeGen(qfl, qfr) template <typename From, typename To> struct copy_qualifier<qfl From qfr, To> : std::type_identity<qfl To qfr>{};

ArrangeGen(const,);

ArrangeGen(const volatile,);

ArrangeGen(, &);

ArrangeGen(, &&);

ArrangeGen(const, &);

ArrangeGen(const, &&);

ArrangeGen(const volatile, &);

ArrangeGen(const volatile, &&);

export
template <typename From, typename To>
using copy_qualifier_t = typename copy_qualifier<From, To>::type;


// using A = binary_apply_to_tuple_t<copy_qualifier_t, std::tuple<int&, const double>, std::tuple<float, volatile short>>;


export
template <typename...>
struct static_assert_trigger : std::false_type{
};

export
template <typename T>
struct to_signed : std::type_identity<std::make_signed_t<T>>{
};

template <std::floating_point T>
struct to_signed<T> : std::type_identity<T>{
};

template <typename T>
	requires std::same_as<std::remove_cv_t<T>, bool>
struct to_signed<T> : std::type_identity<void>{
	// ReSharper disable once CppStaticAssertFailure
	static_assert(static_assert_trigger<T>::value, "to signed applied to bool type");
};

export template <typename T>
using to_signed_t = typename to_signed<T>::type;

export
template <typename T>
struct function_traits{
};


template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)>{
	using return_type = Ret;
	using args_type = std::tuple<Args...>;
	using mem_func_args_type = std::tuple<Args...>;

	static constexpr std::size_t args_count = std::tuple_size_v<args_type>;
	static constexpr bool is_single = args_count == 1;

	template <typename Func>
	static constexpr bool is_invocable = std::is_invocable_r_v<Ret, Func, Args...>;

	template <typename Func>
	static constexpr bool invocable_as_r_v(){
		return std::is_invocable_r_v<Ret, Func, Args...>;
	}

	template <typename Func>
	static constexpr bool invocable_as_v(){
		return std::is_invocable_v<Func, Args...>;
	}
};

template <typename Ret, typename T, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : function_traits<Ret(T*, Args...)>{
	using mem_func_args_type = std::tuple<Args...>;
};

// export
// template <typename T>
// struct normalized_function{
// 	using type = std::remove_pointer_t<T>;
// };
//
// template <typename T>
// 	requires std::is_member_function_pointer_v<T>
// struct normalized_function<T>{
// 	using type = std::remove_pointer_t<T>;
// };

#define VariantFunc(ext) template<typename Ret, typename... Args> struct function_traits<Ret(Args...) ext> : function_traits<Ret(Args...)>{};
VariantFunc(&);
VariantFunc(&&);
VariantFunc(const);
VariantFunc(const &);
VariantFunc(noexcept);
VariantFunc(& noexcept);
VariantFunc(&& noexcept);
VariantFunc(const noexcept);
VariantFunc(const& noexcept);

#define VariantMemberFunc(ext) template<typename Ret, typename T, typename... Args> struct function_traits<Ret(T::*)(Args...) ext> : function_traits<Ret(T::*)(Args...)>{};
VariantMemberFunc(&);
VariantMemberFunc(&&);
VariantMemberFunc(const);
VariantMemberFunc(const &);
VariantMemberFunc(noexcept);
VariantMemberFunc(& noexcept);
VariantMemberFunc(&& noexcept);
VariantMemberFunc(const noexcept);
VariantMemberFunc(const& noexcept);

template <typename Ret, typename T>
struct function_traits<Ret T::*> : function_traits<Ret(T::*)()>{
};


export
template <typename Ty>
	requires (std::is_class_v<Ty> && std::is_void_v<std::void_t<decltype(&Ty::operator())>>)
struct function_traits<Ty> : function_traits<std::remove_pointer_t<decltype(&Ty::operator())>>{
};


export
template <typename T>
using remove_mfptr_this_args = std::conditional_t<std::is_function_v<T>, typename function_traits<T>::args_type,
	typename function_traits<T>::mem_func_args_type>;


export
template <std::size_t N, typename FuncType>
struct function_arg_at{
	using trait = function_traits<FuncType>;
	static_assert(N < trait::args_count, "error: invalid parameter index.");
	using type = std::tuple_element_t<N, typename trait::args_type>;
};

export
template <typename FuncType>
struct function_arg_at_last{
	using trait = function_traits<FuncType>;
	using type = std::tuple_element_t<trait::args_count - 1, typename trait::args_type>;
};



export
template <typename Dst>
struct convert_to{
	template <typename T>
	constexpr Dst operator()(T&& src) const noexcept(noexcept(Dst{std::forward<T>(src)})){
		static_assert(std::constructible_from<Dst, T>);
		return Dst{std::forward<T>(src)};
	}
};

template <bool Test, auto val1, decltype(val1) val2>
struct conditional_constexpr_val{
	static constexpr auto value = val1;
};

template <auto val1, decltype(val1) val2>
struct conditional_constexpr_val<false, val1, val2>{
	static constexpr auto value = val1;
};

export
template <bool Test, auto val1, decltype(val1) val2>
constexpr auto conditional_v = conditional_constexpr_val<Test, val1, val2>::value;

export
template <typename T, typename = void>
struct is_complete_type : std::false_type{
};

template <typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))> : std::true_type{
};

export
template <typename T>
constexpr bool is_complete = is_complete_type<T>::value;

}

namespace mo_yanxi{

export
template <typename MemberPtr>
struct mptr_info;

template <typename C, typename T>
struct mptr_info<T C::*>{
	using class_type = C;
	using value_type = T;
};


template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...)>{
	using class_type = C;
	using value_type = T;
	using func_args = std::tuple<Args...>;
};

//
template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) const> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) &> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) &&> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) const &> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) const noexcept> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) & noexcept> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) && noexcept> : mptr_info<T (C::*)(Args...)>{
};

template <typename C, typename T, typename... Args>
struct mptr_info<T (C::*)(Args...) const & noexcept> : mptr_info<T (C::*)(Args...)>{
};

export
template <typename T>
concept default_hashable = requires(const T& t){
	requires std::is_default_constructible_v<std::hash<T>>;
	requires requires(const std::hash<T>& hasher){
		{ hasher.operator()(t) } noexcept -> std::same_as<std::size_t>;
	};
};


/*
template <bool Test, class T>
struct ConstConditional{
	using type = T;
};


template <class T>
struct ConstConditional<true, T>{
	using type = std::add_const_t<T>;
};


template <class T>
struct ConstConditional<true, T&>{
	using type = std::add_lvalue_reference_t<std::add_const_t<T>>;
};


template <typename T>
constexpr bool isConstRef = std::is_const_v<std::remove_reference_t<T>>;*/

template <typename T>
struct is_const_lvalue_reference{
	static constexpr bool value = false;
};

template <typename T>
struct is_const_lvalue_reference<T&>{
	static constexpr bool value = false;
};

template <typename T>
struct is_const_lvalue_reference<const T&>{
	static constexpr bool value = true;
};

template <typename T>
struct is_const_lvalue_reference<const volatile T&>{
	static constexpr bool value = true;
};

template <typename T>
constexpr bool is_const_lvalue_reference_v = is_const_lvalue_reference<T>::value;

export
template <typename Tuple>
struct type_to_index{
	static_assert(is_tuple_v<Tuple>, "accept tuple only");

	static constexpr std::size_t arg_size = std::tuple_size_v<Tuple>;
	using args_type = Tuple;

	template <std::size_t N>
		requires (N < arg_size)
	using arg_at = std::tuple_element_t<N, Tuple>;

private:
	template <typename V, std::size_t I>
	static consteval void modify(std::size_t& val) noexcept{
		if constexpr(std::same_as<arg_at<I>, V>){
			val = I;
		}
	}

	template <typename V, std::size_t... I>
	static consteval void indexOfImpl(std::size_t& rst, std::index_sequence<I...>){
		(modify<V, I>(rst), ...);
	}

	template <typename V>
	static consteval auto retIndexOfImpl(){
		std::size_t result{arg_size};

		type_to_index::indexOfImpl<V>(result, std::make_index_sequence<arg_size>{});

		return result;
	}

public:
	template <typename V>
	static constexpr std::size_t index_of = retIndexOfImpl<V>();

	template <typename V>
	static constexpr bool is_type_valid = index_of<V> < arg_size;

	friend bool operator==(const type_to_index&, const type_to_index&) noexcept = default;
	auto operator<=>(const type_to_index&) const noexcept = default;
};

}
