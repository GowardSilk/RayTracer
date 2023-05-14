#ifndef ADRENALINE_HPP
#define ADRENALINE_HPP

#include "Utility.hpp"
#include "Color.hpp"
#include "Camera.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <execution>

namespace raytracer {

	struct STATS_DESCRIPTOR {

		double aspect_ratio;
		int image_width;
		int image_height;
		int samples_per_pixel;
		int max_depth;

	};

	class stats {
	public:

		stats() = default;

		stats(STATS_DESCRIPTOR& stats) {
			m_descriptor = stats;
		}

		STATS_DESCRIPTOR get_descriptor() const { return m_descriptor; }

	private:
		STATS_DESCRIPTOR m_descriptor;
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

		adrenaline(ADRENALINE_DESCRIPTOR& adesc, STATS_DESCRIPTOR& sdesc) {

			if (adesc.foutput.substr(adesc.foutput.length() - 3) != "ppm")
				throw std::exception("not correct image format!");
			m_outfile = std::ofstream(adesc.foutput, std::ios::out);
			m_adesc = adesc;
			m_stats = stats(sdesc);

		}

#ifdef MT
		void render(color** buff) {

			const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

			std::vector<int> range(sdesc.image_width);
			std::generate(std::begin(range), std::end(range),
				[n = 0]() mutable {
					return n++;
				}
			);

			for (int j = sdesc.image_height; j > 0; j++) {
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
						*buff[i * j] = pixel_color;
					}
				);
			}
		}

		void write_img_buff(color** buff) {

			std::stringstream ss;
			const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();

			m_outfile.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering ??
			ss << "P3\n" << sdesc.image_width << " " << sdesc.image_height << " \n255\n";
			m_outfile.write(ss.str().c_str(), sizeof(ss.str().c_str()));
			ss.clear();

			for (int j = sdesc.image_height; j > 0; j--) {
				for (int i = 0; i < sdesc.image_width; i++) {
					auto r = buff[j * i]->x();
					auto g = buff[j * i]->y();
					auto b = buff[j * i]->z();

					// Divide the color by the number of samples.
					auto scale = 1.0 / sdesc.samples_per_pixel;
					r = std::sqrt(scale * r);
					g = std::sqrt(scale * g);
					b = std::sqrt(scale * b);

					// Write the translated [0,255] value of each color component.
					ss << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
						<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
						<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << '\n';

					m_outfile.write(ss.str().c_str(), sizeof(ss.str().c_str()));

					ss.clear();
				}
			}
		}
#else
	#ifdef PAR_RENDER_WRITE
		void render() {

			const STATS_DESCRIPTOR sdesc = m_stats.get_descriptor();
			std::stringstream ss;

			// Write header information to the output file
			ss << "P3\n" << sdesc.image_width << " " << sdesc.image_height << " " << "\n255\n";
			m_outfile.write(ss.str().c_str(), ss.str().size());
			ss.str(std::string());

			m_outfile.rdbuf()->pubsetbuf(nullptr, 0); // Disable buffering ??

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
	private:
		//member data
		std::ofstream m_outfile;
		stats m_stats;
		ADRENALINE_DESCRIPTOR m_adesc;
		//!member data
	};

	class timer {
	public:
		timer() : m_start(std::chrono::high_resolution_clock::now()) {}

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

}

#endif //!ADRENALINE_HPP