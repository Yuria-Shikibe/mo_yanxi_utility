#pragma once

#include "assume.hpp"

#if defined(_MSC_VER)
#define ADAPTED_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define ADAPTED_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if  defined(__clang__)
	#define FORCE_INLINE [[gnu::always_inline]]
#elif _MSC_VER
	#define FORCE_INLINE [[msvc::forceinline]]
#else
	#define FORCE_INLINE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict
#elifdef _MSC_VER
#define RESTRICT __restrict
#endif

#if defined(__GNUC__) || defined(__clang__)
	#define PURE_FN [[gnu::pure]]
#elif _MSC_VER
	#define PURE_FN
#else
	#define PURE_FN
#endif


#if defined(__GNUC__) || defined(__clang__)
	#define CONST_FN [[gnu::const]]
#elif _MSC_VER
	#define CONST_FN
#else
	#define CONST_FN
#endif


#ifdef _MSC_VER
#define EMPTY_BASE __declspec(empty_bases)
#else
#define EMPTY_BASE
#endif


#if defined(_MSC_VER) && !defined(__clang__)
	#define ATTR_FLATTEN [[msvc::flatten]]
	#define ATTR_FORCEINLINE_SENTENCE [[msvc::forceinline_calls]]

// 针对 Clang 编译器
#elif defined(__clang__)
	// Clang 支持 GNU 的 flatten 属性，并且在较新版本中支持将 always_inline 作用于语句（调用点）
	#define ATTR_FLATTEN [[gnu::flatten]]
	#define ATTR_FORCEINLINE_SENTENCE

// 针对 GCC (GNU) 编译器
#elif defined(__GNUC__)
	// GCC 支持函数级别的 flatten，但目前没有直接对应于调用点/语句级别的强制内联属性
	#define ATTR_FLATTEN [[gnu::flatten]]
	#define ATTR_FORCEINLINE_SENTENCE

// 后备选项（未知或不支持的编译器）
#else
	#define ATTR_FLATTEN
	#define ATTR_FORCEINLINE_SENTENCE
#endif
