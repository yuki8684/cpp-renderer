#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "vec3.h"
#include "ray.h"
#include "camera.h"
#include "rtweekend.h"
#include "aabb.h"
#include "hittable.h"
#include "sphere.h"
#include "hittable_list.h"
#include "material.h"
#include "lambertian.h"
#include "metal.h"
#include "dielectric.h"
#include "bvh.h"

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


// ============================================================
// 场景构建：把"造世界"的代码从 main 里搬出来，变成函数
//
// 好处：
//   1. main() 只剩"渲染流程"，一眼能看完整
//   2. 想换场景只改 main 里的一行
//   3. 两个场景可以随时对比
//
// 返回类型 HittableList：按【值】返回。
//   C++ 会做"拷贝省略 / 移动"，实际不会真拷贝整个列表；
//   里面存的 shared_ptr 拷贝成本极低（只是引用计数 +1）。
// ============================================================

// ---------- 场景 A：Day 6 的四个物体（回归测试用）----------
HittableList simple_scene() {
    HittableList world;

    world.add(std::make_shared<Sphere>(Point3(-1, 0, -1), 0.5,
        std::make_shared<Dielectric>(1.5)));                     // 左：玻璃球

    world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5,
        std::make_shared<Lambertian>(Color(0.9, 0.3, 0.3))));   // 中：红色哑光球

    world.add(std::make_shared<Sphere>(Point3(1, 0, -1), 0.5,
        std::make_shared<Metal>(Color(0.8, 0.8, 0.8), 0.0)));   // 右：银色镜面球

    world.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100,
        std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0))));   // 地面

    return world;
}


// ---------- 场景 B：随机小球阵（Day 7 要做的）----------
HittableList random_scene() {
    HittableList world;

    // ---------- 1. 地面 ----------
    //    半径 1000 的巨球，球心在 y = -1000 → 球顶恰好落在 y = 0
    //    这样"半个球"看起来就是无限大的平地（曲率小到看不出来）
    world.add(std::make_shared<Sphere>(Point3(0, -1000, 0), 1000,
        std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5))));

    // ---------- 2. 三颗"主角"球（都在 z = 0 这条线上）----------
    world.add(std::make_shared<Sphere>(Point3(0, 1, 0), 1.0,
        std::make_shared<Dielectric>(1.5)));                      // 中间：玻璃

    world.add(std::make_shared<Sphere>(Point3(-4, 1, 0), 1.0,
        std::make_shared<Lambertian>(Color(0.4, 0.2, 0.1))));     // 左边：深棕哑光

    world.add(std::make_shared<Sphere>(Point3(4, 1, 0), 1.0,
        std::make_shared<Metal>(Color(0.7, 0.6, 0.5), 0.0)));     // 右边：金属

    // ---------- 3. 22 × 22 随机小球阵 ----------
    //    a、b 各从 -11 到 10 → 横纵各 22 个格子，最多 484 个小球
    for (int a = -11; a < 11; ++a) {
        for (int b = -11; b < 11; ++b) {

            // TODO(你填 R1)：随机选材质
            //   用 random_double() 取一个 [0, 1) 的随机数
            //   后面按它落在哪个区间来决定材质
            double choose_mat = random_double();


            // TODO(你填 R2)：小球球心
            //   x = a + 0.9 * random_double()   ← 格内随机抖动，避免排成整齐方阵
            //   y = 0.2                          ← 小球半径 0.2，刚好贴着地面
            //   z = b + 0.9 * random_double()
            Point3 center(a + 0.9 * random_double(), 
                          0.2, 
                          b + 0.9 * random_double());


            // 太靠近主角球的小球就跳过（否则会插进大球里）
            //   0.9 是算出来的：大球（球心 y=1、半径 1）在高度 y=0.2 处的
            //   水平半径 = 根号(1 − 0.8²) = 0.6；再加上小球半径 0.2 → 0.8；
            //   留点余量取 0.9
            if ((center - Point3(-4, 0.2, 0)).length() < 0.9) continue;
            if ((center - Point3( 0, 0.2, 0)).length() < 0.9) continue;
            if ((center - Point3( 4, 0.2, 0)).length() < 0.9) continue;


            // TODO(你填 R3)：按 choose_mat 分三种情况，造材质 + 加进 world
            //
            //   情况一  choose_mat < 0.80   → 哑光球
            //      颜色：两个随机颜色【逐元素相乘】
            //        Color albedo = Color(random_double(), random_double(), random_double())
            //                     * Color(random_double(), random_double(), random_double());
            //      （为什么相乘？两数都在 0~1，相乘后偏向暗色，
            //        能得到灰、褐、暗红这类自然色，不会全是荧光色）
            //      然后 world.add(std::make_shared<Sphere>(center, 0.2,
            //                       std::make_shared<Lambertian>(albedo)));
            //
            //   情况二  choose_mat < 0.95   → 金属球
            //      颜色：三个通道都在 0.5~1.0 之间（金属要亮一点），用 random_double(0.5, 1.0)
            //      fuzz：random_double(0, 0.5)（粗糙度，让金属不那么"镜子"）
            //      半径用 0.2
            //
            //   情况三  否则               → 玻璃球
            //      Dielectric(1.5)，半径 0.2
            //
            //   ⚠️ 三种情况的小球半径都是 0.2
            if (choose_mat < 0.80) {

                // 你的代码写在这里
                Color albedo = Color(random_double(), random_double(), random_double())
                             * Color(random_double(), random_double(), random_double());

                world.add(std::make_shared<Sphere>(center, 0.2,
                std::make_shared<Lambertian>(albedo)));

            } else if (choose_mat < 0.95) {

                // 你的代码写在这里
                Color albedo = Color(random_double(0.5, 1.0), random_double(0.5, 1.0), random_double(0.5, 1.0));
                double fuzz = random_double(0, 0.5);

                world.add(std::make_shared<Sphere>(center, 0.2,
                         std::make_shared<Metal>(albedo, fuzz)));

            } else {

                // 你的代码写在这里
                world.add(std::make_shared<Sphere>(center, 0.2,
                         std::make_shared<Dielectric>(1.5)));

            }
        }
    }

    return world;
}



