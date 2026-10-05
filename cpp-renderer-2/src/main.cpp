#include <iostream>
#include <memory>
#include <vector>
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include "rtweekend.h"

void render(const HittableList& world, const Camera& camera) {
    const int image_width = 400;
    const int image_height = 200;
    const int samples_per_pixel = 100;

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            Vec3 pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                double u = double(i + random_double()) / (image_width - 1);
                double v = double(j + random_double()) / (image_height - 1);
                Ray r = camera.get_ray(u, v);
                pixel_color += ray_color(r, world);
            }
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }
}

int main() {
    // Initialize the scene
    HittableList world;
    auto material_ground = std::make_shared<Lambertian>(Vec3(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<Lambertian>(Vec3(0.7, 0.3, 0.3));
    auto material_left = std::make_shared<Metal>(Vec3(0.8, 0.8, 0.8), 0.3);
    auto material_right = std::make_shared<Metal>(Vec3(0.8, 0.6, 0.2), 1.0);

    world.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100, material_ground));
    world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5, material_center));
    world.add(std::make_shared<Sphere>(Point3(-1, 0, -1), 0.5, material_left));
    world.add(std::make_shared<Sphere>(Point3(1, 0, -1), 0.5, material_right));

    Camera camera;

    // Render the scene
    render(world, camera);

    return 0;
}