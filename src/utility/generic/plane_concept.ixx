//
// Created by Matrix on 2024/11/4.
//

export module ext.dim2.plane_concept;

import std;

namespace mo_yanxi::dim2{
	export
	template <typename P, typename N>
	concept position_acquireable = requires(const P& p){
		{ p.x } -> std::convertible_to<N>;
		{ p.y } -> std::convertible_to<N>;
	};

	export
	template <typename P, typename N>
	concept position_constructable = requires{
		requires std::constructible_from<P, N, N>;
	};
}

