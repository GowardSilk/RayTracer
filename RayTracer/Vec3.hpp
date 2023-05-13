#ifndef VEC3_HPP
#define VEC3_HPP

#include <cmath>
#include <iostream>

#include "Utility.hpp"

namespace raytracer {
	namespace v0 {
		class vec3 {
		public:
			// Constructors
			vec3() : e{0, 0, 0} {}
			vec3(double ex, double ey, double ez)
				: e{ex, ey, ez} {}
			vec3(const vec3& v)
				: e{v.x(), v.y(), v.z()} {}

			// Functions
			double x() const { return e[0]; }
			double y() const { return e[1]; }
			double z() const { return e[2]; }

			bool near_zero() const {
				// Return true if the vector is close to zero in all dimensions.
				const auto s = 1e-8;
				return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
			}

			static vec3 random() {
				return vec3(random_double(), random_double(), random_double());
			}

			static vec3 random(double min, double max) {
				return vec3(random_double(min, max), random_double(min, max), random_double(min, max));
			}

			// Operators
			//	Math ops
			vec3& operator+=(const vec3& v) {
				e[0] += v.e[0]; // x
				e[1] += v.e[1]; // y
				e[2] += v.e[2]; // z
				return *this;
			}

			vec3& operator*=(const double t) {
				e[0] *= t;
				e[1] *= t;
				e[2] *= t;
				return *this;
			}

			vec3& operator/=(const double t) {
				return *this *= 1 / t;
			}

			double length() const {
				return std::sqrt(length_squared());
			}

			double length_squared() const {
				return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
			}

		private:
			//member data
			double e[3];
			//!member data
		};

		// Type aliases
		using point3 = vec3;
		using color = vec3;

		// vec3 utility functions


		inline std::ostream& operator<<(std::ostream& out, const vec3& v) {
			return out << v.x() << " " << v.y() << " " << v.z();
		}

		inline vec3 operator+(const vec3& u, const vec3& v) {
			return vec3(u.x() + v.x(), u.y() + v.y(), u.z() + v.z());
		}

		inline vec3 operator-(const vec3& u, const vec3& v) {
			return vec3(u.x() - v.x(), u.y() - v.y(), u.z() - v.z());
		}

		inline vec3 operator*(const vec3& u, const vec3& v) {
			return vec3(u.x() * v.x(), u.y() * v.y(), u.z() * v.z());
		}

		inline vec3 operator*(double t, const vec3& v) {
			return vec3(t * v.x(), t * v.y(), t * v.z());
		}

		inline vec3 operator*(const vec3& v, double t) {
			return t * v;
		}

		inline vec3 operator/(vec3 v, double t) {
			return (1 / t) * v;
		}

		inline double dot(const vec3& u, const vec3& v) {
			return u.x() * v.x()
				+ u.y() * v.y()
				+ u.z() * v.z();
		}

		inline vec3 cross(const vec3& u, const vec3& v) {
			return vec3(u.y() * v.z() - u.z() * v.y(),
				u.z() * v.x() - u.x() * v.z(),
				u.x() * v.y() - u.y() * v.x());
		}

		vec3 random_in_unit_sphere() {
			while (true) {
				auto p = vec3::random(-1, 1);
				if (p.length_squared() >= 1) continue;
				return p;
			}
		}

		inline vec3 unit_vector(vec3 v) {
			return v / v.length();
		}

		vec3 random_unit_vector() {
			return unit_vector(random_in_unit_sphere());
		}

		vec3 random_in_hemisphere(const vec3& normal) {
			vec3 in_unit_sphere = random_in_unit_sphere();
			if (dot(in_unit_sphere, normal) > 0.0) // In the same hemisphere as the normal
				return in_unit_sphere;
			else
				return -1 * in_unit_sphere;
		}
	}
}


#endif //!VEC3_HPP