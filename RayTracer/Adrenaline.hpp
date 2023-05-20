#ifndef ADRENALINE_HPP
#define ADRENALINE_HPP

using UINT = unsigned int;

#include "Utility.hpp"
#include "Color.hpp"
#include "Camera.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <execution>
#include <functional>

#ifdef KERNEL
	#include <CL/cl2.hpp>
#else
	#ifdef ENABLE_OMP
		#include <omp.h>
	#endif
	#ifdef ENABLE_THREAD
		#include <thread>
	#endif
	#ifdef ENABLE_ASYNC
		#include <future>
	#endif
#endif

namespace raytracer {

	class timer {
	public:
		void reset() {
			m_start = std::chrono::high_resolution_clock::now();
		}

		double elapsed() const {
			auto end = std::chrono::high_resolution_clock::now();
			return std::chrono::duration<double, std::milli>(end - m_start).count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
	};

	struct MEASUREMENT_HEAP {
		int iteration_count;
		std::vector<double> elapsed;
	};

	struct STATS_DESCRIPTOR {

		double aspect_ratio;
		UINT image_width;
		UINT image_height;
		int samples_per_pixel;
		int max_depth;
		MEASUREMENT_HEAP measurements;

		[[nodiscard]]
		inline UINT img_size() const { return image_height * image_width; }

	};

	class stats {
	public:

		stats() = default;

		stats(STATS_DESCRIPTOR& stats) 
			: m_descriptor(stats) {}

		// prevent from binding an rvalue
		stats(STATS_DESCRIPTOR&& stats) = delete;

	public:

		[[noreturn]]
		void measure(std::function<void()> mfunc) {
			for (int i = 0; i < m_descriptor.measurements.iteration_count; i++) {
				t.reset();
				mfunc();
				m_descriptor.measurements.elapsed[i] = t.elapsed();
			}
		}

		STATS_DESCRIPTOR get_descriptor() const { return m_descriptor; }

	private:
		STATS_DESCRIPTOR& m_descriptor;
		timer t;
	};

	struct ADRENALINE_DESCRIPTOR {

		camera1 cam;
		hittable_list world;
		std::string foutput;

		ADRENALINE_DESCRIPTOR& operator=(const ADRENALINE_DESCRIPTOR& adesc) {
			cam = adesc.cam;
			world = adesc.world;
			foutput = adesc.foutput;
			return *this;
		}

	};

	class adrenaline {
	public:

		adrenaline(ADRENALINE_DESCRIPTOR& adesc, STATS_DESCRIPTOR& sdesc)
			: m_outfile(adesc.foutput, std::ios::out),
			m_adesc(adesc), m_stats(sdesc)
		{
			if (adesc.foutput.substr(adesc.foutput.length() - 3) != "ppm")
				throw std::exception("not correct image format!");
		}

	public:

#ifdef KERNEL

#ifdef ENABLE_KERNEL_CPU
#define CL_DEVICE_T CL_DEVICE_TYPE_CPU
#elif defined(ENABLE_KERNEL_GPU)
#define CL_DEVICE_T CL_DEVICE_TYPE_GPU
#endif

	public:

		// InitOpenCL
		void initOpenCL() {
			// Initialize OpenCL platform, devices, and context
			std::vector<cl::Platform> platforms;
			cl::Platform::get(&platforms);

			// Select main platform:
			cl::Platform platform = platforms[0];

			// Search for available devices on selected platform:
			std::vector<cl::Device> devices;
			platform.getDevices(CL_DEVICE_T, &devices);

			// Select main device:
			cl::Device device = devices[0];

			cl::Context context(device);

			// Read the contents of the .cl file
			std::ifstream file("render.cl");
			std::string sourceCode(std::istreambuf_iterator<char>(file), {});

			// Create program:
			cl::Program program(context, sourceCode);
			if (program.build() != CL_SUCCESS)
				throw std::exception("error building");

			// Create Kernel:
			cl::Kernel kernel(program, "render");

			STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();
			// Create an image buffer for the output image
			cl::Image2D imageBuffer(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_RGB, CL_UNSIGNED_INT32),
				sdesc.image_width, sdesc.image_height);

			// Create a buffer for the image descriptor
			cl::Buffer sdescBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(STATS_DESCRIPTOR), &sdesc);

			// Create a buffer for the adrenaline descriptor
			cl::Buffer adescBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(ADRENALINE_DESCRIPTOR), &m_adesc);

			// Set kernel arguments
			kernel.setArg(0, imageBuffer);
			kernel.setArg(1, sdescBuffer);
			kernel.setArg(2, adescBuffer);

			// Define the global and local work sizes
			cl::NDRange globalSize(sdesc.image_width, sdesc.image_height);
			cl::NDRange localSize(1, 1);

