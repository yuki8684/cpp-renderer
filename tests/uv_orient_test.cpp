// ============================================================
// 六视图 UV 朝向测试（Day 26）
//
// 唯一判据（三条）：
//   ① 四个侧面（法线沿 x 或 z）：+v 指向世界 +y
//   ② 顶面 / 底面（法线沿 y）   ：+v 指向世界 +z
//   ③ 不镜像                    ：cross(U, V) == +外法线
//
// ★ ③ 为什么是 +n 而不是 −n：
//   从外面看一个面，要让贴图不镜像，必须让 u 向右增加、v 向上增加。
//   设屏幕右 = a、屏幕上 = b，则 U = a、V = b。
//   而 {a, b, n} 构成右手系（a × b = n，n 指向观察者）
//   → cross(U, V) = +n。
//
//   （Day 23 生成的六视图 UV 一直是 cross(U,V) = −n，即六个面都镜像，
//     只是六个面镜像方式一致，所以彼此对比看不出来。Day 26 修正。）
//
// 怎么算一个面的 +u / +v？—— 就是 Day 25 的切线帧：
//
//   e1 = P1 − P0 = du1·U + dv1·V
//   e2 = P2 − P0 = du2·U + dv2·V
//   det = du1·dv2 − du2·dv1
//   U = ( +dv2·e1 − dv1·e2 ) / det
//   V = ( −du2·e1 + du1·e2 ) / det
//
// ★ 这个测试【不依赖生成器的面顺序】：面的身份由「哪个坐标恒定」决定。
//   所以以后 $cubeF 怎么排都不影响测试的正确性。
//
// ★ 判据 ① 是「规格」不是「实现」：+v 指向 +y 意味着贴图在四面墙上都是正的。
//   判据 ③ 是「不引入第二次变化」：修旋转的时候不要顺手把手性也翻掉。
// ============================================================

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "obj_loader.h"

static int g_pass = 0;
static int g_fail = 0;

static void check_vec(const char* name, const Vec3& got, const Vec3& want,
                      double tol = 1e-9) {
    const bool ok = std::fabs(got.x() - want.x()) <= tol &&
                    std::fabs(got.y() - want.y()) <= tol &&
                    std::fabs(got.z() - want.z()) <= tol;
    if (ok) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=(" << got.x() << ", " << got.y() << ", " << got.z() << ")"
                  << "  期望=(" << want.x() << ", " << want.y() << ", " << want.z() << ")\n";
    }
}

static void check_near(const char* name, double got, double want, double tol = 1e-9) {
    if (std::fabs(got - want) <= tol) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name << "    得到=" << got << "  期望=" << want << '\n';
    }
}

static void check_bool(const char* name, bool got, bool want = true) {
    if (got == want) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name << "    得到=" << (got ? "true" : "false")
                  << "  期望=" << (want ? "true" : "false") << '\n';
    }
}

// Vec3 没有 operator[]，手动按轴取分量
static double comp(const Vec3& p, int a) {
    return a == 0 ? p.x() : (a == 1 ? p.y() : p.z());
}

