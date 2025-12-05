//
// Created by Matrix on 2025/3/22.
//

export module mo_yanxi.spreadable_event_handler;

export import mo_yanxi.type_map;
import std;


namespace mo_yanxi{
	enum struct handle_result{
		pass,
		resolved,
	};

	export
	template <typename Dty, typename ...EventTs>
	struct spreadable_event_handler{
		using derived_type = Dty;

		using invoke_result_type = bool;
		using func_type = std::move_only_function<invoke_result_type(const void*, derived_type&) const>;
		using map_type = type_map_of<func_type, EventTs...>;

	private:
		map_type handlers{};

		/*
	protected:
		derived_type& spread_evnet_handler_get_self() noexcept{
			return static_cast<derived_type&>(*this);
		}
		*/

	public:
		// template <typename T, typename Fn>
		// 	requires map_type::is_type_valid<T> && std::is_invocable_r_v<invoke_result_type, Fn, const T, derived_type&>
		// func_type on(Fn&& fn){
		// 	return std::exchange(handlers.template at<T>(), [f = std::forward<Fn>(fn)](
		// 		const void* e, derived_type& s
		// 	) -> invoke_result_type {
		// 		return std::invoke(f, *static_cast<const T*>(e), s);
		// 	});
		// }

		template <typename T, typename Fn, std::derived_from<spreadable_event_handler> S>
			requires map_type::template is_type_valid<T> && std::is_invocable_r_v<invoke_result_type, Fn, const T, S&>
		func_type spreadable_event_handler_on(this S& self, Fn&& fn){
			return std::exchange(self.handlers.template at<T>(), [f = std::forward<Fn>(fn)](
				const void* e, derived_type& s
			) -> invoke_result_type {
				return std::invoke(f, *static_cast<const T*>(e), static_cast<S&>(s));
			});
		}

		template <typename T, std::derived_from<spreadable_event_handler> S>
			requires (map_type::template is_type_valid<T>)
		invoke_result_type spreadable_event_handler_fire(this S& self, const T& e) {
			const auto& fn = self.handlers.template at<T>();
			if(fn){
				return fn(static_cast<const void*>(&e), self);
			}else{
				return invoke_result_type{};
			}
		}
	};
}
