module ;

#include "mo_yanxi/adapted_attributes.hpp"

export module mo_yanxi.math.vector3;

import mo_yanxi.concepts;
export import mo_yanxi.math;
import std;

export namespace mo_yanxi::math{
	template <arithmetic T>
	struct vector3{
		T x, y, z;

		using value_type = T;
		using floating_point_t = std::conditional_t<std::floating_point<T>, T,
		                                            std::conditional_t<sizeof(T) <= 4, float, double>
		>;
		using const_pass_t = mo_yanxi::conditional_pass_type<const vector3, sizeof(T) * 3>;

		[[nodiscard]] FORCE_INLINE static constexpr auto length(const T x, const T y, const T z) noexcept{
			return std::hypot(x, y, z);
		}

		[[nodiscard]] FORCE_INLINE static constexpr T length2(const T x, const T y, const T z) noexcept{
			return x * x + y * y + z * z;
		}

		/** @return The Euclidean distance between the two specified vectors */
		[[nodiscard]] FORCE_INLINE static constexpr T dst(const T x1, const T y1, const T z1, const T x2, const T y2,
		                                                  const T z2) noexcept{
			return std::sqrt(vector3::dst2(x1, x2, y1, y2, z1, z2));
		}

		/** @return the squared distance between the given points */
		[[nodiscard]] FORCE_INLINE static constexpr T dst2(const T x1, const T y1, const T z1, const T x2, const T y2,
		                                                   const T z2) noexcept{
			const T a = math::dst_safe(x2 - x1);
			const T b = math::dst_safe(y2 - y1);
			const T c = math::dst_safe(z2 - z1);
			return a * a + b * b + c * c;
		}

		/** @return The dot product between the two vectors */
		[[nodiscard]] FORCE_INLINE static constexpr T dot(const T x1, const T y1, const T z1, const T x2, const T y2,
		                                                  const T z2) noexcept{
			return x1 * x2 + y1 * y2 + z1 * z2;
		}

		FORCE_INLINE constexpr vector3& div(const_pass_t other) noexcept{
			x /= other.x;
			y /= other.y;
			z /= other.z;

			return *this;
		}

		FORCE_INLINE constexpr vector3& set(const_pass_t other) noexcept{
			this->operator=(other);
			return *this;
		}

		FORCE_INLINE constexpr vector3& set(const T x, const T y, const T z) noexcept{
			this->x = x;
			this->y = y;
			this->z = z;
			return *this;
		}

		/**
		 * Sets the components from the given spherical coordinate
		 * @param azimuthalAngle The angle between x-axis in radians [0, 2pi]
		 * @param polarAngle The angle between z-axis in radians [0, pi]
		 * @return This vector for chaining
		 */
		FORCE_INLINE constexpr vector3& from_spherical(const float azimuthalAngle, const float polarAngle) noexcept{
			const float cosPolar = math::cos(polarAngle);
			const float sinPolar = math::sin(polarAngle);

			const float cosAzim = math::cos(azimuthalAngle);
			const float sinAzim = math::sin(azimuthalAngle);

			return this->set(cosAzim * sinPolar, sinAzim * sinPolar, cosPolar);
		}

		[[nodiscard]] constexpr vector3& copy() noexcept{
			return vector3{*this};
		}

		FORCE_INLINE constexpr vector3& add(const_pass_t vector) noexcept{
			return this->add(vector.x, vector.y, vector.z);
		}

		FORCE_INLINE constexpr vector3& add(const_pass_t vector, const T scale) noexcept{
			return this->add(vector.x * scale, vector.y * scale, vector.z * scale);
		}

		FORCE_INLINE constexpr vector3& sub(const_pass_t vector, const T scale) noexcept{
			return this->sub(vector.x * scale, vector.y * scale, vector.z * scale);
		}

		FORCE_INLINE constexpr vector3& add(const T x, const T y, const T z) noexcept{
			this->x += x;
			this->y += y;
			this->z += z;

			return *this;
		}


		FORCE_INLINE constexpr vector3& add(const T val) noexcept{
			return this->add(val, val, val);
		}


		FORCE_INLINE constexpr vector3& sub(const_pass_t other) noexcept{
			return this->sub(other.x, other.y, other.z);
		}

		FORCE_INLINE constexpr vector3& sub(const T x, const T y, const T z) noexcept{
			this->x -= x;
			this->y -= y;
			this->z -= z;
			return *this;
		}

		FORCE_INLINE constexpr vector3& sub(const T value) noexcept{
			return this->sub(value, value, value);
		}

		FORCE_INLINE constexpr vector3& scl(const T x, const T y, const T z) noexcept{
			this->x *= x;
			this->y *= y;
			this->z *= z;
			return *this;
		}

