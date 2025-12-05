#pragma once

#include <cassert>

#ifdef __clang__

#define CHECKED_ASSUME(no_side_effect_expr) \
assert(no_side_effect_expr);\
[[assume(no_side_effect_expr)]];\

#elifdef _MSC_VER

#define CHECKED_ASSUME(no_side_effect_expr)\
	assert(no_side_effect_expr);\
	__assume(no_side_effect_expr);\

#else

#define CHECKED_ASSUME(no_side_effect_expr) assert(no_side_effect_expr);


#endif

