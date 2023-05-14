#ifndef COLOR_HPP
#define COLOR_HPP

#include "Vec3.hpp"
#include "Utility.hpp"
#include "Ray.hpp"
#include "Hittable.hpp"
#include "Material.hpp"

#include <iostream>

namespace raytracer {
    void write_color(std::ostream& out, color pixel_color) {
        out << static_cast<int>(255.999 * pixel_color.x()) << ' '
            << static_cast<int>(255.999 * pixel_color.y()) << ' '
            << static_cast<int>(255.999 * pixel_color.z()) << '\n';
    }

    void write_color(std::ostream& out, color pixel_color, int samples_per_pixel) {
        auto r = pixel_color.x();
        auto g = pixel_color.y();
        auto b = pixel_color.z();

        // Divide the color by the number of samples.
        auto scale = 1.0 / samples_per_pixel;
        r = std::sqrt(scale * r);
        g = std::sqrt(scale * g);
        b = std::sqrt(scale * b);

        // Write the translated [0,255] value of each color component.
        out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
            << static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
            << static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';
    }
    /*
        The ray_color(ray) function linearly blends white and blue depending on the height of the y
        coordinate after scaling the ray direction to unit length (so −1.0<y<1.0).
        Because we're looking at the y height after normalizing the vector, you'll notice a horizontal gradient to the color in addition to the vertical gradient.
    */
    color ray_color(const ray& r, const hittable& world, int depth) {
        hit_record rec;

        if (depth <= 0)
            return color{ 0, 0, 0 };

        if (world.hit(r, 0, infinity, rec)) {
            ray scattered;
            color attenuation;
            if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
                return attenuation * ray_color(scattered, world, depth - 1);
            return color{ 0, 0, 0 };
        }
        vec3 unit_direction = unit_vector(r.direction());
        auto t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * color { 1.0, 1.0, 1.0 }
        + t * color{ 0.5, 0.7, 1.0 };
    }
}

#endif //!COLOR_HPP