static Vec3 axis_unit(int a) {
    return a == 0 ? Vec3(1, 0, 0) : (a == 1 ? Vec3(0, 1, 0) : Vec3(0, 0, 1));
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 六视图 UV 朝向测试（Day 26） =====\n";

    ObjData d = load_obj("models/cube_uv.obj");
    if (d.tri_indices.empty()) {
        std::cout << "  [FAIL] 读不到 models/cube_uv.obj\n";
        return 1;
    }

    const int ntri = static_cast<int>(d.tri_indices.size()) / 3;
    std::cout << "  三角形数 = " << ntri
              << "   tri_uvs = " << d.tri_uvs.size() << "\n";

    // 生成器把每个四边形拆成 2 个三角形（扇形三角化：0-1-2 和 0-2-3）
    const int nquad = ntri / 2;
    std::cout << "\n--- 组 O1~O" << (nquad * 2) << "：每面的 +u / +v ---\n";

    double uv_area_sum = 0.0;
    bool   handedness_ok = true;
    int    nface_seen = 0;

    for (int f = 0; f < nquad; ++f) {
        const int t = 2 * f;

        const Vec3 P0 = d.positions[d.tri_indices[3 * t + 0]];
        const Vec3 P1 = d.positions[d.tri_indices[3 * t + 1]];
        const Vec3 P2 = d.positions[d.tri_indices[3 * t + 2]];
        const UV   u0 = d.tri_uvs[3 * t + 0];
        const UV   u1 = d.tri_uvs[3 * t + 1];
        const UV   u2 = d.tri_uvs[3 * t + 2];

        // ---- 面的身份：哪一个坐标在三个顶点上恒定 ----
        int    axis = -1;
        double cval = 0.0;
        for (int a = 0; a < 3; ++a) {
            if (std::fabs(comp(P0, a) - comp(P1, a)) < 1e-9 &&
                std::fabs(comp(P0, a) - comp(P2, a)) < 1e-9) {
                axis = a;
                cval = comp(P0, a);
            }
        }
        if (axis < 0) {
            std::cout << "  [FAIL] 四边形 " << f << " 找不到恒定坐标（不是立方体的面？）\n";
            ++g_fail;
            continue;
        }
        ++nface_seen;

        const Vec3   n     = (cval > 0.0 ? 1.0 : -1.0) * axis_unit(axis);   // 外法线
        const char   ax_ch = "xyz"[axis];
        const int    sg    = (cval > 0.0) ? 1 : -1;

        // ---- Day 25 的 2×2 解 ----
        const Vec3   e1  = P1 - P0;
        const Vec3   e2  = P2 - P0;
        const double du1 = u1.u - u0.u, dv1 = u1.v - u0.v;
        const double du2 = u2.u - u0.u, dv2 = u2.v - u0.v;
        const double det = du1 * dv2 - du2 * dv1;

        if (!(std::fabs(det) > 1e-20)) {
            std::cout << "  [FAIL] 四边形 " << f << " 的 UV 退化（det = 0）\n";
            ++g_fail;
            continue;
        }

        const Vec3 U = unit_vector(( dv2 * e1 - dv1 * e2) / det);
        const Vec3 V = unit_vector((-du2 * e1 + du1 * e2) / det);

        // ---- 期望值：由规格推导，不是抄表 ----
        //   ① V 指向 +y（侧面）或 +z（顶底）
        //   ③ U = cross(V, n)  ← 保证 cross(U,V) = +n（不镜像）
        const Vec3 expV = (axis == 1) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
        const Vec3 expU = cross(expV, n);

        char name[128];
        std::snprintf(name, sizeof(name), "O%d  %c = %+d 面  +v", 2 * f + 1, ax_ch, sg);
        check_vec(name, V, expV);

        std::snprintf(name, sizeof(name), "O%d  %c = %+d 面  +u", 2 * f + 2, ax_ch, sg);
        check_vec(name, U, expU);

        // ---- 手性（累积判定）----
        const Vec3 cr = cross(U, V);
        if (std::fabs(cr.x() - n.x()) > 1e-9 ||
            std::fabs(cr.y() - n.y()) > 1e-9 ||
            std::fabs(cr.z() - n.z()) > 1e-9) {
            handedness_ok = false;
        }

        // ---- UV 面积（累积）----
        uv_area_sum += std::fabs(det) / 2.0;
    }

    std::cout << "\n--- 组 O" << (nquad * 2 + 1) << "~：全局不变量 ---\n";

    check_bool("O13 六个面不镜像（cross(U,V) == +外法线）", handedness_ok);
    check_near("O14 六个面 UV 面积和 == 6.0（每面铺满 [0,1]²）", uv_area_sum * 2.0, 6.0);
    check_near("O15 识别出 6 个面", static_cast<double>(nface_seen), 6.0);

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) std::cout << "ALL PASSED\n";
    return g_fail == 0 ? 0 : 1;
}
