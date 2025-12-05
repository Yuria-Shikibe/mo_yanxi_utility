//
// Created by Matrix on 2024/10/25.
//

export module mo_yanxi.func_initialzer;

import std;
export import mo_yanxi.meta_programming;

namespace mo_yanxi{
	export
	template <typename Fn, typename Base>
	concept func_initializer_of = requires{
		requires mo_yanxi::is_complete<function_traits<std::decay_t<Fn>>>;
		typename function_arg_at_last<std::decay_t<Fn>>::type;
		requires std::is_lvalue_reference_v<typename function_arg_at_last<std::decay_t<Fn>>::type>;
		requires std::derived_from<std::remove_cvref_t<typename function_arg_at_last<std::decay_t<Fn>>::type>, Base>;
	};

	export
	template <typename Fn, typename Base, typename ...Args>
	concept invocable_func_initializer_of = requires{
		requires func_initializer_of<Fn, Base>;
		requires std::invocable<
			Fn,
			std::add_lvalue_reference_t<std::remove_cvref_t<typename function_arg_at_last<std::decay_t<Fn>>::type>>,
			Args...
		>;
	};

	export
	template <typename InitFunc>
	struct func_initializer_trait{
		using target_type = std::remove_cvref_t<typename function_arg_at_last<std::decay_t<InitFunc>>::type>;
	};
}
