module;

#include <limits>

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.intersection;

export import mo_yanxi.math;
export import mo_yanxi.math.vector2;

import std;

namespace mo_yanxi::math{
	using fp_t = float;

	export
	/** Returns a point on the segment nearest to the specified point. */
	[[nodiscard]] FORCE_INLINE constexpr vec2 nearest_segment_point(const vec2 where, const vec2 start, const vec2 end) noexcept{
		const auto length2 = start.dst2(end);
		if(length2 == 0) [[unlikely]]{
			 return start;
		}

		const auto t = ((where.x - start.x) * (end.x - start.x) + (where.y - start.y) * (end.y - start.y)) / length2;
		if(t <= 0) return start;
		if(t >= 1) return end;
		return {start.x + t * (end.x - start.x), start.y + t * (end.y - start.y)};
	}

	// constexpr vector2<fp_t> intersectCenterPoint(const QuadBox& subject, const QuadBox& object) {
	// 	const fp_t x0 = Math::max(subject.v0.x, object.v0.x);
	// 	const fp_t y0 = Math::max(subject.v0.y, object.v0.y);
	// 	const fp_t x1 = Math::min(subject.v1.x, object.v1.x);
	// 	const fp_t y1 = Math::min(subject.v1.y, object.v1.y);
	//
	// 	return { (x0 + x1) * 0.5f, (y0 + y1) * 0.5f };
	// }

	export
	struct possible_point{
		vector2<fp_t> pos;
		bool valid;

		constexpr explicit operator bool() const noexcept{
			return valid;
		}

		[[nodiscard]] FORCE_INLINE constexpr vector2<fp_t> value_or(vector2<fp_t> vec) const noexcept{
			return valid ? pos : vec;
		}
	};


	export
	FORCE_INLINE constexpr fp_t dst2_to_line(vector2<fp_t> where, const vector2<fp_t> pointOnLine, const vector2<fp_t> lineDirVec) noexcept {
		if(lineDirVec.is_zero()) return where.dst2(pointOnLine);
		where.sub(pointOnLine);
		const auto dot  = where.dot(lineDirVec);
		const auto porj = dot * dot / lineDirVec.length2();

		return where.length2() - porj;
	}


	export
	FORCE_INLINE fp_t dst_to_line(const vector2<fp_t> vec2, const vector2<fp_t> pointOnLine, const vector2<fp_t> directionVec) noexcept {
		return std::sqrt(dst2_to_line(vec2, pointOnLine, directionVec));
	}

	export
	FORCE_INLINE fp_t dst_to_line_by_seg(const vector2<fp_t> vec2, const vector2<fp_t> p1_on_line, vector2<fp_t> p2_on_line) {
		p2_on_line -= p1_on_line;
		return dst_to_line(vec2, p1_on_line, p2_on_line);
	}

	export
	FORCE_INLINE fp_t dst2_to_line_by_seg(const vector2<fp_t> vec2, const vector2<fp_t> p1_on_line, vector2<fp_t> p2_on_line) {
		p2_on_line -= p1_on_line;
		return dst2_to_line(vec2, p1_on_line, p2_on_line);
	}

	export
	FORCE_INLINE fp_t dst_to_segment(const vector2<fp_t> p, const vector2<fp_t> a, const vector2<fp_t> b) noexcept {
		const auto nearest = nearest_segment_point(p, a, b);

		return nearest.dst(p);
	}

	export
	FORCE_INLINE constexpr fp_t dst2_to_segment(const vector2<fp_t> p, const vector2<fp_t> a, const vector2<fp_t> b) noexcept {
		const auto nearest = nearest_segment_point(p, a, b);

		return nearest.dst2(p);
	}

	[[deprecated]] vector2<fp_t> arrive(const vector2<fp_t> position, const vector2<fp_t> dest, const vector2<fp_t> curVel, const fp_t smooth, const fp_t radius, const fp_t tolerance) {
		auto toTarget = vector2<fp_t>{ dest - position };

		const fp_t distance = toTarget.length();

		if(distance <= tolerance) return {};
		fp_t targetSpeed = curVel.length();
		if(distance <= radius) targetSpeed *= distance / radius;

		return toTarget.sub(curVel.x / smooth, curVel.y / smooth).limit_max_length(targetSpeed);
	}

	export
	constexpr vector2<fp_t> intersection_line(
		const vector2<fp_t> p11, const vector2<fp_t> p12,
		const vector2<fp_t> p21, const vector2<fp_t> p22) {
		const fp_t x1 = p11.x, x2 = p12.x, x3 = p21.x, x4 = p22.x;
		const fp_t y1 = p11.y, y2 = p12.y, y3 = p21.y, y4 = p22.y;

		const fp_t dx1 = x1 - x2;
		const fp_t dy1 = y1 - y2;

		const fp_t dx2 = x3 - x4;
		const fp_t dy2 = y3 - y4;

		const fp_t det = dx1 * dy2 - dy1 * dx2;

		if (det == 0.0f) {
			return vector2<fp_t>{(x1 + x2) * 0.5f, (y1 + y2) * 0.5f};  // Return the midpoint of overlapping lines
		}

		const fp_t pre = x1 * y2 - y1 * x2, post = x3 * y4 - y3 * x4;
		const fp_t x   = (pre * dx2 - dx1 * post) / det;
		const fp_t y   = (pre * dy2 - dy1 * post) / det;

		return vector2<fp_t>{x, y};
	}

