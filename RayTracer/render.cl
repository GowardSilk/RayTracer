// -----------RANDOM-----------
// Define the Mersenne Twister parameters
#define MT_N 624
#define MT_M 397
#define MT_MATRIX_A 0x9908b0dfUL
#define MT_UPPER_MASK 0x80000000UL
#define MT_LOWER_MASK 0x7fffffffUL

// Initialize the random number generator with a seed
void initialize_rng(__global unsigned int* state, unsigned int seed, unsigned int index) {
    state[index] = seed;
}

// Generate a random unsigned integer
unsigned int generate_random_uint(__global unsigned int* state, unsigned int index) {
    unsigned int y;

    if (index >= MT_N) {
        for (int i = 0; i < MT_N - MT_M; i++) {
            y = (state[i] & MT_UPPER_MASK) | (state[i + 1] & MT_LOWER_MASK);
            state[i] = state[i + MT_M] ^ (y >> 1) ^ (-(y & 1) & MT_MATRIX_A);
        }

        for (int i = MT_N - MT_M; i < MT_N - 1; i++) {
            y = (state[i] & MT_UPPER_MASK) | (state[i + 1] & MT_LOWER_MASK);
            state[i] = state[i + (MT_M - MT_N)] ^ (y >> 1) ^ (-(y & 1) & MT_MATRIX_A);
        }

        y = (state[MT_N - 1] & MT_UPPER_MASK) | (state[0] & MT_LOWER_MASK);
        state[MT_N - 1] = state[MT_M - 1] ^ (y >> 1) ^ (-(y & 1) & MT_MATRIX_A);

        index = 0;
    }

    y = state[index];
    index++;
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

// Generate a random double in the range [0, 1)
double random_double(__global unsigned int* state, unsigned int index) {
    return generate_random_uint(state, index) * (1.0 / 4294967296.0);
}
//!-----------RANDOM-----------

//-----------VECTOR-----------
typedef double3 point3;
typedef double3 vec3;
typedef int3	color;

vec3 unit_vector(vec3 dir) {
	return dir / length(dir);
}

double dot(__const vec3* u, __const vec3* v) {
	return u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
}
//!-----------VECTOR-----------

//-----------RAY-----------
typedef struct {

	point3 origin;
	vec3 dir;

} ray;
//!-----------RAY-----------

//-----------UTILITY-----------
double clamp(double x, double min, double max) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}
//!-----------UTILITY-----------

//-----------CAM-----------
typedef struct {

	double aspect_ratio;
	double viewport_height;
	double focal_length;
	double viewport_width;

	double3 horizontal;
	double3 vertical;
	point3 origin;
	point3 lower_left_corner;

} CAM_DESCRIPTOR;

ray get_ray(CAM_DESCRIPTOR* cam, double* u, double* v) {
	return (ray) {
		.origin = cam->origin,
		.dir = cam->lower_left_corner
			+ u * cam->horizontal,
			+ v * cam->vertical,
			- cam->origin
	};
}
//!-----------CAM-----------

//-----------HITTABLE-----------
typedef struct {
	point3 p;
	vec3 normal;
	double t;
	bool front_face;
} hit_record;

void set_face_normal(hit_record* rec, const ray* r, const vec3* outward_normal) {
	rec->front_face = dot(r->dir, outward_normal) < 0;
	rec->normal = rec->front_face ? outward_normal : (-1)*outward_normal;
}

bool hittable_list_hit(const ray* r, double t_min, double t_max, hit_record* rec) {
	hit_record temp_rec;
	bool hit_anything = false;
	double closest_so_far = t_max;

	/*
		for(__const ) {
			if() {
				hit_anything = true;
				closest_so_far = temp_rec.t;
				*rec = temp_rec;
			}
		}
	*/
	return hit_anything;
}
//!-----------HITTABLE-----------

//-----------STATS-----------
typedef struct {

	double aspect_ratio;
	unsigned int image_width;
	unsigned int image_height;
	int samples_per_pixel;

} STATS_DESCRIPTOR;
//!-----------STATS-----------

//-----------ADRENALINE-----------
typedef struct {
	
	CAM_DESCRIPTOR cam;
	hittable* world;

} ADRENALINE_DESCRIPTOR;
//!-----------ADRENALINE-----------

__kernel void render(
	__global __write_only image2d_t* buff
	__global __read_only STATS_DESCRIPTOR* sdesc,
	__global __read_only ADRENALINE_DESCRIPTOR* adesc,
	) {
 
    // Get the index of the current element to be processed
    int j = get_global_id(0);

	// Do the operation
	for (int i = 0; i < sdesc->image_width; i++) {
		color pixel_color = { 0, 0, 0 };

		for (int s = 0; s < sdesc->samples_per_pixel; s++) {
			double u = (i + random_double()) / (sdesc->image_width - 1);
			double v = (j + random_double()) / (sdesc->image_height - 1);
			ray r = get_ray(&adesc->cam, &u, &v);
			//set_color(&pixel_color, &ray_color(&r, &adesc->world, &sdesc->max_depth));
			pixel_color = ray_color(r, adesc->world, sdesc->max_depth);
		}

		color clr = pixel_color / sdesc->samples_per_pixel;

		double r = sqrt(clr[0]);
		double g = sqrt(clr[1]);
		double b = sqrt(clr[2]);

		clr = {
			int(256 * clamp(r, 0.0, 0.999));
			int(256 * clamp(g, 0.0, 0.999));
			int(256 * clamp(b, 0.0, 0.999));
		};
		int2 pos = { i, j };
		write_imagef(buff, pos, clr);
	}
}