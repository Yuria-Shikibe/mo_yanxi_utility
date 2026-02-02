export module mo_yanxi.guard;

import std;

export namespace mo_yanxi{
	template <typename T, bool passByMove = std::is_move_assignable_v<T>>
		requires requires{
			requires std::is_object_v<T>;
			requires
			(passByMove && std::is_move_assignable_v<T>) ||
			(!passByMove && std::is_copy_assignable_v<T>);
		}
	class resumer{
		T& tgt;
		T original;

		static constexpr bool is_nothrow =
			(passByMove && std::is_nothrow_move_assignable_v<T>) ||
			(!passByMove && std::is_nothrow_copy_assignable_v<T>);

	public:
		[[nodiscard]] constexpr explicit(false) resumer(T& tgt) noexcept(is_nothrow) requires (!passByMove) :
			tgt{tgt}, original{tgt}{}

		[[nodiscard]] constexpr explicit(false) resumer(T& tgt) noexcept(is_nothrow) requires (passByMove) :
			tgt{tgt}, original{std::move(tgt)}{}

		[[nodiscard]] constexpr explicit(false) resumer(const T&& tgt) = delete;


		constexpr ~resumer() {
			if constexpr(passByMove){
				tgt = std::move(original);
			} else{
				tgt = original;
			}
		}
	};
}
