#include "Color.hpp"
#include "Ray.hpp"
#include "Sphere.hpp"
#include "Utility.hpp"
#include "Camera.hpp"
#include <algorithm>
#include <array>
#include <execution>

using namespace raytracer::v0;

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
		point3 target = rec.p + rec.normal + random_in_hemisphere(rec.normal);
		return 0.5 * ray_color(ray{ rec.p, target - rec.p }, world, depth - 1);
	}
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color{ 1.0, 1.0, 1.0 }
			+ t * color{ 0.5, 0.7, 1.0 };
}

auto main() -> int {

	// Image setup
	const auto aspect_ratio = 16.0 / 9.0;
	const int image_width = 400;
	const int image_height = static_cast<int>(image_width / aspect_ratio);
	const int samples_per_pixel = 100;
	const int max_depth = 50;

	// World setup
	hittable_list world;
	//world.add(std::make_shared<sphere>(point3{ 0, 0, -1 }, 0.5));
	//world.add(std::make_shared<sphere>(point3{ 0, -100.5, -1 }, 100));

	// Camera setup
	camera cam;

	// Rendering
	std::cout << "P3\n" << image_width << " " << image_height << " " << "\n255\n";

//#define MT
#ifdef MT
	std::array<int, 400> clr_vec;
	std::generate(std::begin(clr_vec), std::end(clr_vec), 
		[n = 0.0] () mutable {
			return n++;
		}
	);
	color* img_buff = new color[image_height * image_width];
#endif

	for (int j = image_height; j > 0; j--) {
		std::cerr << "\rScanlines remaining: " << j << " " << std::flush;
#ifdef MT
		std::for_each(
			std::execution::par,
			std::begin(clr_vec),
			std::end(clr_vec),
			[&] (int& i) {
				color pixel_color(0, 0, 0);
				for (int s = 0; s < samples_per_pixel; ++s) {
					auto u = (i + random_double()) / (image_width - 1);
					auto v = (j + random_double()) / (image_height - 1);
					ray r = cam.get_ray(u, v);
					pixel_color += ray_color(r, world, max_depth);
				}
				img_buff[i * j] = pixel_color;
			}
		);
#else
		for (int i = 0; i < image_width; i++) {
			color pixel_color(0, 0, 0);
			for (int s = 0; s < samples_per_pixel; ++s) {
				auto u = (i + random_double()) / (image_width - 1);
				auto v = (j + random_double()) / (image_height - 1);
				ray r = cam.get_ray(u, v);
				pixel_color += ray_color(r, world, max_depth);
			}
			write_color(std::cout, pixel_color, samples_per_pixel);
		}
#endif
	}
#ifdef MT
	for (int j = image_height; j > 0; j--) {
		for (int i = 0; i < image_width; i++) {
			write_color(std::cout, img_buff[i * j], samples_per_pixel);
		}
	}
	delete[] img_buff;
#endif
	std::cerr << "\nDone.\n";

	//std::cin.get();
}