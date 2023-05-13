#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "Utility.hpp"
#include "Ray.hpp"

namespace raytracer {
    namespace v0 {
        class camera {
        public:
            camera() {
				const double aspect_ratio = 16.0 / 9.0;
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

				origin = point3{ 0, 0, 0 };
				horizontal = vec3(viewport_width, 0, 0);
				vertical = vec3(0, viewport_height, 0);
				/*
					lower_left_corner {
						0 - wievport_width,
						0 - wievport_height,
						0 - focal_length
					}
				*/
				lower_left_corner = origin - horizontal / 2 - vertical / 2 - vec3{ 0, 0, focal_length };

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
                return ray(origin, lower_left_corner + u * horizontal + v * vertical - origin);
            }

        private:
            point3 origin;
            point3 lower_left_corner;
            vec3 horizontal;
            vec3 vertical;
        };
    }
}


#endif //!CAMERA_HPP