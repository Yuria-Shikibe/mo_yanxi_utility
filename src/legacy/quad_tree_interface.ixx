//
// Created by Matrix on 2024/5/31.
//

export module mo_yanxi.math.quad_tree.interface;

export import mo_yanxi.math.vector2;
export import mo_yanxi.math.rect_ortho;
import mo_yanxi.concepts;

export namespace mo_yanxi::math{
	template <typename Impl, arithmetic N = float>
	struct quad_tree_adaptor{
		using quad_tree_impl_type = Impl;
		using quad_tree_coord_type = N;
		using quad_tree_rect_type = rect_ortho<N>;
		using quad_tree_vector_type = vector2<N>;

		[[nodiscard]] quad_tree_rect_type quad_tree_get_bound() const noexcept = delete;

		[[nodiscard]] bool quad_tree_rough_intersect_with(const quad_tree_impl_type& other) const = delete;

		[[nodiscard]] bool quad_tree_exact_intersect_with(const quad_tree_impl_type& other) const = delete;

		[[nodiscard]] bool quad_tree_contains(typename quad_tree_vector_type::const_pass_t point) const = delete;
	};
}