int main() {
#ifdef _WIN32
    // 让 Windows 控制台按 UTF-8 解释输出，避免中文日志变成乱码
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---------- 图像参数 ----------
    // ⚠️ 随机场景有 ~480 个球，比 Day 6 慢约 100 倍！
    //    第一次先用小图验证：image_width = 160, image_height = 90, samples = 8
    //    确认没问题，再改回 400 / 225 / 32 正式渲染
    const int image_width  = 400;
    const int image_height = 225;
    
    // 每个像素采样次数（越大越平滑，但越慢）
    const int samples_per_pixel = 32;

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
    // 想换场景只改这一行：simple_scene() 或 random_scene()
    HittableList world_list = random_scene();
    BVHNode world(world_list);

    // ---------- 验证 Sphere / HittableList 的包围盒 ----------
    {
        AABB world_box;
        if (world.bounding_box(world_box)) {
            std::cerr << "[AABB] 整个世界的包围盒\n"
                      << "   最小角 = " << world_box.minimum << '\n'
                      << "   最大角 = " << world_box.maximum << "\n\n";
        } else {
            std::cerr << "[AABB] 整个世界的包围盒：算不出来（有物体没实现 bounding_box）\n\n";
        }
    }

    Camera camera(Point3(13, 2, 3),      // 站远一点、高一点
                  Point3(0, 0, 0),       // 看向原点
                  Vec3(0, 1, 0),         // 世界上方向
                  20,                    // 垂直视野角 20°（长焦）
                  10.0,                  // 对焦距离（随机场景里物体分布很广，取中间值）
                  0.1);                  // 光圈半径

    // PPM 行序从上到下；渲染时习惯从下往上遍历
    for (int j = image_height - 1; j >= 0; --j) {
        std::cerr << "\r剩余扫描线: " << (j + 1) << ' ' << std::flush;

        for (int i = 0; i < image_width; ++i) {

            // 累积器：必须在采样循环【外面】，否则每次采样都被清零
            Color pixel_color(0, 0, 0);

            // 第 3 层循环：同一个像素射 samples_per_pixel 条光线
            // ⚠️ 计数器叫 sample，不叫 s —— 因为下面像素坐标已经用了 s
            for (int sample = 0; sample < samples_per_pixel; ++sample) {
                // 在像素【内部】随机取点 —— 这就是抗锯齿的来源
                double s = (i + random_double()) / image_width;
                double t = (j + random_double()) / image_height;

                Ray r = camera.get_ray(s, t);
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




