#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

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
#include "color.h"
#include "quad.h"
#include "triangle.h"
#include "mesh.h"
#include "diffuse_light.h"


// ============================================================
// ★ 调试开关：法线着色（Day 14）
//
// 命中后直接输出法线方向映射到 [0,1]，不弹射、不消费随机数。
// 好处是【完全确定性】：同一个几何，两次渲染逐字节相同。
// 这让"同一个立方体的两种拆法"可以精确到字节地比对 ——
// 不需要担心"漫反射弹射把浮点尾数差异放大"的问题。
// ============================================================
static bool g_normal_shading = false;

// ============================================================
// ★ 背景色（Day 16）
//
// 以前背景写死成"蓝白渐变天空"。但 Cornell Box 是【封闭盒子】，
// 盒子正面开口朝向相机 —— 如果背景还是蓝天，
// "天空的光"就会从那个开口漏进盒子里，影子全部变淡，失去意义。
//
// 所以背景色必须能按场景切换。
// ★ 记住：在这个渲染器里，背景色就是"环境光"。
// ============================================================
static Color g_bg_top    = Color(0.5, 0.7, 1.0);
static Color g_bg_bottom = Color(1.0, 1.0, 1.0);


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

       // ★ 法线调试模式：直接返回法线颜色，不弹射
       if (g_normal_shading) {
           return 0.5 * Color(rec.normal.x() + 1.0,
                              rec.normal.y() + 1.0,
                              rec.normal.z() + 1.0);
       }

       Ray   scattered;    // 弹射出去的光线
       Color attenuation;  // 反射率（打几折，是个颜色）
       Color emitted = rec.mat->emitted();//判断这个材质是否自己发光

       
       if (rec.mat->scatter(r, rec, attenuation, scattered))
       {
          return emitted + attenuation * ray_color(scattered, world, depth - 1); 
       }

       
       return emitted;
    }

    // 没命中：返回背景色
    // 默认是蓝白渐变天空；Cornell Box 会把两个颜色都设成纯黑
    Vec3 unit_dir = unit_vector(r.direction());
    double t = 0.5 * (unit_dir.y() + 1.0);

    return (1.0 - t) * g_bg_bottom + t * g_bg_top;
}


// ============================================================
// 计时工具
//
// steady_clock 是【单调递增】的：只会往前走，不会被系统校时影响。
//   system_clock 会被 NTP 同步、夏令时跳变 → 可能测出负数！
// ============================================================
using Clock = std::chrono::steady_clock;

