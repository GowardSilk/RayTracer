#define MT
	#define ENABLE_ASYNC

//#define PAR_RENDER_WRITE

//#define KERNEL
//	#define ENABLE_KERNEL_GPU
	//#define ENABLE_KERNEL_CPU

#include "Adrenaline.hpp"
#include "Sphere.hpp"
#include <array>

using namespace raytracer;

auto main() -> int {

	// Image setup
	raytracer::STATS_DESCRIPTOR sdesc = {};
	sdesc.aspect_ratio = 16.0 / 9.0;
	sdesc.image_width = 400;
	sdesc.image_height = static_cast<int>(sdesc.image_width / sdesc.aspect_ratio);
	sdesc.samples_per_pixel = 100;
	sdesc.max_depth = 50;
	sdesc.measurements.iteration_count = 10;
	sdesc.measurements.elapsed = std::vector<double>(sdesc.measurements.iteration_count);

	// World setup
	hittable_list world;

	auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
	auto material_center = std::make_shared<lambertian>(color(0.7, 0.3, 0.3));
	auto material_left	 = std::make_shared<metal>(color(0.8, 0.8, 0.8));
	auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2));

	world.add(std::make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0, material_ground));
	world.add(std::make_shared<sphere>(point3(0.0, 0.0, -1.0), 0.5, material_center));
	world.add(std::make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
	world.add(std::make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

	// Camera setup
	raytracer::CAM_DESCRIPTOR camd;
	camera1 cam(camd);

	// Rendering
	raytracer::ADRENALINE_DESCRIPTOR adesc;
	adesc.cam = cam;
	adesc.world = world;
	adesc.foutput = "output.ppm";

	adrenaline adr(adesc, sdesc);
#ifdef KERNEL
	adr.initOpenCL();
#else
	#ifndef PAR_RENDER_WRITE
		color* img_buff = new color[sdesc.img_size()];
		std::fill(img_buff, img_buff + sdesc.img_size(), color{ 0, 0, 0 });
		adr.render_w_future(&img_buff);
		adr.write_img_buff(&img_buff);
		delete[] img_buff;
	#else
		adr.render();
	#endif
#endif

	std::cerr << "MEASUREMENT:\n";
	for (double m : sdesc.measurements.elapsed)
		std::cerr << "time: ...\t" << m << "ms" << std::endl;

}