		FORCE_INLINE constexpr vector3& scl(const T scalar) noexcept{
			this->x *= scalar;
			this->y *= scalar;
			this->z *= scalar;
			return *this;
		}

		FORCE_INLINE constexpr vector3& scl(const_pass_t other) noexcept{
			return this->scl(other.x, other.y, other.z);
		}

		FORCE_INLINE constexpr vector3& fma(vector3& vec, const T scalar) noexcept{
			this->x = math::fma(vec.x, scalar, x);
			this->y = math::fma(vec.y, scalar, y);
			this->z = math::fma(vec.z, scalar, z);
			return *this;
		}

		FORCE_INLINE constexpr vector3& mulAdd(vector3& vec, vector3& mulVec) noexcept{
			this->x = math::fma(vec.x, mulVec.x, x);
			this->y = math::fma(vec.y, mulVec.y, y);
			this->z = math::fma(vec.z, mulVec.z, z);
			return *this;
		}

		[[nodiscard]] FORCE_INLINE auto length() const noexcept{
			return vector3::length(x, y, z);
		}

		[[nodiscard]] FORCE_INLINE constexpr T length2() const noexcept{
			return vector3::length2(x, y, z);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool equals(const_pass_t vector) const noexcept{
			return math::equal(x, vector.x) && math::equal(y, vector.y) && math::equal(z, vector.z);
		}


		[[nodiscard]] FORCE_INLINE auto dst(const_pass_t vector) const noexcept{
			return vector3::dst(x, y, z, vector.x, vector.y, vector.z);
		}

		[[nodiscard]] FORCE_INLINE auto dst(const T tx, const T ty, const T tz) const noexcept{
			return vector3::dst(x, y, z, tx, ty, tz);
		}

		[[nodiscard]] FORCE_INLINE constexpr T dst2(const_pass_t vector) const noexcept{
			return vector3::dst2(x, y, z, vector.x, vector.y, vector.z);
		}

		[[nodiscard]] FORCE_INLINE constexpr T dst2(const T tx, const T ty, const T tz) const noexcept{
			return vector3::dst(x, y, z, tx, ty, tz);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool within(const_pass_t v, const T dst) const noexcept{
			return this->dst2(v) < dst * dst;
		}

		FORCE_INLINE vector3& normalize() noexcept{
			const auto len2 = this->length2();
			if(len2 == static_cast<T>(0) || len2 == static_cast<T>(1)) return *this;
			return this->scl(1.0f / std::sqrt(static_cast<float>(len2)));
		}

		[[nodiscard]] FORCE_INLINE constexpr T dot(const_pass_t vector) const noexcept{
			return x * vector.x + y * vector.y + z * vector.z;
		}

		[[nodiscard]] FORCE_INLINE auto angle_rad(const_pass_t vector) const noexcept{
			float l = length();
			float l2 = vector.length();
			return std::acos(this->dot(x / l, y / l, z / l, vector.x / l2, vector.y / l2, vector.z / l2));
		}

		[[nodiscard]] FORCE_INLINE constexpr float angle(const_pass_t vector) const noexcept{
			return this->angle_rad(vector) * math::rad_to_deg_v<floating_point_t>;
		}

		[[nodiscard]] FORCE_INLINE T constexpr dot(const T x, const T y, const T z) const noexcept{
			return this->x * x + this->y * y + this->z * z;
		}

		[[nodiscard]] FORCE_INLINE vector3 cross(const_pass_t vector) const noexcept{
			return {y * vector.z - z * vector.y, z * vector.x - x * vector.z, x * vector.y - y * vector.x};
		}

		// /**
		//  * Left-multiplies the vector by the given 4x3 column major matrix. The matrix should be composed by a 3x3 matrix representing
		//  * rotation and scale plus a 1x3 matrix representing the translation.
		//  * @param matrix The matrix
		//  * @return This vector for chaining
		//  */
		// Vector3D& mul4x3(float[] matrix){
		//     return set(x * matrix[0] + y * matrix[3] + z * matrix[6] + matrix[9], x * matrix[1] + y * matrix[4] + z * matrix[7]
		//     + matrix[10], x * matrix[2] + y * matrix[5] + z * matrix[8] + matrix[11]);
		// }
		//
		// /**
		//  * Left-multiplies the vector by the given matrix.
		//  * @param matrix The matrix
		//  * @return This vector for chaining
		//  */
		// Vector3D& mul(Mat matrix){
		//     const float[] l_mat = matrix.val;
		//     return set(x * l_mat[Mat.M00] + y * l_mat[Mat.M01] + z * l_mat[Mat.M02], x * l_mat[Mat.M10] + y
		//     * l_mat[Mat.M11] + z * l_mat[Mat.M12], x * l_mat[Mat.M20] + y * l_mat[Mat.M21] + z * l_mat[Mat.M22]);
		// }

		// /**
		//  * Multiplies the vector by the transpose of the given matrix.
		//  * @param matrix The matrix
		//  * @return This vector for chaining
		//  */
		// Vector3D& traMul(Mat matrix){
		//     const float[] l_mat = matrix.val;
		//     return set(x * l_mat[Mat.M00] + y * l_mat[Mat.M10] + z * l_mat[Mat.M20], x * l_mat[Mat.M01] + y
		//     * l_mat[Mat.M11] + z * l_mat[Mat.M21], x * l_mat[Mat.M02] + y * l_mat[Mat.M12] + z * l_mat[Mat.M22]);
		// }

		// /**
		//  * Rotates this vector by the given angle in degrees around the given axis.
		//  * @param axis the axis
		//  * @param degrees the angle in degrees
		//  * @return This vector for chaining
		//  */
		// Vector3D& rotate(const Vector3D& axis, float degrees){
		//     tmpMat.setToRotation(axis, degrees);
		//     return this->mul(tmpMat);
		// }

		[[nodiscard]] FORCE_INLINE constexpr bool is_normalized() const noexcept{
			return is_normalized(0.000000001);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_normalized(const T margin) const noexcept{
			return math::abs(length2() - 1) < margin;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_zero() const noexcept{
			return x == 0 && y == 0 && z == 0;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool is_zero(const T margin) const noexcept{
			return length2() < margin;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool colinear(const_pass_t other,
		                                                   const T epsilon = static_cast<T>(
			                                                   std::numeric_limits<T>::epsilon())) const noexcept{
			return this->cross(other).length2() <= epsilon;
		}


		[[nodiscard]] FORCE_INLINE constexpr bool colinear_directional(const_pass_t other,
		                                                               const T epsilon = static_cast<T>(
			                                                               std::numeric_limits<T>::epsilon())) const
			noexcept{
			return this->colinear(other, epsilon) && (this->dot(other) > 0);
		}

		[[nodiscard]] FORCE_INLINE constexpr bool colinear_inv_directional(const_pass_t other,
		                                                                   const T epsilon = static_cast<T>(
			                                                                   std::numeric_limits<T>::epsilon())) const
			noexcept{
			return this->colinear(other, epsilon) && this->dot(other) < 0;
		}

		[[nodiscard]] FORCE_INLINE constexpr bool perpendicular_with(const_pass_t vector,
		                                                             const T epsilon = static_cast<T>(
			                                                             std::numeric_limits<T>::epsilon())) const noexcept{
			return math::zero(this->dot(vector), epsilon);
		}

		FORCE_INLINE vector3& lerp(const_pass_t target, const std::floating_point auto alpha) noexcept{
			x = math::fma(target.x - x, alpha, x);
			y = math::fma(target.y - y, alpha, y);
			z = math::fma(target.z - z, alpha, z);
			return *this;
		}

		// Vector3D& interpolate(Vector3D& target, float alpha, Interp interpolator){
		//     return lerp(target, interpolator.apply(0f, 1f, alpha));
		// }

		/**
		 * Spherically interpolates between this vector and the target vector by alpha which is in the range [0,1]. The result is
		 * stored in this vector.
		 * @param target The target vector
		 * @param alpha The interpolation coefficient
		 * @return This vector for chaining.
		 */
		//TODO apply acos table...
		FORCE_INLINE vector3& slerp(const_pass_t target, const std::floating_point auto alpha) noexcept{
			const float dot = dot(target);
			// If the inputs are too close for comfort, simply linearly interpolate.
			if(dot > 0.9995 || dot < -0.9995) return this->lerp(target, alpha);

			// theta0 = angle between input vectors
			const auto theta0 = std::acos(dot);
			// theta = angle between this vector and result
			const auto theta = theta0 * alpha;

			const auto st = math::sin(theta);
			const auto tx = target.x - x * dot;
			const auto ty = target.y - y * dot;
			const auto tz = target.z - z * dot;
			const auto l2 = tx * tx + ty * ty + tz * tz;
			const auto dl = st * (l2 < 0.0001 ? 1.0 : 1.0 / std::sqrt(l2));

			return this->scl(math::cos(theta)).add(tx * dl, ty * dl, tz * dl).normalize();
		}

		friend std::ostream& operator<<(std::ostream& os, const_pass_t obj){
			return os
				<< '(' << std::to_string(obj.x)
				<< ", " << std::to_string(obj.y)
				<< ", " << std::to_string(obj.z)
				<< ')';
		}


		FORCE_INLINE vector3& limit(const T limit) noexcept{
			return this->limit2(limit * limit);
		}


		FORCE_INLINE vector3& limit2(const T limit2) noexcept{
			auto len2 = length2();
			if(len2 > limit2){
				this->scl(std::sqrt(limit2 / len2));
			}
			return this;
		}


		FORCE_INLINE vector3& set_length(const T len) noexcept{
			return this->set_length2(len * len);
		}


		FORCE_INLINE vector3& set_length2(const T len2) noexcept{
			const T oldLen2 = this->length2();
			return (oldLen2 == 0 || oldLen2 == len2) ? *this : this->scl(std::sqrt(len2 / oldLen2));
		}


		FORCE_INLINE vector3& clamp(const T min, const T max) noexcept{
			const T len2 = len2();
			if(len2 == 0) return *this;
			const T max2 = max * max;
			if(len2 > max2) return this->scl(std::sqrt(max2 / len2));
			const T min2 = min * min;
			if(len2 < min2) return this->scl(std::sqrt(min2 / len2));
			return this;
		}

		// public int hashCode(){
		//     const int prime = 31;
		//     int result = 1;
		//     result = prime * result + Float.floatToIntBits(x);
		//     result = prime * result + Float.floatToIntBits(y);
		//     result = prime * result + Float.floatToIntBits(z);
		//     return result;


		FORCE_INLINE vector3& set_zero() noexcept{
			this->x = 0;
			this->y = 0;
			this->z = 0;
			return this;
		}

		FORCE_INLINE constexpr vector3& operator+=(const_pass_t rhs) noexcept{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator-=(const_pass_t rhs) noexcept{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
			return *this;
		}

		// 逐分量与标量复合赋值运算
		FORCE_INLINE constexpr vector3& operator*=(const_pass_t rhs) noexcept{
			x *= rhs.x;
			y *= rhs.y;
			z *= rhs.z;
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator*=(T scalar) noexcept{
			x *= scalar;
			y *= scalar;
			z *= scalar;
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator/=(const_pass_t rhs) noexcept{
			x /= rhs.x;
			y /= rhs.y;
			z /= rhs.z;
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator/=(const T scalar) noexcept{
			x /= scalar;
			y /= scalar;
			z /= scalar;
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator%=(const_pass_t rhs) noexcept{
			x = math::mod(x, rhs.x);
			y = math::mod(y, rhs.y);
			z = math::mod(z, rhs.z);
			return *this;
		}

		FORCE_INLINE constexpr vector3& operator%=(T scalar) noexcept{
			x = math::mod(x, scalar);
			y = math::mod(y, scalar);
			z = math::mod(z, scalar);
			return *this;
		}

		// 友元声明运算符重载
		FORCE_INLINE constexpr friend vector3 operator+(const_pass_t lhs, const_pass_t rhs) noexcept{
			return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
		}

		FORCE_INLINE constexpr friend vector3 operator-(const_pass_t lhs, const_pass_t rhs) noexcept{
			return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
		}

		// 逐分量乘法（向量和标量）
		FORCE_INLINE constexpr friend vector3 operator*(const_pass_t lhs, const_pass_t rhs) noexcept{
			return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
		}

		FORCE_INLINE constexpr friend vector3 operator*(const_pass_t lhs, T scalar) noexcept{
			return {lhs.x * scalar, lhs.y * scalar, lhs.z * scalar};
		}

		FORCE_INLINE constexpr friend vector3 operator*(T scalar, const_pass_t rhs) noexcept{
			return rhs * scalar;
		}

		// 逐分量除法（向量和标量）
		FORCE_INLINE constexpr friend vector3 operator/(const_pass_t lhs, const_pass_t rhs) noexcept{
			return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
		}

		FORCE_INLINE constexpr friend vector3 operator/(const_pass_t lhs, T scalar) noexcept{
			return {lhs.x / scalar, lhs.y / scalar, lhs.z / scalar};
		}

		// 逐分量取模（向量和标量）
		FORCE_INLINE constexpr friend vector3 operator%(const_pass_t lhs, const_pass_t rhs) noexcept{
			return lhs.copy() %= rhs;
		}

		FORCE_INLINE constexpr friend vector3 operator%(const_pass_t lhs, T scalar) noexcept{
			return lhs.copy() %= scalar;
		}

		template <typename Ty>
			requires( std::constructible_from<Ty, T, T>)
		FORCE_INLINE constexpr explicit operator Ty() const noexcept{
			return Ty{x, y};
		}

		constexpr friend bool operator==(const vector3& lhs, const vector3& rhs) noexcept = default;
	};

	using Vec3 = vector3<float>;
	using Point3 = vector3<float>;
	using Point3U = vector3<unsigned int>;
	using Point3S = vector3<short>;
	using Point3US = vector3<unsigned short>;

	constexpr Vec3 ZERO3{0, 0, 0};
	constexpr Vec3 X3{1, 0, 0};
	constexpr Vec3 Y3{0, 1, 0};
	constexpr Vec3 Z3{0, 0, 1};
}
