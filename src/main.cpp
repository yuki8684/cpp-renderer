#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "vec3.h"
#include "ray.h"
#include "camera.h"
#include "rtweekend.h"
#include "hittable.h"
#include "sphere.h"
#include "hittable_list.h"
#include "material.h"
#include "lambertian.h"
#include "metal.h"
#include "dielectric.h"
// ============================================================
// 光线 -> 颜色
// 必须定义在 main 外面；漫反射 / 金属都需要它递归调用自己
// ============================================================
Color ray_color(const Ray& r, const Hittable& world, int depth) {
    // ★ 保险丝：弹射次数用完就放弃这条光线。
    //   必须有！否则玻璃的"全反射"会让递归无限深入 → 栈溢出崩溃 (0xC00000FD)
    if (depth <= 0) return Color(0, 0, 0);

    HitRecord rec;

    if (world.hit(r, 0.01, infinity, rec)) {
       Ray   scattered;    // 弹射出去的光线
       Color attenuation;  // 反射率（打几折，是个颜色）

       if (rec.mat->scatter(r, rec, attenuation, scattered))
       {
         return attenuation * ray_color(scattered, world, depth - 1);
       }

       return Color(0, 0, 0); // 光被吸收了
    }

    // 没命中：蓝白渐变背景
    Vec3 unit_dir = unit_vector(r.direction());
    double t = 0.5 * (unit_dir.y() + 1.0);

    return (1.0 - t) * Color(1.0, 1.0, 1.0)
         +         t  * Color(0.5, 0.7, 1.0);
}



int main() {
#ifdef _WIN32
    // 让 Windows 控制台按 UTF-8 解释输出，避免中文日志变成乱码
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---------- 图像参数 ----------
    const int image_width  = 400;
    const int image_height = 225;
    
    // 每个像素采样次数（越大越平滑，但越慢）
    const int samples_per_pixel = 8;

    // 一条光线最多弹射多少次（防止递归无限深入 → 栈溢出）
    const int max_depth = 50;

    // ---------- 打开输出文件（P3 文本 PPM，便于肉眼检查） ----------
    std::ofstream out("output/image.ppm");
    if (!out) {
        std::cerr << "无法创建 output/image.ppm，请确认 output/ 目录存在\n";
        return 1;
    }

    out << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    // ---------- 场景：一个装着所有物体的"世界" ----------
    HittableList world;


  

    // --------------------------------------------------------
    // 场景：四个物体（玻璃球 / 红哑光球 / 银金属球 / 地面）
    //   地面用半径 100、球心在 y = -100.5 的巨球，顶面恰好是 y = -0.5
    // --------------------------------------------------------
    world.add(std::make_shared<Sphere>(Point3(-1, 0, -1), 0.5,
        std::make_shared<Dielectric>(1.5)));                     // 左：玻璃球

    world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5,
        std::make_shared<Lambertian>(Color(0.9, 0.3, 0.3))));   // 中：红色哑光球

    world.add(std::make_shared<Sphere>(Point3(1, 0, -1), 0.5,
        std::make_shared<Metal>(Color(0.8, 0.8, 0.8), 0.0)));   // 右：银色镜面球

    world.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100,
        std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0))));   // 地面

    Camera camera;

    // PPM 行序从上到下；渲染时习惯从下往上遍历
    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\r剩余扫描线: " << (j + 1) << ' ' << std::flush;

        for (int i = 0; i < image_width; ++i) {

            // 累积器：必须在采样循环【外面】，否则每次采样都被清零
            Color pixel_color(0, 0, 0);

            // 第 3 层循环：同一个像素射 samples_per_pixel 条光线
            for (int s = 0; s < samples_per_pixel; ++s) {
                // 在像素【内部】随机取点 —— 这就是抗锯齿的来源
                double u = (i + random_double()) / image_width;
                double v = (j + random_double()) / image_height;

                Ray r = camera.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }

            // 平均
            Color color = pixel_color / double(samples_per_pixel);

            // ---------- 写像素 ----------
            int ir = static_cast<int>(255.999 * color.x());
            int ig = static_cast<int>(255.999 * color.y());
            int ib = static_cast<int>(255.999 * color.z());

            out << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

    std::cerr << "\n渲染完成 -> output/image.ppm\n";
    return 0;
}




