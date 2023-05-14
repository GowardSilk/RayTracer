#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Utility.hpp"
#include "Ray.hpp"

namespace raytracer {

	class camera {
	};

	struct CAM_DESCRIPTOR {
		double aspect_ratio = 16.0 / 9.0;
		/*
			For the standard square pixel spacing,
			the viewport's aspect ratio should be the same as our rendered image.
			We'll just pick a viewport two units in height.
		*/
		double viewport_height = 2.0;
		/*
			We'll also set the distance between the projection plane and the projection point
			to be one unit.
		*/
		double focal_length = 1.0;
		double viewport_width = aspect_ratio * viewport_height;

		vec3 horizontal = vec3{ viewport_width, 0.0, 0.0 };
		vec3 vertical = vec3{ 0.0, viewport_height, 0.0 };
		point3 origin = point3{ 0.0, 0.0, 0.0 };
		point3 lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3{ 0.0, 0.0, focal_length };
	};

    class camera1 : virtual camera {
    public:

		camera1() = default;

        camera1(CAM_DESCRIPTOR camd) {
			m_camd = camd;
			//const double aspect_ratio = 16.0 / 9.0;
			//double viewport_height = 2.0;
			//double focal_length = 1.0;
			//double viewport_width = aspect_ratio * viewport_height;

			//origin = point3{ 0, 0, 0 };
			//horizontal = vec3(viewport_width, 0, 0);
			//vertical = vec3(0, viewport_height, 0);
			//lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3{ 0, 0, focal_length };
        }

        ray get_ray(double u, double v) const {
            return ray(
				m_camd.origin,
				m_camd.lower_left_corner
					+ u * m_camd.horizontal
					+ v * m_camd.vertical
					- m_camd.origin
			);
        }

    private:
		CAM_DESCRIPTOR m_camd;
    };
}


#endif //!CAMERA_HPP