			cl::CommandQueue queue(context, device);
			cl::Event event;
			queue.enqueueNDRangeKernel(kernel, cl::NullRange, globalSize, localSize, nullptr, &event);

			// Wait for the kernel execution to complete
			event.wait();

			// Check for any errors during kernel execution
			cl_int status = event.getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();
			if (status != CL_COMPLETE) {
				std::cerr << "Kernel execution failed with status: " << status << std::endl;
				// Handle the error appropriately
				return;
			}

			// Read the output image data
			std::vector<unsigned char> imageData(sdesc.image_width * sdesc.image_height * 3);
			cl::array<cl::size_type, 3> origin{}, region{};
			origin[0] = origin[1] = origin[2] = 0;
			region[0] = sdesc.image_width;
			region[1] = sdesc.image_height;
			region[2] = 1;
			cl::Event readEvent;
			queue.enqueueReadImage(imageBuffer, CL_TRUE, origin, region, 0, 0, imageData.data(), nullptr, &readEvent);

			// Wait for the read operation to complete
			readEvent.wait();

			// Check for any errors during read operation
			status = readEvent.getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();
			if (status != CL_COMPLETE) {
				std::cerr << "Image read failed with status: " << status << std::endl;
				// Handle the error appropriately
				return;
			}

		}
		//!InitOpenCL
