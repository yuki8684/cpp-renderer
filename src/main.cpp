#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>      // Day 18：std::thread / hardware_concurrency
#include <atomic>      // Day 18：进度计数
#include <cstdint>     // Day 18：std::uint8_t

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
#include "obj_loader.h"
#include <thread>
#include <cstdint>

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
// ★ 光源登记表（Day 17）
//
// 以前光源靠"随机乱弹碰巧撞上"。NEE 要改成"主动朝光源打"，
// 就必须能【枚举】场景里的光源。直接把 Quad 指针 + 它发的光记下来，
// 采样时不用遍历整个世界。
//
// ⚠️ 指针生命周期：
//    这些 Quad 由 HittableList 里的 shared_ptr 持有；
//    BVH 拷贝的也是同一批 shared_ptr（对象没被复制），
//    所以渲染期间指针一直有效。
// ============================================================
struct LightRef {
    const Quad* geo;      // 几何体（用来在它上面采样）
    Color       emitted;  // 它发出的光
};
static std::vector<LightRef> g_lights;

static bool g_nee = true;   // --no-nee 关闭，用于 A/B 对比

// ★ Day 18：开几个线程。0 = 自动（取硬件线程数）
static int g_threads = 0;


// ============================================================
// ★ NEE：从表面点 p 直接朝光源采样（Day 17）
//
// 返回这一点的【直接光照】贡献，单位已经是"辐射亮度"，
// 可以像 emitted / 间接光一样直接相加。
//
// 为什么需要它？
//   随机弹射撞上小光源的概率极低 → 绝大多数采样返回 0 → 方差巨大。
//   NEE 直接对光源面积做积分，把"碰运气"变成"确定性地找"。
// ============================================================
// 从场景里所有光源中，随机挑一盏，并记下挑中它的概率
static Color estimate_direct_light(const Point3& p, const Vec3& n,
                                   const Color& albedo, const Hittable& world) {
    if (g_lights.empty()) return Color(0, 0, 0);

    // ---------- 1. 均匀挑一个光源 ----------
    int    idx        = int(random_double() * double(g_lights.size()));// idx属于[0,g_lights.size()]，每个光源被选中的概率是 1/g_light.size()
    if (idx >= int(g_lights.size())) idx = int(g_lights.size()) - 1;
    const LightRef& L = g_lights[std::size_t(idx)];// 引用选中光源
    const double pick_pdf = 1.0 / double(g_lights.size());   // 选中它的概率

    // ---------- 2. 在光源上均匀采样一点 ----------
    Point3 lp; 
    Vec3   ln;// 法线 
    double pdf_area = 0.0;// 面积
    L.geo->sample_area(lp, ln, pdf_area);
    if (pdf_area <= 0.0) return Color(0, 0, 0);

    // ---------- 3. 方向与距离 ----------
    Vec3   to_light = lp - p; // p : 光线打到的位置
    double dist2    = dot(to_light, to_light); 
    double dist     = std::sqrt(dist2);//dist和dist2都是在计算to_light的长度，dist2是第一步，dist是结果
    if (dist < 1e-9) return Color(0, 0, 0);
    Vec3 wi = to_light / dist;              // 从 p 指向光源的单位向量

    // ---------- 4. 两个余弦因子 ----------
    // TODO(T1)：表面法线 n 与 wi 的余弦
    //   ⚠️ 算完要判 <= 0：光源跑到表面背后时不该有贡献
    double cos_surf = dot(n, wi);// 地面法线朝上，正对灯
    if (cos_surf <= 0.0) return Color(0, 0, 0);

    // TODO(T2)：光源法线 ln 与"从光源指向 p 的方向"的余弦
    //   提示：从光源指向 p 的单位向量是 -wi
    //   ⚠️ 同样要判 <= 0（背面朝向 p 的光源不发光）
    double cos_light = dot(ln, -wi); // 灯朝下，正对地面
    if (cos_light <= 0.0) return Color(0, 0, 0);

    // ---------- 5. 阴影射线：中间有没有东西挡着？ ----------
    // TODO(T3)：朝 wi 打一条射线，只要在 (0.001, dist - 0.001) 内命中任何东西，
    //           就说明被遮挡，直接返回黑色
    Ray shadow(p, wi);
    HitRecord srec;
    if (world.hit(shadow, 0.001, dist - 0.001, srec)) return Color(0, 0, 0);

    // ---------- 6. 面积 pdf → 立体角 pdf ----------
    //   换元公式：  dω = cos_light / dist² · dA
    //   所以概率密度也要跟着换：  pdf_ω = pdf_A · dist² / cos_light
    //   多光源时还要乘上"选中这个光源的概率"
    // TODO(T4)
    double pdfw = pick_pdf * pdf_area * dist2 / cos_light;

    // ---------- 7. 最终估计量 ----------
    //   Lambertian 的 BRDF： f = albedo / π
    //   估计量 = f · L_e · cos_surf / pdf_ω
    Color f = albedo / pi;
    // TODO(T5)
    return f * L.emitted * cos_surf / pdfw;
}


