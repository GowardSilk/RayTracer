#ifndef RAY_HPP
#define RAY_HPP

#include "Vec3.hpp"

namespace raytracer {
	class ray {
	public:
		// Constructors
		ray() = default;
		ray(const point3& origin, const vec3& direction)
			: m_origin(origin), m_dir(direction) {}

		// Functions
		point3 origin() const { return m_origin; }
		vec3 direction() const { return m_dir; }

		// Op
		point3 at(double t) const {
			return m_origin + t * m_dir;
		}

	private:
		//member data
		point3 m_origin;
		vec3 m_dir;
		//!member data
	};
}

#endif //!RAY_HPP