	// constexpr auto i = intersectionLine({-1, 0}, {1, 0}, {0, -1}, {0, 1});

	 [[deprecated]] std::optional<vector2<fp_t>>
	intersect_segments(const vector2<fp_t> p1, const vector2<fp_t> p2, const vector2<fp_t> p3, const vector2<fp_t> p4){
		const fp_t x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y, x3 = p3.x, y3 = p3.y, x4 = p4.x, y4 = p4.y;

		const fp_t dx1 = x2 - x1;
		const fp_t dy1 = y2 - y1;

		const fp_t dx2 = x4 - x3;
		const fp_t dy2 = y4 - y3;

		const fp_t d = dy2 * dx1 - dx2 * dy1;
		if(d == 0) return std::nullopt;

		const fp_t yd = y1 - y3;
		const fp_t xd = x1 - x3;

		const fp_t ua = (dx2 * yd - dy2 * xd) / d;
		if(ua < 0 || ua > 1) return std::nullopt;

		const fp_t ub = (dx1 * yd - dy1 * xd) / d;
		if(ub < 0 || ub > 1) return std::nullopt;

		return vector2<fp_t>{x1 + dx1 * ua, y1 + dy1 * ua};
	}

	export
	FORCE_INLINE constexpr bool
	intersect_segments(const vector2<fp_t> p1, const vector2<fp_t> p2, const vector2<fp_t> p3, const vector2<fp_t> p4, vector2<fp_t>& out) noexcept{
		const fp_t x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y, x3 = p3.x, y3 = p3.y, x4 = p4.x, y4 = p4.y;

		const fp_t dx1 = x2 - x1;
		const fp_t dy1 = y2 - y1;

		const fp_t dx2 = x4 - x3;
		const fp_t dy2 = y4 - y3;

		const fp_t d = dy2 * dx1 - dx2 * dy1;
		if(d == 0) return false;

		const fp_t yd = y1 - y3;
		const fp_t xd = x1 - x3;
		const fp_t ua = (dx2 * yd - dy2 * xd) / d;
		if(ua < 0 || ua > 1) return false;

		const fp_t ub = (dx1 * yd - dy1 * xd) / d;
		if(ub < 0 || ub > 1) return false;

		(void)out.set(x1 + dx1 * ua, y1 + dy1 * ua);
		return true;
	}

	using vec_t = vector2<fp_t>;

	export
	FORCE_INLINE constexpr possible_point ray_seg_intersection(
		const vec_t ray_cap, const vec_t ray_dir,
		const vec_t seg_v1, const vec_t seg_v2){
		const auto seg_dir = seg_v2 - seg_v1;
		const auto denom = seg_dir.cross(ray_dir);

		// 处理分母不为零的情况（射线与线段不平行）
		if(math::abs(denom) > 1e-6f){
			const auto ao = seg_v1 - ray_cap;
			const auto t_numerator = ao.cross(seg_dir);
			const auto s_numerator = ray_dir.cross(ao);

			const auto t = t_numerator / denom;
			const auto s = s_numerator / denom;

			if(t >= 0.0f && s >= 0.0f && s <= 1.0f){
				return {{ray_cap.x + t * ray_dir.x, ray_cap.y + t * ray_dir.y}, true};
			}
			return {{}, false};
		}

		// 处理共线情况
		const auto ab = seg_dir;
		const auto ao = ray_cap - seg_v1;
		const auto cross = ab.cross(ao);

		// 检查是否真正共线
		if(math::abs(cross) > 1e-6f) return {{}, false};


		// 检查射线起点是否在线段上
		const auto dot_ab = ab.dot(ab);
		const auto dot_ao_ab = ao.dot(ab);
		const auto k = dot_ab != 0 ? dot_ao_ab / dot_ab : 0;

		if(k >= 0 && k <= 1) return {ray_cap, true};

		// 计算线段端点对应的射线参数
		fp_t t_a, t_b;
		if(math::abs(ray_dir.x) > 1e-6f){
			t_a = (seg_v1.x - ray_cap.x) / ray_dir.x;
			t_b = (seg_v2.x - ray_cap.x) / ray_dir.x;
		} else{
			t_a = (seg_v1.y - ray_cap.y) / ray_dir.y;
			t_b = (seg_v2.y - ray_cap.y) / ray_dir.y;
		}

		// 确定有效交点
		const auto t_min = math::min(t_a, t_b);
		const auto t_max = math::max(t_a, t_b);

		if(t_max < 0) return {{}, false};
		if(t_min > 0) return {(t_a < t_b) ? seg_v1 : seg_v2, true};

		// 计算线段参数s
		if(math::abs(t_a - t_b) < 1e-6f) return {{}, false};
		const auto s_val = (0 - t_a) / (t_b - t_a);

		if(s_val >= 0 && s_val <= 1){
			return {
					vec_t{
						seg_v1.x + s_val * seg_dir.x,
						seg_v1.y + s_val * seg_dir.y
					},
					true
				};
		}

		// 返回有效端点
		const bool a_valid = t_a >= 0;
		const bool b_valid = t_b >= 0;

		if(a_valid && b_valid) return {(t_a < t_b) ? seg_v1 : seg_v2, true};
		if(a_valid) return {seg_v1, true};
		if(b_valid) return {seg_v2, true};

		return {{}, false};
	}


