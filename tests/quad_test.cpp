// ============================================================
// Quad 单元测试（独立可执行文件）
//
// 为什么值得单独写？
//   Quad 里有 10 个公式（叉积求法线、平面求交、平面内坐标、内部判定）。
//   靠"渲染出来看一眼"根本分不清哪一步错了 ——
//   画面可能"看起来像个盒子"，但尺寸、位置全是错的。
//
//   单元测试的好处：每个数字都是【手工算过】的，
//   错在哪一行立刻定位。
//
// 全部期望值都在注释里写了推导过程。
// ============================================================

#include <iostream>
#include <cmath>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "ray.h"
#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "bvh.h"
#include "material.h"
#include "lambertian.h"
#include "quad.h"

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

static void check_near(const char* name, double got, double want, double tol = 1e-9) {
    if (std::fabs(got - want) <= tol) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << got << "  期望=" << want << '\n';
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== Quad 单元测试 =====\n";

    auto mat = std::make_shared<Lambertian>(Color(0.5, 0.5, 0.5));

    // ========================================================
    // 组 A：轴对齐正方形（u ⊥ v，最容易心算）
    //
    //   Q = (0,0,0)
    //   u = (2,0,0)     沿 x，长度 2
    //   v = (0,0,2)     沿 z，长度 2
    //
    // 预处理（手工验算）：
    //   n = u × v = (0·2−0·0, 0·0−2·2, 2·0−0·0) = (0,−4,0)
    //   normal = (0,−1,0)
    //   D = normal·Q = 0
    //   n·n = 16  →  w = (0,−0.25,0)
    // ========================================================
    Quad q(Point3(0, 0, 0), Vec3(2, 0, 0), Vec3(0, 0, 2), mat);

    // ---------- 正中心 ----------
    {
        HitRecord rec;
        Ray r(Point3(1, 5, 1), Vec3(0, -1, 0));

        // denom = (0,−1,0)·(0,−1,0) = 1
        // t     = (0 − (0,−1,0)·(1,5,1)) / 1 = (0 − (−5)) / 1 = 5
        // p     = (1, 0, 1)
        // p−Q   = (1, 0, 1)
        // p×v   = (1,0,1)×(0,0,2) = (0·2−1·0, 1·0−1·2, 1·0−0·0) = (0,−2,0)
        // α     = (0,−0.25,0)·(0,−2,0) = 0.5
        // u×p   = (2,0,0)×(1,0,1) = (0·1−0·0, 0·1−2·1, 2·0−0·1) = (0,−2,0)
        // β     = 0.5
        bool hit = q.hit(r, 0.001, infinity, rec);
        check("A 正中心命中", hit, true);
        if (hit) {
            check_near("A t = 5", rec.t, 5.0);
            check_near("A 交点 x = 1", rec.p.x(), 1.0);
            check_near("A 交点 y = 0", rec.p.y(), 0.0);
            check_near("A 交点 z = 1", rec.p.z(), 1.0);
            // 光线从上方下来，几何外法线是 −y，翻面后应朝上 +y
            check("A 命中背面 (front_face=false)", rec.front_face, false);
            check_near("A 翻面后法线 y = +1", rec.normal.y(), 1.0);
        }
    }

    // ---------- x 方向打偏（α = 1.5 > 1）----------
    // O=(3,5,1) → p=(3,0,1)
    // p×v = (3,0,1)×(0,0,2) = (0, −6, 0)  →  α = 1.5
    {
        HitRecord rec;
        check("A x 超界 (α=1.5)",
              q.hit(Ray(Point3(3, 5, 1), Vec3(0, -1, 0)), 0.001, infinity, rec), false);
    }

    // ---------- z 方向打偏（β = 2 > 1）----------
    // O=(1,5,4) → p=(1,0,4)
    // u×p = (2,0,0)×(1,0,4) = (0, −8, 0)  →  β = 2.0
    {
        HitRecord rec;
        check("A z 超界 (β=2)",
              q.hit(Ray(Point3(1, 5, 4), Vec3(0, -1, 0)), 0.001, infinity, rec), false);
    }

    // ---------- 负方向打偏（α = −0.5）----------
    // O=(−1,5,1) → p=(−1,0,1)
    // p×v = (−1,0,1)×(0,0,2) = (0·2−1·0, 1·0−(−1)·2, 0) = (0, 2, 0)  →  α = −0.5
    {
        HitRecord rec;
        check("A 负方向超界 (α=−0.5)",
              q.hit(Ray(Point3(-1, 5, 1), Vec3(0, -1, 0)), 0.001, infinity, rec), false);
    }

    // ---------- 平行于平面 ----------
    // d = (1,0,0)，normal = (0,−1,0) → denom = 0
    {
        HitRecord rec;
        check("A 光线平行于平面",
              q.hit(Ray(Point3(1, 5, 1), Vec3(1, 0, 0)), 0.001, infinity, rec), false);
    }

    // ---------- 平面在光线【身后】----------
    // d = (0,1,0) → denom = −1，t = −5 < t_min
    {
        HitRecord rec;
        check("A 平面在身后 (t=−5)",
              q.hit(Ray(Point3(1, 5, 1), Vec3(0, 1, 0)), 0.001, infinity, rec), false);
    }

    // ---------- t 窗口 ----------
    // t = 5，t_min = 6 → 太近
    {
        HitRecord rec;
        check("A t_min=6 挡住",
              q.hit(Ray(Point3(1, 5, 1), Vec3(0, -1, 0)), 6.0, infinity, rec), false);
    }
    // t = 5，t_max = 4 → 太远
    {
        HitRecord rec;
        check("A t_max=4 挡住",
              q.hit(Ray(Point3(1, 5, 1), Vec3(0, -1, 0)), 0.001, 4.0, rec), false);
    }
    // t = 5，t_max = 6 → 通过
    {
        HitRecord rec;
        check("A t_max=6 通过",
              q.hit(Ray(Point3(1, 5, 1), Vec3(0, -1, 0)), 0.001, 6.0, rec), true);
    }

    // ---------- 边界点 ----------
    // P=(0,0,0) → α=0, β=0（is_interior 用的是 >= 和 <=，边界算命中）
    {
        HitRecord rec;
        check("A 角点 (α=0,β=0) 算命中",
              q.hit(Ray(Point3(0, 5, 0), Vec3(0, -1, 0)), 0.001, infinity, rec), true);
    }
    // P=(2,0,2) → α=1, β=1
    {
        HitRecord rec;
        check("A 对角 (α=1,β=1) 算命中",
              q.hit(Ray(Point3(2, 5, 2), Vec3(0, -1, 0)), 0.001, infinity, rec), true);
    }

    // ========================================================
    // 组 B：斜的平行四边形（u 与 v 【不垂直】）
    //
    //   Q = (0,0,0)
    //   u = (2,0,0)
    //   v = (1,0,2)        ← 斜的
    //
    //   n = u × v = (0·2−0·0, 0·1−2·2, 2·0−0·1) = (0,−4,0)
    //   → 和组 A 一模一样！（因为 u、v 都在 XZ 平面内，法线都是 ±y）
    //   w = (0,−0.25,0)
    //
    //   这组专门测"u、v 不垂直时，(α,β) 还能不能算对"。
    // ========================================================
    Quad qs(Point3(0, 0, 0), Vec3(2, 0, 0), Vec3(1, 0, 2), mat);

    // ---------- 中心：P=(2,0,1) ----------
    // p = (2,0,1)
    // p×v = (2,0,1)×(1,0,2) = (0·2−1·0, 1·1−2·2, 2·0−0·1) = (0,−3,0)
    //     wait：  (1·1 − 2·2) = 1 − 4 = −3
    // α = (0,−0.25,0)·(0,−3,0) = 0.75
    // u×p = (2,0,0)×(2,0,1) = (0·1−0·0, 0·2−2·1, 2·0−0·2) = (0,−2,0)
    // β = 0.5
    // 验算：Q + 0.75·u + 0.5·v = (1.5,0,0) + (0.5,0,1) = (2,0,1)  ✓
    {
        HitRecord rec;
        bool hit = qs.hit(Ray(Point3(2, 5, 1), Vec3(0, -1, 0)), 0.001, infinity, rec);
        check("B 斜四边形中心命中 (α=0.75, β=0.5)", hit, true);
        if (hit) check_near("B t = 5", rec.t, 5.0);
    }

    // ---------- β = 1.1（超出）----------
    // P = 0.5·u + 1.1·v = (1,0,0) + (1.1,0,2.2) = (2.1, 0, 2.2)
    // p×v = (2.1,0,2.2)×(1,0,2) = (0·2−2.2·0, 2.2·1−2.1·2, 0) = (0, −2, 0) → α = 0.5
    // u×p = (2,0,0)×(2.1,0,2.2) = (0, 0−4.4, 0) = (0,−4.4,0)      → β = 1.1
    {
        HitRecord rec;
        check("B β=1.1 超出",
              qs.hit(Ray(Point3(2.1, 5, 2.2), Vec3(0, -1, 0)), 0.001, infinity, rec), false);
    }

    // ---------- β = 0.9（刚好在内）----------
    // P = 0.5·u + 0.9·v = (1,0,0) + (0.9,0,1.8) = (1.9, 0, 1.8)
    // u×p = (2,0,0)×(1.9,0,1.8) = (0, 0−3.6, 0) = (0,−3.6,0)       → β = 0.9
    {
        HitRecord rec;
        check("B β=0.9 刚好在内",
              qs.hit(Ray(Point3(1.9, 5, 1.8), Vec3(0, -1, 0)), 0.001, infinity, rec), true);
    }

    // ========================================================
    // 组 C：包围盒与 BVH 一致性（今天最关键的验证）
    // ========================================================
    {
        AABB box;
        check("C 能算出包围盒", qs.bounding_box(box), true);

        // qs 完全躺在 y=0 平面上 → 原始厚度 0，
        // 经过 pad_to_minimums 后应该是 0.0001
        check_near("C 包围盒 y 厚度被撑开到 0.0001",
                   box.maximum.y() - box.minimum.y(), 0.0001, 1e-9);

        // 撑开后的盒子必须【能被打中】—— 否则 BVH 会错误跳过它
        check("C 包围盒可被光线命中",
              box.hit(Ray(Point3(2, 5, 1), Vec3(0, -1, 0)), 0.001, infinity), true);
    }

    {
        // 同一批射线，分别喂给"裸 Quad"和"套了 BVH 的 Quad"，
        // 命中结果必须完全一致。
        auto list_quad = std::make_shared<Quad>(Point3(0, 0, 0), Vec3(2, 0, 0), Vec3(0, 0, 2), mat);
        HittableList list;
        list.add(list_quad);
        BVH bvh(list);

        struct Probe { double ox, oz; bool want; };
        const Probe probes[] = {
            { 1.0,  1.0, true  },   // 中心
            { 0.0,  0.0, true  },   // 角点
            { 2.0,  2.0, true  },   // 对角
            { 3.0,  1.0, false },   // α 超界
            { 1.0,  4.0, false },   // β 超界
            {-1.0,  1.0, false },   // α 负
        };

        int agree = 0, total = 0;
        for (const auto& p : probes) {
            Ray r(Point3(p.ox, 5, p.oz), Vec3(0, -1, 0));
            HitRecord ra, rb;
            bool a = list_quad->hit(r, 0.001, infinity, ra);
            bool b = bvh.hit(r, 0.001, infinity, rb);
            ++total;
            if (a == b && a == p.want) ++agree;
        }
        check("C 裸 Quad 与 BVH 结果一致（6 条射线）", agree == total, true);
    }

    // ========================================================
    std::cout << "\n  === 结果：" << g_pass << " 通过, " << g_fail << " 失败 ===\n\n";
    return (g_fail == 0) ? 0 : 1;
}
