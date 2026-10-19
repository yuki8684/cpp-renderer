// ============================================================
// 光滑着色（顶点法线插值）单元测试 —— Day 15
//
// 编译运行:
//   g++ -std=c++17 -O2 -Wall -Wextra -I include -o build/smooth_test.exe tests/smooth_test.cpp
//   .\build\smooth_test.exe
//
// 四组：
//   A  顶点处      插值法线 == 该顶点的法线
//   B  重心处      插值法线 == normalize(n0 + n1 + n2)
//   C  平面着色    仍然是面法线（不能因为加了新构造函数就坏掉）
//   D  球面网格    ★ 定量：插值法线 vs 解析球面法线的夹角误差
//                  同时证明 smooth 和 flat 命中的是【同一批点】（几何不变）
// ============================================================

#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "ray.h"
#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "lambertian.h"
#include "quad.h"
#include "triangle.h"
#include "mesh.h"

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* name, bool got, bool want) {
    if (got == want) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << (got ? "true" : "false")
                  << "  期望=" << (want ? "true" : "false") << '\n';
    }
}

static void check_vec(const char* name, const Vec3& got, const Vec3& want,
                      double eps = 1e-12) {
    if (std::fabs(got.x() - want.x()) <= eps &&
        std::fabs(got.y() - want.y()) <= eps &&
        std::fabs(got.z() - want.z()) <= eps) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=(" << got.x() << ", " << got.y() << ", " << got.z() << ")"
                  << "  期望=(" << want.x() << ", " << want.y() << ", " << want.z() << ")\n";
    }
}