	export
	FORCE_INLINE fp_t ray_seg_intersection_dst(const vec_t ray_origin, const vec_t ray_dir,
	                               const vec_t seg_v1, const vec_t seg_v2) {
	    constexpr auto inf = std::numeric_limits<fp_t>::infinity();

	    // 计算线段方向向量
	    const auto seg_dir = seg_v2 - seg_v1;

	    // 计算分母（叉积）
	    const auto denom = seg_dir.cross(ray_dir);

	    // 处理非平行情况
	    if (math::abs(denom) > 1e-6f) {
	        const vec_t ao{seg_v1.x - ray_origin.x, seg_v1.y - ray_origin.y};

	        // 计算射线参数t和线段参数s
	        const auto t = (ao.y * seg_dir.x - ao.x * seg_dir.y) / denom;
	        const auto s = (ray_dir.x * ao.y - ray_dir.y * ao.x) / denom;

	        // 检查交点是否有效
	        if (t >= 0.0f && s >= 0.0f && s <= 1.0f) {
	            // 计算交点并返回距离
	            const vec_t intersection{
	                ray_origin.x + t * ray_dir.x,
	                ray_origin.y + t * ray_dir.y
	            };
	            const float dx = intersection.x - ray_origin.x;
	            const float dy = intersection.y - ray_origin.y;
	            return std::hypot(dx, dy);
	        }
	        return inf;
	    }

	    // 处理平行或共线情况
	    // 检查是否共线
	    const vec2 ao{seg_v1.x - ray_origin.x, seg_v1.y - ray_origin.y};
	    const auto cross = ray_dir.x * ao.y - ray_dir.y * ao.x;
	    if (math::abs(cross) > 1e-6f) return inf; // 平行但不共线

	    // 共线情况 - 计算线段端点在射线上的投影
	    auto t1 = (seg_v1.x - ray_origin.x) / (math::abs(ray_dir.x) > 1e-6f ? ray_dir.x : 1e-6f);
	    auto t2 = (seg_v2.x - ray_origin.x) / (math::abs(ray_dir.x) > 1e-6f ? ray_dir.x : 1e-6f);

	    // 如果射线方向y分量较大，使用y坐标计算
	    if (math::abs(ray_dir.y) > math::abs(ray_dir.x)) {
	        t1 = (seg_v1.y - ray_origin.y) / ray_dir.y;
	        t2 = (seg_v2.y - ray_origin.y) / ray_dir.y;
	    }

	    // 找到有效的t值（t >= 0）
	    auto min_t = inf;
	    if (t1 >= 0) min_t = math::min(min_t, t1);
	    if (t2 >= 0) min_t = math::min(min_t, t2);

	    // 如果找到有效t值，计算距离
	    if (min_t != inf) {
	        return min_t * std::hypot(ray_dir.x, ray_dir.y);
	    }

	    return inf;
	}
	// OrthoRectFloat maxContinousBoundOf(const std::vector<QuadBox>& traces) {
	// 	const auto& front = traces.front().getMaxOrthoBound();
	// 	const auto& back  = traces.back().getMaxOrthoBound();
	// 	auto [minX, maxX] = std::minmax({ front.getSrcX(), front.getEndX(), back.getSrcX(), back.getEndX() });
	// 	auto [minY, maxY] = std::minmax({ front.getSrcY(), front.getEndY(), back.getSrcY(), back.getEndY() });
	//
	// 	return OrthoRectFloat{ minX, minY, maxX - minX, maxY - minY };
	// }
	//
	// template <ext::number T>
	// bool overlap(const T x, const T y, const T radius, const Rect_Orthogonal<T>& rect) {
	// 	T closestX = std::clamp<T>(x, rect.getSrcX(), rect.getEndX());
	// 	T closestY = std::clamp<T>(y, rect.getSrcY(), rect.getEndY());
	//
	// 	T distanceX       = x - closestX;
	// 	T distanceY       = y - closestY;
	// 	T distanceSquared = distanceX * distanceX + distanceY * distanceY;
	//
	// 	return distanceSquared <= radius * radius;
	// }

	// template <ext::number T>
	// bool overlap(const Circle<T>& circle, const Rect_Orthogonal<T>& rect) {
	// 	return Geom::overlap<T>(circle.getCX(), circle.getCY(), circle.getRadius(), rect);
	// }
}
