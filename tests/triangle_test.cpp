// ============================================================
// Triangle 单元测试（独立可执行文件）
//
// 编译运行（在项目根目录）:
//   g++ -std=c++17 -O2 -Wall -Wextra -I include -o build/triangle_test.exe tests/triangle_test.cpp
//   .\build\triangle_test.exe
//
// 或者用脚本:  .\tools\test.ps1
//
// 五组测试，每组【目的不同】：
//   A  直角三角形，且顶点 a 【故意不在原点】  → 抓 "传顶点当边向量" 的 bug
//   B  朝向与法线                            → 抓 绕向写反
//   C  退化保护 + 尺度                          → 抓 epsilon 没做归一化
//   D  包围盒                                 → 证明 "三角形用平行四边形的盒子"
//   E  同一立方体面，两种拆法                     → 今天的核心验收
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

static void check_near(const char* name, double got, double want, double eps = 1e-9) {
    if (std::fabs(got - want) <= eps) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << got << "  期望=" << want << '\n';
    }
}

static bool box_contains(const AABB& b, const Point3& p) {
    return b.minimum.x() <= p.x() && p.x() <= b.maximum.x()
        && b.minimum.y() <= p.y() && p.y() <= b.maximum.y()
        && b.minimum.z() <= p.z() && p.z() <= b.maximum.z();
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== Triangle 单元测试 =====\n";

    auto mat = std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));

    // ========================================================
    // 组 A：直角三角形，顶点 a 【故意不在原点】
    //
    //   a = (1,0,0)      ← 关键：不在原点
    //   b = (1,1,0)
    //   c = (0,1,0)
    //
    //   u = b - a = ( 0, 1, 0)
    //   v = c - a = (-1, 1, 0)
    //   n = u × v = ( 0, 0, 1)          → normal = (0,0,1)，D = 0，w = (0,0,1)
    //
    //   内部点的 (α,β)：
    //     a           → (0,   0  )     α+β = 0
    //     b           → (1,   0  )     α+β = 1
    //     c           → (0,   1  )     α+β = 1
    //     重心        → (1/3, 1/3)
    //     ab 中点     → (0.5, 0  )
    //     ac 中点     → (0,   0.5)
    //     bc 中点     → (0.5, 0.5)     α+β = 1
    //     平行四边形第四角 (0,2,0) → (1,1)  α+β = 2  ★ 三角形不该命中
    // ========================================================
    {
        Triangle tri(Point3(1, 0, 0), Point3(1, 1, 0), Point3(0, 1, 0), mat);

        // 小工具：从正上方（+z）朝 -z 打一条射线
        auto shoot = [&](double x, double y, HitRecord& rec) {
            return tri.hit(Ray(Point3(x, y, 5), Vec3(0, 0, -1)), 0.001, infinity, rec);
        };

        HitRecord rec;

        // ---------- 重心 ----------
        // a + (1/3)u + (1/3)v = (1,0,0) + (0,1/3,0) + (-1/3,1/3,0) = (2/3, 2/3, 0)
        bool h = shoot(2.0 / 3.0, 2.0 / 3.0, rec);
        check("A 重心命中", h, true);
        if (h) check_near("A t = 5", rec.t, 5.0);

        // ---------- 三个顶点（闭集，都是合法命中）----------
        check("A 顶点 a 命中 (α=0,β=0)", shoot(1.0, 0.0, rec), true);
        check("A 顶点 b 命中 (α=1,β=0)", shoot(1.0, 1.0, rec), true);
        check("A 顶点 c 命中 (α=0,β=1)", shoot(0.0, 1.0, rec), true);

        // ---------- 三条边的中点 ----------
        check("A 边 ab 中点命中 (α=0.5,β=0)", shoot(1.0, 0.5, rec), true);
        check("A 边 ac 中点命中 (α=0,β=0.5)", shoot(0.5, 0.5, rec), true);
        check("A 斜边 bc 中点命中 (α+β=1)", shoot(0.5, 1.0, rec), true);

        // ---------- ★ 关键：平行四边形第四角 ----------
        // Q + u + v = (1,0,0) + (0,1,0) + (-1,1,0) = (0,2,0)，对应 (α,β) = (1,1)
        // 同一个 Quad 会命中这里，三角形必须拒绝 —— 这是两者的分界线
        check("A 平行四边形第四角 (α=1,β=1) 不命中", shoot(0.0, 2.0, rec), false);

        // ---------- 外部 ----------
        check("A 外部 (α=2) 不命中", shoot(1.5, 1.5, rec), false);

        // ---------- 平行 / 背后 / t 区间 ----------
        check("A 光线平行于平面不命中",
              tri.hit(Ray(Point3(0.5, 0.5, 5), Vec3(1, 0, 0)), 0.001, infinity, rec), false);
        check("A t_min 挡住",
              tri.hit(Ray(Point3(2.0 / 3.0, 2.0 / 3.0, 5), Vec3(0, 0, -1)), 6.0, infinity, rec), false);
        check("A t_max 挡住",
              tri.hit(Ray(Point3(2.0 / 3.0, 2.0 / 3.0, 5), Vec3(0, 0, -1)), 0.001, 4.0, rec), false);
    }

    // ========================================================
    // 组 B：朝向与法线
    // ========================================================
    {
        Triangle tri(Point3(1, 0, 0), Point3(1, 1, 0), Point3(0, 1, 0), mat);

        HitRecord rec;
        bool h = tri.hit(Ray(Point3(2.0 / 3.0, 2.0 / 3.0, 5), Vec3(0, 0, -1)),
                         0.001, infinity, rec);
        check("B 正面命中", h, true);
        check("B 正面 front_face = true", h && rec.front_face, true);
        check("B 正面法线 z = +1", h && std::fabs(rec.normal.z() - 1.0) < 1e-12, true);

        HitRecord rec2;
        bool h2 = tri.hit(Ray(Point3(2.0 / 3.0, 2.0 / 3.0, -5), Vec3(0, 0, 1)),
                          0.001, infinity, rec2);
        check("B 背面命中", h2, true);
        check("B 背面 front_face = false", h2 && !rec2.front_face, true);
        check("B 背面法线被翻成 z = -1", h2 && std::fabs(rec2.normal.z() + 1.0) < 1e-12, true);
    }

    // ========================================================
    // 组 C：退化保护 + 尺度不变性
    // ========================================================
    {
        // ---------- 三点共线：u ∥ v ----------
        Triangle flat(Point3(0, 0, 0), Point3(1, 0, 0), Point3(2, 0, 0), mat);
        HitRecord rec;
        check("C 共线三点(退化)打不中",
              flat.hit(Ray(Point3(0.5, 0, 5), Vec3(0, 0, -1)), 0.001, infinity, rec), false);

        // ---------- 零向量边：a == b ----------
        Triangle zero(Point3(0, 0, 0), Point3(0, 0, 0), Point3(0, 1, 0), mat);
        check("C 零边(退化)打不中",
              zero.hit(Ray(Point3(0, 0.5, 5), Vec3(0, 0, -1)), 0.001, infinity, rec), false);

        // ---------- 细针三角形：夹角 1e-9 弧度 ----------
        // sin²θ ≈ 1e-18 < 1e-12 → 判为退化（法线方向不可信）
        Triangle sliver(Point3(0, 0, 0), Point3(1, 0, 0), Point3(1, 1e-9, 0), mat);
        check("C 细针三角形(夹角1e-9)判为退化",
              sliver.hit(Ray(Point3(0.75, 1e-10, 5), Vec3(0, 0, -1)), 0.001, infinity, rec), false);

        // ---------- ★ 小三角形：边长 1e-3 ----------
        // uu = vv = 1e-6，len2 = 1e-12
        //   判据 len2 < 1e-8            → 1e-12 < 1e-8  成立 → 误判退化 ❌
        //   判据 len2 < 1e-12 · uu · vv → 1e-12 < 1e-24 不成立 → 正常 ✅
        // 这条就是用来抓"epsilon 没做尺度归一化"的
        Triangle tiny(Point3(0, 0, 0), Point3(1e-3, 0, 0), Point3(0, 1e-3, 0), mat);
        check("C 小三角形(边长1e-3)仍能命中",
              tiny.hit(Ray(Point3(3e-4, 3e-4, 5), Vec3(0, 0, -1)), 0.001, infinity, rec), true);
    }

    // ========================================================
    // 组 D：包围盒 —— 证明"三角形用的是平行四边形的盒子"
    // ========================================================
    {
        Triangle tri(Point3(0, 0, 0), Point3(1, 0, 0), Point3(0, 1, 0), mat);

        AABB box;
        check("D 能算出包围盒", tri.bounding_box(box), true);
        check("D 包围盒含顶点 a (0,0,0)",   box_contains(box, Point3(0, 0, 0)), true);
        check("D 包围盒含顶点 b (1,0,0)",   box_contains(box, Point3(1, 0, 0)), true);
        check("D 包围盒含顶点 c (0,1,0)",   box_contains(box, Point3(0, 1, 0)), true);

        // ★ 幽灵角：(0,0,0) + u + v = (1,1,0)。它【不是】三角形的顶点，
        //   但包围盒是平行四边形的盒子，所以一定包含它 —— 这就是"偏大"。
        check("D 包围盒含幽灵角 (1,1,0) —— 偏大但不坏",
              box_contains(box, Point3(1, 1, 0)), true);

        // 但三角形本身不包含幽灵角
        HitRecord rec;
        check("D 三角形本身不含幽灵角",
              tri.hit(Ray(Point3(1, 1, 5), Vec3(0, 0, -1)), 0.001, infinity, rec), false);

        // 包围盒在 z 方向本应 0 厚度，被 pad_to_minimums 撑开了
        check("D 包围盒 z 厚度被撑开",
              std::fabs((box.maximum.z() - box.minimum.z()) - 0.0001) < 1e-12, true);
    }

    // ========================================================
    // 组 E：★ 今天的核心验收 —— 同一立方体面，两种拆法
    //
    //   +z 面：Q = (-1,-1,1)   u = (2,0,0)   v = (0,2,0)
    //   拆成 (Q, Q+u, Q+v) 和 (Q+u, Q+u+v, Q+v)
    // ========================================================
    {
        Quad     qface(Point3(-1, -1, 1), Vec3(2, 0, 0), Vec3(0, 2, 0), mat);
        Triangle t1(Point3(-1, -1, 1), Point3( 1, -1, 1), Point3(-1, 1, 1), mat);
        Triangle t2(Point3( 1, -1, 1), Point3( 1,  1, 1), Point3(-1, 1, 1), mat);

        int agree_t = 0, agree_n = 0, total = 0;

        for (int i = 0; i < 7; ++i) {
            for (int j = 0; j < 7; ++j) {
                double x = -0.9 + 0.3 * i;
                double y = -0.9 + 0.3 * j;
                Ray r(Point3(x, y, 5), Vec3(0, 0, -1));

                HitRecord rq, ra, rb;
                bool hq = qface.hit(r, 0.001, infinity, rq);
                bool ha = t1.hit(r, 0.001, infinity, ra);
                bool hb = t2.hit(r, 0.001, infinity, rb);
                bool ht = ha || hb;
                double tt = ha ? ra.t : rb.t;             // 只有 ht 为真时才用
                const Vec3& tn = ha ? ra.normal : rb.normal;

                ++total;
                if (hq == ht && (!hq || std::fabs(rq.t - tt) < 1e-9)) ++agree_t;
                if (hq == ht && (!hq ||
                    (std::fabs(rq.normal.x() - tn.x()) < 1e-12 &&
                     std::fabs(rq.normal.y() - tn.y()) < 1e-12 &&
                     std::fabs(rq.normal.z() - tn.z()) < 1e-12))) ++agree_n;
            }
        }

        std::cout << "        (共 " << total << " 条射线，t 一致 "
                  << agree_t << "，法线一致 " << agree_n << ")\n";
        check("E Quad 面与 2 个 Triangle 的 t 完全一致", agree_t == total, true);
        check("E Quad 面与 2 个 Triangle 的法线完全一致（绕向正确）", agree_n == total, true);
    }

    std::cout << "\n  === 结果：" << g_pass << " 通过, " << g_fail << " 失败 ===\n\n";
    return g_fail == 0 ? 0 : 1;
}