// ============================================================
// 光线 -> 颜色
//
// ★ Day 17：多了一个 skip_emitted 参数
//   "上一次弹射已经做过 NEE 了，所以这次直接命中光源的亮度要跳过"
//   否则同一个光源会被算两遍（NEE 算一次 + 随机弹射撞上再算一次）。
//
// 必须定义在 main 外面；漫反射 / 金属都需要它递归调用自己
// ============================================================
Color ray_color(const Ray& r, const Hittable& world, int depth, bool skip_emitted) {
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

       // ★ 上一次做过 NEE → 这次直接看到光源的贡献要跳过（避免重复计算）
       Color emitted = skip_emitted ? Color(0, 0, 0) : rec.mat->emitted();

       Ray   scattered;    // 弹射出去的光线
       Color attenuation;  // 反射率（打几折，是个颜色）

       if (!rec.mat->scatter(r, rec, attenuation, scattered)) {
           return emitted;      // 不散射：路径终止（纯光源走这条路）
       }

       // ★ NEE：漫反射表面额外做一次"直接朝光源采样"
       Color direct(0, 0, 0);
       bool  did_nee = false;
       if (g_nee && rec.mat->is_diffuse()) {
           direct  = estimate_direct_light(rec.p, rec.normal,
                                           rec.mat->diffuse_albedo(), world);
           did_nee = true;
       }

       return emitted
            + direct
            + attenuation * ray_color(scattered, world, depth - 1, did_nee);
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
// 渲染函数：把世界渲染成图像（Day 18 改成两阶段并行）
//
// 阶段 1（并行）：每个线程算自己那一段【连续的行】，结果写进 pixels[]
// 阶段 2（串行）：主线程按 PPM 要求的顺序统一写文件
//
//   为什么必须分两阶段？
//   多个线程同时往同一个 ofstream 里写 → 行顺序错乱 → PPM 文件损坏。
//   先算到内存、再统一写出，就不会有这个问题；
//   而且"结果可复现"也变简单了：线程调度再乱，内存布局都是确定的。
// ============================================================
void render(const Hittable& world, const Camera& camera,
                int width, int height, int samples_per_pixel, int max_depth,
                std::ostream* out) {

    // ---------- 决定开几个线程 ----------
    unsigned nthreads = (g_threads > 0)
        ? unsigned(g_threads)
        : std::thread::hardware_concurrency();   // g_threads == 0(默认) : 调std::thread::hardware_concurrency(),返回硬件支持的线程数
    if (nthreads == 0u) nthreads = 1u;           // 查不到就单线程（规范允许返回 0）
    if (nthreads > unsigned(height)) nthreads = unsigned(height); // 如有多出来的线程，销毁开销，钳制到height

    if (out) {
        std::cerr << "\n[线程] " << nthreads
                  << " 个，图像 " << width << 'x' << height
                  << "，每像素 " << samples_per_pixel << " 采样\n";
    }

    // ---------- 阶段 1：并行计算，结果先存内存 ----------
    // 每个像素 3 个字节（RGB）。size_t 换算防止 4K 图时 int 溢出。
    std::vector<std::uint8_t> pixels(std::size_t(width) * std::size_t(height) * 3u, 0);

    // 一个线程要干的活：算第 [row_begin, row_end) 行，tid是线程编号
    auto worker = [&](unsigned tid) {
        const int row_begin = int(std::uint64_t(height) * tid       / nthreads);
        const int row_end   = int(std::uint64_t(height) * (tid + 1) / nthreads);

        for (int j = row_begin; j < row_end; ++j) {

            // ★★ 关键设计：每一行【单独播种】，种子由行号散列得到
            //
            //   为什么不按线程编号播种？因为那样"结果"会依赖于线程数：
            //   开 8 个线程和开 1 个线程，同一行会落在不同线程里，
            //   用了不同的随机序列 → 图像不一样 → 无法验收。
            //
            //   按【行号】播种则得到更强的不变量：
            //   【线程数完全不影响结果】，--threads 1 和 --threads 8 逐字节相同。
            //
            //   2654435761u 是 Knuth 乘法散列常数（黄金比例的 2^32 倍），
            //   把连续的行号打散，避免相邻行拿到相关的随机序列。
            reset_rng(5489u + 2654435761u * unsigned(j));// 种子只有行数j决定，不管哪个线程算，种子都是hash(j)

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
                    pixel_color += ray_color(r, world, max_depth, false);
                }

                // 平均
                Color color = pixel_color / double(samples_per_pixel);

                // 写进内存（不是文件！）pixels是width * height * 3的一维数组，每像素3字节
                const std::size_t k = (std::size_t(j) * std::size_t(width)
                                       + std::size_t(i)) * 3u;
                //例：pixels[6030]:R; pixels[6031]:G; piexels[6032]:B;
                pixels[k + 0] = std::uint8_t(to_byte(color.x()));
                pixels[k + 1] = std::uint8_t(to_byte(color.y()));
                pixels[k + 2] = std::uint8_t(to_byte(color.z()));
            }
        }
    };

    // ---------- 开线程 ----------
    // 进度用原子计数器，只在线程【整块算完】时打一次，避免控制台乱码
    std::atomic<int> blocks_done{0};

    auto worker_with_progress = [&](unsigned tid) {
        worker(tid);// 干活
        if (out) {
            const int d = ++blocks_done;// 原子递增
            std::cerr << "\r[线程] 完成 " << d << '/' << nthreads << ' ' << std::flush;
        }
    };

    std::vector<std::thread> pool;
    if (nthreads > 1u) pool.reserve(nthreads - 1u);

    // t = 1到nthreads - 1：开nthread - 1个线程
    for (unsigned t = 1; t < nthreads; ++t) {
        pool.emplace_back(worker_with_progress, t);
    }
    worker_with_progress(0);          // 主线程也干活，省一个线程的创建开销

    // join():阻塞，等线程结束
    for (auto& th : pool) th.join();   // ★ 必须 join，否则 main 返回时 std::terminate

    if (out) std::cerr << "\n";

    // ---------- 阶段 2：主线程按顺序写文件（串行） ----------
    if (!out) return;                 // nullptr = 只算不写（基准测试用）

    *out << "P3\n" << width << ' ' << height << "\n255\n";
    for (int j = height - 1; j >= 0; --j) {
        for (int i = 0; i < width; ++i) {
            const std::size_t k = (std::size_t(j) * std::size_t(width)
                                   + std::size_t(i)) * 3u;
            *out << int(pixels[k + 0]) << ' '
                 << int(pixels[k + 1]) << ' '
                 << int(pixels[k + 2]) << '\n';
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
    // ⚠️ 实验记录：换成 Lambertian(Color(15,15,15)) 后整张图 mean=0.000 / max=0（纯黑）。
    //    因为"发光"和"反光"是两件事：emitted() 负责注入能量，albedo 只负责打几折。
    //    详见 docs/devlog.md Day 16「实验：把光源换成极亮的哑光白墙」。
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
    // ★ Day 17：同时把它登记进光源表，NEE 才知道该往哪里采样
    auto light_quad = std::make_shared<Quad>(Point3(W / 2 + 130, H - 1, 370),
                                             Vec3(-260, 0, 0), Vec3(0, 0, -180), light);
    world.add(light_quad);
    g_lights.push_back({ light_quad.get(), Color(15.0, 15.0, 15.0) });

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


// ---------- 场景 G：OBJ 模型（Day 20）----------
// 从 models/*.obj 读三角形。
//
// ⚠️ OBJ 的坐标范围不可控（有的模型半径 0.01，有的 1000），
//    所以 center / scale 必须能调，不能写死。
HittableList obj_scene(const std::string& path, const Point3& center, double scale) {
    HittableList world;

    auto white = std::make_shared<Lambertian>(Color(0.72, 0.72, 0.72));
    auto metal = std::make_shared<Metal>(Color(0.78, 0.68, 0.52), 0.05);

    // 地面
    world.add(std::make_shared<Quad>(Point3(-9, -1.8, 9),
                                     Vec3(18, 0, 0), Vec3(0, 0, -18), white));

    // 模型
    HittableList m = obj_mesh(path, center, scale, metal);
    for (const auto& o : m.objects) world.add(o);

    std::cerr << "[OBJ] " << path << " -> " << m.objects.size() << " 个三角形\n";
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

    // 解析命令行参数，argv[0]是程序名
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
        else if(arg == "--obj-ico") {
            scene_name = "obj-ico";
        }
        else if(arg == "--obj-torus") {
            scene_name = "obj-torus";
        }
        else if(arg == "--normal") {
            g_normal_shading = true;   // 法线着色（确定性，用于几何比对）
        }
        else if(arg == "--no-bvh") {
            use_bvh = false;
        }
        else if(arg == "--no-nee") {
            g_nee = false;   // 关闭光源采样，回到纯随机弹射（A/B 对比用）
        }
        else if(arg == "--threads" && i + 1 < argc) {
            g_threads = std::stoi(argv[i + 1]);
            ++i;
        }
        else if(arg == "--uniform-hs") {
            g_uniform_hemisphere = true;   // Day 19 对照：均匀半球采样（“差”的那种）
        }
        else if(arg == "--spp" && i + 1 < argc) {
            samples_per_pixel = std::stoi(argv[i + 1]);
            ++i;
        }
       
    }
    
    //--bench时只跑性能测试，不渲染图像，直接返回
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
    g_lights.clear();          // 光源表按场景重建
    HittableList world_list;
    if      (scene_name == "quads")         world_list = quad_scene();
    else if (scene_name == "cube-quads")    world_list = cube_scene(false);
    else if (scene_name == "cube-tris")     world_list = cube_scene(true);
    else if (scene_name == "sphere-flat")   world_list = sphere_mesh_scene(false);
    else if (scene_name == "sphere-smooth") world_list = sphere_mesh_scene(true);
    else if (scene_name == "cornell")       world_list = cornell_box();
    else if (scene_name == "obj-ico")
        world_list = obj_scene("models/icosahedron.obj", Point3(0, -0.03, 0), 1.76);
    else if (scene_name == "obj-torus")
        world_list = obj_scene("models/torus.obj",       Point3(0, -1.24, 0), 1.55);
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
        : (scene_name == "obj-ico")
        ? Camera(Point3(0, 1.9, 6.3), Point3(0, -0.45, 0), Vec3(0, 1, 0), 37, 6.5, 0.0)
        : (scene_name == "obj-torus")
        ? Camera(Point3(0, 2.6, 5.6), Point3(0, -1.05, 0), Vec3(0, 1, 0), 34, 5.8, 0.0)
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