// 两个向量的夹角（度）
static double angle_deg(const Vec3& a, const Vec3& b) {
    double d = dot(unit_vector(a), unit_vector(b));
    if (d >  1.0) d =  1.0;
    if (d < -1.0) d = -1.0;
    return std::acos(d) * 180.0 / pi;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 光滑着色单元测试 =====\n";

    auto mat = std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));

    // 三角形几何和 Day 14 一样：a=(1,0,0) b=(1,1,0) c=(0,1,0)，平面 z = 0
    // 射线从 +z 打向 -z，所以顶点法线的 z 分量必须为正，否则 set_face_normal 会翻面
    const Vec3 n0 = unit_vector(Vec3(0.30, 0.00, 1.0));
    const Vec3 n1 = unit_vector(Vec3(0.00, 0.30, 1.0));
    const Vec3 n2 = unit_vector(Vec3(0.00, 0.00, 1.0));

    Triangle smooth_tri(Point3(1, 0, 0), Point3(1, 1, 0), Point3(0, 1, 0),
                        n0, n1, n2, mat);

    // 从正上方朝 -z 打一条射线
    auto shoot = [&](Triangle& tri, double x, double y, HitRecord& rec) {
        return tri.hit(Ray(Point3(x, y, 5), Vec3(0, 0, -1)), 0.001, infinity, rec);
    };

    // ========================================================
    // 组 A：三个顶点处，插值法线应该【精确等于】那个顶点的法线
    // ========================================================
    {
        HitRecord rec;

        bool h = shoot(smooth_tri, 1.0, 0.0, rec);          // α=0, β=0  → 权重全给 n0
        check("A 顶点 a 命中", h, true);
        check_vec("A 顶点 a 的法线 == n0", rec.normal, n0);

        h = shoot(smooth_tri, 1.0, 1.0, rec);               // α=1, β=0  → n1
        check("A 顶点 b 命中", h, true);
        check_vec("A 顶点 b 的法线 == n1", rec.normal, n1);

        h = shoot(smooth_tri, 0.0, 1.0, rec);               // α=0, β=1  → n2
        check("A 顶点 c 命中", h, true);
        check_vec("A 顶点 c 的法线 == n2", rec.normal, n2);
    }

    // ========================================================
    // 组 B：重心处，(α,β) = (1/3, 1/3)
    //        三个权重都是 1/3 → n = (n0 + n1 + n2) / 3，归一化
    // ========================================================
    {
        HitRecord rec;
        bool h = shoot(smooth_tri, 2.0 / 3.0, 2.0 / 3.0, rec);
        check("B 重心命中", h, true);

        Vec3 want = unit_vector(n0 + n1 + n2);
        check_vec("B 重心处的法线 == normalize(n0+n1+n2)", rec.normal, want, 1e-12);

        // 顺便验证"插值会缩短长度"：不归一化时长度明显小于 1
        Vec3 raw = (1.0 / 3.0) * n0 + (1.0 / 3.0) * n1 + (1.0 / 3.0) * n2;
        double len = std::sqrt(dot(raw, raw));
        std::cout << "        (不归一化时长度 = " << len << " ，比 1 短 "
                  << (1.0 - len) * 100.0 << "%，所以必须 unit_vector)\n";
        check("B 归一化后长度 == 1", std::fabs(std::sqrt(dot(rec.normal, rec.normal)) - 1.0) < 1e-12, true);
    }

    // ========================================================
    // 组 C：平面着色仍然是面法线（新构造函数不能破坏老行为）
    // ========================================================
    {
        Triangle flat_tri(Point3(1, 0, 0), Point3(1, 1, 0), Point3(0, 1, 0), mat);

        HitRecord rec;
        bool h = shoot(flat_tri, 2.0 / 3.0, 2.0 / 3.0, rec);
        check("C 平面着色命中", h, true);
        check_vec("C 平面着色法线 == 面法线 (0,0,1)", rec.normal, Vec3(0, 0, 1));
    }

    // ========================================================
    // 组 D：★ 球面网格 —— 定量比较"插值法线 vs 解析真法线"
    //
    //   球心在原点、半径 1 时，真法线就是 normalize(交点位置)。
    //   我们沿 -z 打一批平行射线，统计最大夹角误差。
    //
    //   预期：flat 误差大（约半个三角形张角），smooth 误差很小。
    //   并且两者的 t 必须完全一致 —— 证明只改了法线，没改几何。
    // ========================================================
    {
        const int rings = 12, segments = 24;

        HittableList smooth_ball, flat_ball;
        add_sphere_mesh(smooth_ball, Point3(0, 0, 0), 1.0, rings, segments, mat, true);
        add_sphere_mesh(flat_ball,   Point3(0, 0, 0), 1.0, rings, segments, mat, false);

        double max_smooth = 0.0, max_flat = 0.0, max_t_diff = 0.0;
        int hits = 0;

        for (int i = 0; i < 25; ++i) {
            for (int j = 0; j < 25; ++j) {
                double x = -0.9 + 0.075 * i;
                double y = -0.9 + 0.075 * j;
                if (x * x + y * y > 0.81) continue;   // 只取球面正面区域

                Ray r(Point3(x, y, 5), Vec3(0, 0, -1));

                HitRecord rs, rf;
                if (!smooth_ball.hit(r, 0.001, infinity, rs)) continue;
                if (!flat_ball.hit(r, 0.001, infinity, rf)) continue;
                ++hits;

                max_smooth = std::fmax(max_smooth, angle_deg(rs.normal, unit_vector(rs.p)));
                max_flat   = std::fmax(max_flat,   angle_deg(rf.normal, unit_vector(rf.p)));
                max_t_diff = std::fmax(max_t_diff, std::fabs(rs.t - rf.t));
            }
        }

        std::cout << "        (共 " << hits << " 条命中；法线最大夹角误差：smooth "
                  << max_smooth << "° ，flat " << max_flat << "°)\n";

        check("D 有足够多的命中点", hits > 300, true);
        check("D 光滑着色法线误差 < 1°", max_smooth < 1.0, true);
        check("D 平面着色法线误差 > 3°", max_flat > 3.0, true);
        check("D 光滑比平面更接近真法线", max_smooth < max_flat, true);
        // ★ 几何没变：命中距离必须完全一致
        check("D 两种着色的命中距离完全相同（几何不变）", max_t_diff < 1e-12, true);
    }

    std::cout << "\n  === 结果：" << g_pass << " 通过, " << g_fail << " 失败 ===\n\n";
    return g_fail == 0 ? 0 : 1;
}