double seconds_since(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

// ============================================================
// 渲染函数：把世界渲染成图像
void render(const Hittable& world, const Camera& camera,
                int width, int height, int samples_per_pixel, int max_depth,
                std::ostream* out) {

    // PPM 文件头 —— 只有在要写文件时才写
    if (out) *out << "P3\n" << width << ' ' << height << "\n255\n";

    for (int j = height - 1; j >= 0; --j) {
        // 进度条只在正式渲染时打，benchmark 模式下会刷屏
        if (out) std::cerr << "\r剩余扫描线: " << (j + 1) << ' ' << std::flush;

        for (int i = 0; i < width; ++i) {

            // 累积器：必须在采样循环【外面】，否则每次采样都被清零
            Color pixel_color(0, 0, 0);

            // 第 3 层循环：同一个像素射 samples_per_pixel 条光线
            // ⚠️ 计数器叫 sample，不叫 s —— 因为下面像素坐标已经用了 s
            for (int sample = 0; sample < samples_per_pixel; ++sample) {
                // 在像素【内部】随机取点 —— 这就是抗锯齿的来源
                double s = (i + random_double()) / width;
                double t = (j + random_double()) / height;

                Ray r = camera.get_ray(s, t);
                pixel_color += ray_color(r, world, max_depth);
            }

            // 平均
            Color color = pixel_color / double(samples_per_pixel);

            // ---------- 写像素 ----------
            int ir = to_byte(color.x());
            int ig = to_byte(color.y());
            int ib = to_byte(color.z());

            if(out) *out << ir << ' ' << ig << ' ' << ib << '\n';
        }
    }

}


// ============================================================
// 性能测试：渲染不同数量的小球，测 BVH 构建和求交的耗时
//
// ★ 前向声明（forward declaration）
//   run_benchmark 用到了 random_scene，但后者定义在本文件【后面】。
//   C++ 是单遍编译的：名字必须【先见过才能用】。
//   这一行等于告诉编译器：“有这么个函数，参数是 int，返回 HittableList，
//   具体定义待会儿在下面”，编译器就先放行了。
// ============================================================
HittableList random_scene(int half);

// ============================================================
// 性能测试函数
void run_benchmark() {
    const int Width = 120, Height = 68, spp = 4, max_depth = 50;
    
    Camera camera(Point3(13, 2, 3), Point3(0, 0, 0), Vec3(0, 1, 0),
     20, 10.0, 0.1 );

     const int halves[] = {5, 11, 22, 45};// 5, 11, 22, 45 → 25, 121, 484, 2025 个小球

     std::ofstream csv("output/bench.csv");
     csv << "n,build_ms,naive_ms,bvh_ms\n";

     // 遍历不同的场景规模
     for(int half : halves) {

        reset_rng();
        HittableList list = random_scene(half);
        int n = (int)list.objects.size();

        //测建树耗时
        auto tb = Clock::now();
        BVH bvh(list);
        double build_ms = seconds_since(tb) * 1000.0;

        // 测求交耗时：三次取最小值，避免偶然的系统干扰
        double best_naive = 1e18;
        for(int rep = 0; rep < 3; ++rep) {
            reset_rng();
            auto t0 = Clock::now();
            render(list, camera, Width, Height, spp, max_depth, nullptr);
            double dt = seconds_since(t0) * 1000.0;
            if(dt < best_naive) best_naive = dt;
        }

        // 测 BVH 求交耗时
        double best_bvh = 1e18;
        for(int rep = 0; rep < 3; ++rep) {
            reset_rng();
            auto t0 = Clock::now();
            render(bvh, camera, Width, Height, spp, max_depth, nullptr);
            double dt = seconds_since(t0) * 1000.0;
            if(dt < best_bvh) best_bvh = dt;
        }


           csv << n << ',' << build_ms << ',' << best_naive 
               << ',' << best_bvh << '\n';

                 //屏幕：给人看
           std::cout << "N=" << n
                     << "  build=" << build_ms << "ms"
                     << "  naive=" << best_naive << "ms"
                     << "  bvh=" << best_bvh << "ms"
                     << "  加速 " << (best_naive / best_bvh) << "x\n";


     }
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
HittableList random_scene(int half) {
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
    for (int a = -half; a < half; ++a) {
        for (int b = -half; b < half; ++b) {

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


// ---------- 场景 C：四边形（Day 13）----------
// 5 个彩色平行四边形摆成一个"开放的盒子"。
// 用途：
//   1. 验证第二个几何图元能正确渲染
//   2. ★ 验证【零厚度包围盒】的 padding —— 加不加 BVH 必须逐像素一致
HittableList quad_scene() {
    HittableList world;

    auto red    = std::make_shared<Lambertian>(Color(0.65, 0.05, 0.05));
    auto green  = std::make_shared<Lambertian>(Color(0.12, 0.45, 0.15));
    auto blue   = std::make_shared<Lambertian>(Color(0.20, 0.20, 1.00));
    auto white  = std::make_shared<Lambertian>(Color(0.73, 0.73, 0.73));
    auto orange = std::make_shared<Lambertian>(Color(1.00, 0.50, 0.00));

    // 左墙
    world.add(std::make_shared<Quad>(Point3(-3, -2, 5),
                                     Vec3(0, 0, -4), Vec3(0, 4, 0), red));
    // 后墙
    world.add(std::make_shared<Quad>(Point3(-2, -2, 0),
                                     Vec3(4, 0, 0), Vec3(0, 4, 0), green));
    // 右墙
    world.add(std::make_shared<Quad>(Point3(3, -2, 1),
                                     Vec3(0, 0, 4), Vec3(0, 4, 0), blue));
    // 天花板
    world.add(std::make_shared<Quad>(Point3(-2, 3, 1),
                                     Vec3(4, 0, 0), Vec3(0, 0, 4), orange));
    // 地板
    world.add(std::make_shared<Quad>(Point3(-2, -3, 5),
                                     Vec3(4, 0, 0), Vec3(0, 0, -4), white));

    return world;
}


// ---------- 场景 D：同一个立方体的两种拆法（Day 14）----------
// as_triangles = false → 每个面 1 个 Quad（6 个面 = 6 个图元）
// as_triangles = true  → 每个面 2 个 Triangle（6 个面 = 12 个图元）
//
// ★ 这两种拆法必须渲染出【一模一样】的画面 —— 这就是今天的核心验收。
//
// ⚠️ 三角形版的绕向必须和四边形一致，否则法线会翻转：
//    面 (Q, u, v) 拆成  (Q, Q+u, Q+v)  和  (Q+u, Q+u+v, Q+v)
//    第二个三角形的边是 v 和 (v-u)，cross(v, v-u) = -cross(v,u) = cross(u,v) ✓
HittableList cube_scene(bool as_triangles) {
    HittableList world;

    auto red   = std::make_shared<Lambertian>(Color(0.70, 0.12, 0.12));
    auto green = std::make_shared<Lambertian>(Color(0.15, 0.55, 0.22));
    auto blue  = std::make_shared<Lambertian>(Color(0.18, 0.30, 0.85));
    auto white = std::make_shared<Lambertian>(Color(0.72, 0.72, 0.72));

    // 把一个面加进世界（四边形版 1 个图元 / 三角形版 2 个图元）
    auto add_face = [&](const Point3& Q, const Vec3& u, const Vec3& v,
                        std::shared_ptr<Material> m) {
        if (!as_triangles) {
            world.add(std::make_shared<Quad>(Q, u, v, m));
        } else {
            world.add(std::make_shared<Triangle>(Q,         Q + u, Q + v, m));
            world.add(std::make_shared<Triangle>(Q + u, Q + u + v, Q + v, m));
        }
    };

    // 边长 2、中心在 (cx, 0, 0) 的立方体：y ∈ [-1,1]，z ∈ [-1,1]
    // 每个面的 u × v 都朝【外】，这是六个面绕向正确与否的关键
    auto add_box = [&](double cx, std::shared_ptr<Material> m) {
        const double h = 1.0;
        add_face(Point3(cx + h, -h, -h), Vec3(0, 2, 0), Vec3(0, 0, 2), m); // +x
        add_face(Point3(cx - h, -h, -h), Vec3(0, 0, 2), Vec3(0, 2, 0), m); // -x
        add_face(Point3(cx - h,  h, -h), Vec3(0, 0, 2), Vec3(2, 0, 0), m); // +y
        add_face(Point3(cx - h, -h, -h), Vec3(2, 0, 0), Vec3(0, 0, 2), m); // -y
        add_face(Point3(cx - h, -h,  h), Vec3(2, 0, 0), Vec3(0, 2, 0), m); // +z
        add_face(Point3(cx - h, -h, -h), Vec3(0, 2, 0), Vec3(2, 0, 0), m); // -z
    };

    add_box(-2.5, red);
    add_box( 0.0, green);
    add_box( 2.5, blue);

    // 地面（两种版本都是 Quad，不参与对比）
    world.add(std::make_shared<Quad>(Point3(-8, -1, 8),
                                     Vec3(16, 0, 0), Vec3(0, 0, -16), white));

    return world;
}


// ---------- 场景 E：球面网格，平面着色 vs 光滑着色（Day 15）----------
// 三个球网格密度不同：越粗糙，flat 和 smooth 的差别越剧烈。
// 但两者的【几何完全相同】—— 只有法线不一样，所以剪影必须一致。
//
// ⚠️ 材质用【金属】而不是哑光：
//    哑光向四面八方散射，亮度对法线几乎不敏感 → flat/smooth 差别看不出来。
//    镜面反射方向 = reflect(d, n)，完全由法线决定 → 法线的折线立刻暴露。
HittableList sphere_mesh_scene(bool smooth) {
    HittableList world;

    auto red   = std::make_shared<Metal>(Color(0.75, 0.22, 0.22), 0.03);
    auto green = std::make_shared<Metal>(Color(0.22, 0.68, 0.32), 0.03);
    auto blue  = std::make_shared<Metal>(Color(0.28, 0.42, 0.90), 0.03);
    auto white = std::make_shared<Lambertian>(Color(0.72, 0.72, 0.72));

    // 地面
    world.add(std::make_shared<Quad>(Point3(-8, -1, 8),
                                     Vec3(16, 0, 0), Vec3(0, 0, -16), white));

    const double xs[3]        = { -2.6, 0.0, 2.6 };
    const int    rings[3]     = { 6,   12,  24  };
    const int    segments[3]  = { 10,  20,  40  };
    std::shared_ptr<Material> mats[3] = { red, green, blue };

    for (int k = 0; k < 3; ++k) {
        add_sphere_mesh(world, Point3(xs[k], 0, 0), 1.0,
                        rings[k], segments[k], mats[k], smooth);
    }

    return world;
}


// ---------- 场景 F：Cornell Box（Day 16）----------
// 图形学里最著名的测试场景：封闭盒子，一面墙绿一面墙红，天花板上开一个方形光口。
// 它能同时测试：颜色渗透（color bleeding）、软阴影、间接光。
//
// 尺寸：x ∈ [0, 987]，y ∈ [0, 555]，z ∈ [0, 555]
//   为什么 x 取 987 而不是 555？因为画面是 16:9，
//   盒子开口的宽高比必须和画面一致，否则要么裁掉上下、要么左右露黑边。
//   987 / 555 = 1.778 = 16 / 9 ✓
//
// 开口在 z = 0 一侧朝向相机。
// ⚠️ 相机在盒子外面，所以背景【必须是纯黑】——否则天光会从开口漏进来。
HittableList cornell_box() {
    HittableList world;

    auto red   = std::make_shared<Lambertian>(Color(0.65, 0.05, 0.05));
    auto white = std::make_shared<Lambertian>(Color(0.73, 0.73, 0.73));
    auto green = std::make_shared<Lambertian>(Color(0.12, 0.45, 0.15));
    auto light = std::make_shared<DiffuseLight>(Color(15.0, 15.0, 15.0));

    const double W = 987.0;
    const double H = 555.0;

    // 左墙（红） 法线 +x
    // ⚠️ 相机在 z = -800 朝 +z 看，所以"世界 +x 方向"出现在【画面左侧】：
    //    面向 +z、头顶 +y 时，你的右手指向 -x。
    //    按 RTIOW 原版，红在左、绿在右。
    world.add(std::make_shared<Quad>(Point3(0, 0, 0),
                                     Vec3(0, H, 0), Vec3(0, 0, H), red));
    // 右墙（绿） 法线 -x
    world.add(std::make_shared<Quad>(Point3(W, 0, 0),
                                     Vec3(0, 0, H), Vec3(0, H, 0), green));
    // 地板（白） 法线 +y
    world.add(std::make_shared<Quad>(Point3(0, 0, 0),
                                     Vec3(0, 0, H), Vec3(W, 0, 0), white));
    // 天花板（白） 法线 -y
    world.add(std::make_shared<Quad>(Point3(W, H, H),
                                     Vec3(-W, 0, 0), Vec3(0, 0, -H), white));
    // 后墙（白） 法线 -z
    world.add(std::make_shared<Quad>(Point3(0, 0, H),
                                     Vec3(0, H, 0), Vec3(W, 0, 0), white));

    // 天花板上的方形光源 —— 法线朝下（-y）
    world.add(std::make_shared<Quad>(Point3(W / 2 + 130, H - 1, 370),
                                     Vec3(-260, 0, 0), Vec3(0, 0, -180), light));

    // 两个盒子（和 RTIOW 的 Cornell Box 摆位一致）
    auto add_box = [&](double x0, double x1, double y0, double y1,
                       double z0, double z1, std::shared_ptr<Material> m) {
        const double sx = x1 - x0, sy = y1 - y0, sz = z1 - z0;
        world.add(std::make_shared<Quad>(Point3(x1, y0, z0), Vec3(0, sy, 0), Vec3(0, 0, sz), m)); // +x
        world.add(std::make_shared<Quad>(Point3(x0, y0, z0), Vec3(0, 0, sz), Vec3(0, sy, 0), m)); // -x
        world.add(std::make_shared<Quad>(Point3(x0, y1, z0), Vec3(0, 0, sz), Vec3(sx, 0, 0), m)); // +y
        world.add(std::make_shared<Quad>(Point3(x0, y0, z0), Vec3(sx, 0, 0), Vec3(0, 0, sz), m)); // -y
        world.add(std::make_shared<Quad>(Point3(x0, y0, z1), Vec3(sx, 0, 0), Vec3(0, sy, 0), m)); // +z
        world.add(std::make_shared<Quad>(Point3(x0, y0, z0), Vec3(0, sy, 0), Vec3(sx, 0, 0), m)); // -z
    };

    add_box(330, 590,   0, 330, 180, 440, white);   // 高盒子
    add_box(650, 880,   0, 190, 300, 540, white);   // 矮盒子

    return world;
}


int main(int argc, char** argv) {
#ifdef _WIN32
    // 让 Windows 控制台按 UTF-8 解释输出，避免中文日志变成乱码
    SetConsoleOutputCP(CP_UTF8);
#endif

    // ---------- 打印命令行参数 ----------

    int half = 11; // 默认随机场景的半径
    bool bench = false; // 是否只跑性能测试（不输出图片）
    bool use_bvh = true;    // 是否用 BVH（--no-bvh 关闭，用于两步验收）
    int  samples_per_pixel = 32;   // 每像素采样数（--spp 覆盖）

    // 场景名：random / quads / cube-quads / cube-tris
    std::string scene_name = "random";

    // 解析命令行参数
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--grid" && i + 1 < argc) {
            half = std::stoi(argv[i + 1]);
            ++i;
        }
        // 解析 --bench 参数
        else if(arg == "--bench") {
            bench = true;
        }
        else if(arg == "--quads") {
            scene_name = "quads";
        }
        else if(arg == "--cube-quads") {
            scene_name = "cube-quads";
        }
        else if(arg == "--cube-tris") {
            scene_name = "cube-tris";
        }
        else if(arg == "--sphere-flat") {
            scene_name = "sphere-flat";
        }
        else if(arg == "--sphere-smooth") {
            scene_name = "sphere-smooth";
        }
        else if(arg == "--cornell") {
            scene_name = "cornell";
        }
        else if(arg == "--normal") {
            g_normal_shading = true;   // 法线着色（确定性，用于几何比对）
        }
        else if(arg == "--no-bvh") {
            use_bvh = false;
        }
        else if(arg == "--spp" && i + 1 < argc) {
            samples_per_pixel = std::stoi(argv[i + 1]);
            ++i;
        }
       
    }
    
    if(bench) {
        run_benchmark();
        return 0;
    }

     std::cerr << "[参数] half = " << half 
               << " → 最多" << (2 * half) * (2 * half) << "个小球\n";

    // ---------- 图像参数 ----------
    // ⚠️ 随机场景有 ~480 个球，比 Day 6 慢约 100 倍！
    //    第一次先用小图验证：image_width = 160, image_height = 90, samples = 8
    //    确认没问题，再改回 400 / 225 / 32 正式渲染
    const int image_width  = 400;
    const int image_height = 225;
    
    // 每个像素采样次数（越大越平滑，但越慢）
    // —— 由 --spp 覆盖，默认 32

    // 一条光线最多弹射多少次（防止递归无限深入 → 栈溢出）
    const int max_depth = 50;

    // ---------- 打开输出文件（P3 文本 PPM，便于肉眼检查） ----------
    std::ofstream out("output/image.ppm");
    if (!out) {
        std::cerr << "无法创建 output/image.ppm，请确认 output/ 目录存在\n";
        return 1;
    }

    // ---------- 场景：一个装着所有物体的"世界" ----------
    // 想换场景只改这一行：simple_scene() / random_scene() / quad_scene() / cube_scene()
    HittableList world_list;
    if      (scene_name == "quads")         world_list = quad_scene();
    else if (scene_name == "cube-quads")    world_list = cube_scene(false);
    else if (scene_name == "cube-tris")     world_list = cube_scene(true);
    else if (scene_name == "sphere-flat")   world_list = sphere_mesh_scene(false);
    else if (scene_name == "sphere-smooth") world_list = sphere_mesh_scene(true);
    else if (scene_name == "cornell")       world_list = cornell_box();
    else                                    world_list = random_scene(half);

    // Cornell Box 是封闭盒子，背景必须纯黑（否则天光从开口漏进来）
    if (scene_name == "cornell") {
        g_bg_top    = Color(0, 0, 0);
        g_bg_bottom = Color(0, 0, 0);
    }

    // ⚠️ BVH 的构造函数是【按值传参】，它拿到的是 world_list 的副本，
    //    所以建完树之后 world_list 仍然完整可用 —— 这正是"两步验收"的前提。
    BVH world(world_list);                 // ← 扁平化 BVH（Day 10）

    // ★ 两步验收开关：
    //   --no-bvh  直接用线性表遍历（无加速结构）
    //   默认      用 BVH
    //   两者必须渲染出【逐像素相同】的画面 —— 否则加速结构就是错了。
    const Hittable& scene = use_bvh ? static_cast<const Hittable&>(world)
                                    : static_cast<const Hittable&>(world_list);

    // ---------- 验证包围盒 ----------
    {
        AABB world_box;
        if (scene.bounding_box(world_box)) {
            std::cerr << "[AABB] 整个世界的包围盒\n"
                      << "   最小角 = " << world_box.minimum << '\n'
                      << "   最大角 = " << world_box.maximum << "\n\n";
        } else {
            std::cerr << "[AABB] 整个世界的包围盒：算不出来（有物体没实现 bounding_box）\n\n";
        }
    }

    Camera camera = (scene_name == "quads")
        ? Camera(Point3(0, 0, 9), Point3(0, 0, 0), Vec3(0, 1, 0), 80, 1.0, 0.0)
        : (scene_name == "cube-quads" || scene_name == "cube-tris")
        ? Camera(Point3(0, 4, 11), Point3(0, 0, 0), Vec3(0, 1, 0), 32, 12.0, 0.0)
        : (scene_name == "sphere-flat" || scene_name == "sphere-smooth")
        ? Camera(Point3(0, 3.5, 11), Point3(0, 0, 0), Vec3(0, 1, 0), 32, 12.0, 0.0)
        : (scene_name == "cornell")
        ? Camera(Point3(493.5, 277.5, -800), Point3(493.5, 277.5, 0),
                 Vec3(0, 1, 0), 38.3, 800.0, 0.0)
        : Camera(Point3(13, 2, 3),      // 站远一点、高一点
                 Point3(0, 0, 0),       // 看向原点
                 Vec3(0, 1, 0),         // 世界上方向
                 20,                    // 垂直视野角 20°（长焦）
                 10.0,                  // 对焦距离（随机场景里物体分布很广，取中间值）
                 0.1);                  // 光圈半径



    render(scene, camera, image_width, image_height, samples_per_pixel, max_depth, &out);
    // PPM 行序从上到下；渲染时习惯从下往上遍历
    /*
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
    }*/

    std::cerr << "\n渲染完成 -> output/image.ppm\n";
    return 0;
}




