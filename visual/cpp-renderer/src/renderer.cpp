#include "renderer.h"
#include "hittable_list.h"
#include "camera.h"
#include "color.h"
#include "visualizer.h"
#include "ray.h"
#include <iostream>

void render_scene(const HittableList& world, const Camera& camera, Visualizer& visualizer, int image_width, int image_height) {
    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            double u = double(i) / (image_width - 1);
            double v = double(j) / (image_height - 1);
            Ray r = camera.get_ray(u, v);
            Color pixel_color = ray_color(r, world);
            visualizer.set_pixel(i, j, pixel_color);
        }
    }
}

Color ray_color(const Ray& r, const HittableList& world, int depth) {
    HitRecord rec;
    if (depth <= 0) {
        return Color(0, 0, 0); // Background color
    }
    if (world.hit(r, 0.001, infinity, rec)) {
        Ray scattered;
        Color attenuation;
        if (rec.material->scatter(r, rec, attenuation, scattered)) {
            return attenuation * ray_color(scattered, world, depth - 1);
        }
        return Color(0, 0, 0); // No scatter
    }
    // Background color
    Vec3 unit_direction = unit_vector(r.direction());
    double t = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
}