#else
	#if defined(MT)
		#ifdef PAR_RENDER_WRITE
		#else

	#ifdef ENABLE_OMP
			// OpenMP MT rendering
			void render_w_omp(color** buff) {

				m_stats.measure([&]() {
					const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

					#pragma omp parallel for
					for (int j = sdesc.image_height - 1; j >= 0; --j) {
						for (int i = 0; i < sdesc.image_width; ++i) {
							color pixel_color{ 0, 0, 0 };
							for (int s = 0; s < sdesc.samples_per_pixel; ++s) {
								auto u = (i + random_double()) / (sdesc.image_width - 1);
								auto v = (j + random_double()) / (sdesc.image_height - 1);
								ray r = m_adesc.cam.get_ray(u, v);
								pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
							}
							(*buff)[j * sdesc.image_width + i] = pixel_color;
						}
					}

				});
			}
	#endif

			// std::thread MT rendering
	#ifdef ENABLE_THREAD
			void render_w_thread(color** buff) {
				const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

				auto nthreads = std::thread::hardware_concurrency();
				auto blocksz = sdesc.img_size() / nthreads;
				std::vector<std::thread> threads(nthreads);
				// thread function
				const auto exec_block = [&](int hstart, int hend) {
					for (int j = hend - 1; j >= hstart; j--) {
						for (int i = 0; i < sdesc.image_width; ++i) {
							color pixel_color{ 0, 0, 0 };
							for (int s = 0; s < sdesc.samples_per_pixel; ++s) {
								auto u = (i + random_double()) / (sdesc.image_width - 1);
								auto v = (j + random_double()) / (sdesc.image_height - 1);
								ray r = m_adesc.cam.get_ray(u, v);
								pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
							}
							(*buff)[j * sdesc.image_width + i] = pixel_color;
						}
					}
				};
				// setup threads
				for (int i = 0; i < nthreads; i++) {
					int start = (i * blocksz);
					int end = ((i + 1) * blocksz);
					if (i == nthreads - 1)
						end = sdesc.img_size();
					threads[i] = std::thread(exec_block, start, end);
				}

				// wait for all threads to finish
				for (std::thread& thread : threads)
					thread.join();
			}
	#endif

	#ifdef ENABLE_ASYNC
			// Future & async MT rendering
			void render_w_future(color** buff) {

				m_stats.measure([&]() {
					const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

					std::vector<std::future<void>> futures;
					futures.reserve(sdesc.image_height);

					for (int j = sdesc.image_height - 1; j >= 0; --j) {
						futures.emplace_back(
							std::async(std::launch::async, 
							[&, j]() {
								for (int i = 0; i < sdesc.image_width; ++i) {
									color pixel_color{ 0, 0, 0 };
									for (int s = 0; s < sdesc.samples_per_pixel; ++s) {
										auto u = (i + random_double()) / (sdesc.image_width - 1);
										auto v = (j + random_double()) / (sdesc.image_height - 1);
										ray r = m_adesc.cam.get_ray(u, v);
										pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
									}
									(*buff)[j * sdesc.image_width + i] = pixel_color;
								}
							}
							)
						);
					}

					// Wait for all the asynchronous tasks to complete
					for (auto& future : futures) {
						future.wait();
					}
				});
			}

	#endif

			// Default MT rendering
			void render_def(color** buff) {

				const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

				std::vector<int> range(sdesc.image_width);
				std::generate(std::begin(range), std::end(range),
					[n = 0]() mutable {
						return n++;
					}
				);

				for (int j = sdesc.image_height-1; j >= 0; j--) {
					std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
					std::for_each(
						std::execution::par,
						std::begin(range),
						std::end(range),
						[&](int& i) {
							color pixel_color{ 0, 0, 0 };
							for (int s = 0; s < sdesc.samples_per_pixel; s++) {
								auto u = (i + random_double()) / (sdesc.image_width - 1);
								auto v = (j + random_double()) / (sdesc.image_height - 1);
								ray r = m_adesc.cam.get_ray(u, v);
								pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
							}
							(*buff)[j * sdesc.image_width + i] = pixel_color;
						}
					);
				}
			}

			void write_img_buff(color** buff) {

				std::stringstream ss;
				const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

				m_outfile.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering ??
				ss << "P3\n" << sdesc.image_width << " " << sdesc.image_height << " \n255\n";
				m_outfile.write(ss.str().c_str(), ss.str().size());
				ss.str(std::string());

				for (int j = sdesc.image_height-1; j >= 0; j--) {
					for (int i = 0; i < sdesc.image_width; i++) {
						color clr = (*buff)[j * sdesc.image_width + i];
						auto r = clr.x();
						auto g = clr.y();
						auto b = clr.z();

						// Divide the color by the number of samples.
						auto scale = 1.0 / sdesc.samples_per_pixel;
						r = std::sqrt(scale * r);
						g = std::sqrt(scale * g);
						b = std::sqrt(scale * b);

						// Write the translated [0,255] value of each color component.
						ss << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
							<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
							<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';

						m_outfile.write(ss.str().c_str(), ss.str().size());

						ss.str(std::string());
					}
				}
			}
		#endif

	#else // ST (single-thread)
		#ifdef PAR_RENDER_WRITE
			void render() {

				m_stats.measure([&]() {
					const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();
					std::stringstream ss;

					// Write header information to the output file
					ss << "P3\n" << sdesc.image_width << " " << sdesc.image_height << " " << "\n255\n";
					m_outfile.write(ss.str().c_str(), ss.str().size());
					ss.str(std::string());

					m_outfile.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering ??

					for (int j = sdesc.image_height - 1; j >= 0; j--) {
						//std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
						for (int i = 0; i < sdesc.image_width; i++) {
							color pixel_color{ 0, 0, 0 };
							for (int s = 0; s < sdesc.samples_per_pixel; s++) {
								auto u = (i + random_double()) / (sdesc.image_width - 1);
								auto v = (j + random_double()) / (sdesc.image_height - 1);
								ray r = m_adesc.cam.get_ray(u, v);
								pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
							}
							auto clr = pixel_color / sdesc.samples_per_pixel;
							auto r = std::sqrt(clr.x());
							auto g = std::sqrt(clr.y());
							auto b = std::sqrt(clr.z());

							ss << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
								<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
								<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';

							m_outfile.write(ss.str().c_str(), ss.str().size());
							ss.str(std::string());
						}
					}
				});
			}
		#else
			void render(color** buff) {

			const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

			for (int j = sdesc.image_height - 1; j >= 0; j--) {
				std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
				for (int i = 0; i < sdesc.image_width; i++) {
					color pixel_color{ 0, 0, 0 };
					for (int s = 0; s < sdesc.samples_per_pixel; s++) {
						auto u = (i + random_double()) / (sdesc.image_width - 1);
						auto v = (j + random_double()) / (sdesc.image_height - 1);
						ray r = m_adesc.cam.get_ray(u, v);
						pixel_color += ray_color(r, m_adesc.world, sdesc.max_depth);
					}
					(*buff)[j * sdesc.image_width + i] = pixel_color;
				}
			}
		}

		void write_img_buff(color** buff) {

			std::stringstream ss;
			const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

			ss << "P3\n" << sdesc.image_width << " " << sdesc.image_height << " " << "\n255\n";
			m_outfile.write(ss.str().c_str(), ss.str().size());
			ss.str(std::string());

			m_outfile.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering ??

			for (int j = sdesc.image_height - 1; j >= 0; j--) {
				for (int i = 0; i < sdesc.image_width; i++) {
					color clr = (*buff)[j * sdesc.image_width + i];
					auto r = clr.x();
					auto g = clr.y();
					auto b = clr.z();

					// Divide the color by the number of samples.
					auto scale = 1.0 / sdesc.samples_per_pixel;
					r = std::sqrt(scale * r);
					g = std::sqrt(scale * g);
					b = std::sqrt(scale * b);

					// Write the translated [0,255] value of each color component.
					ss << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
						<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
						<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';

					m_outfile.write(ss.str().c_str(), ss.str().size());

					ss.str(std::string());
				}
			}
		}
		#endif

	#endif
#endif


	private:
		//member data
		std::ofstream m_outfile;
		stats m_stats;
		ADRENALINE_DESCRIPTOR m_adesc;
		//!member data
	};
}

#endif //!ADRENALINE_HPP