#include <iostream>
#include "hittable.h"
#include "hittable_list.h"
#include "camera.h"
#include "color.h"
#include "ray.h"
#include "sphere.h"
#include "rtweekend.h"

void render() {
    // Set up the scene
    const int image_width = 800;
    const int image_height = 600;
    const int samples_per_pixel = 100;

    // Create a camera
    Camera camera;

    // Create a hittable list
    HittableList world;

    // Add objects to the world
    world.add(make_shared<Sphere>(Point3(0, 0, -1), 0.5));
    world.add(make_shared<Sphere>(Point3(0, -100.5, -1), 100));

    // Render loop
    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            Color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                double u = (i + random_double()) / (image_width - 1);
                double v = (j + random_double()) / (image_height - 1);
                Ray r = camera.get_ray(u, v);
                pixel_color += ray_color(r, world);
            }
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }
}

int main() {
    render();
    return 0;
}