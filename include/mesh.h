#pragma once

#include "hittable_list.h"
#include "triangle.h"
#include "vec3.h"

// ============================================================
// 球面网格：用三角形拼一个球（Day 15）
//
// 为什么用球来演示光滑着色？
//   球的顶点法线有【解析解】：  n = (p − center) / radius
//   不需要估计，不需要"平均相邻面法线"。
//
// 参数化（v 是纬度，从北极 0 到南极 π；u 是经度，绕一圈 0→2π）：
//
//   dir(u,v) = ( sin v·cos u ,  cos v ,  sin v·sin u )
//   pos      = center + radius · dir
//
//   注意 dir 本身就是单位向量，而且正好等于球面在该点的外法线。
//
// rings    纬度分段数
// segments 经度分段数
// 三角形总数 ≈ rings × segments × 2（极点处有退化的会被跳过）
//
// ⚠️ 绕向必须朝外：四边形 (A,B,C,D) 拆成 (A,B,D) 和 (B,C,D)
//    其中 A=(v0,u0) B=(v0,u1) C=(v1,u1) D=(v1,u0)
//    这样 cross(∂/∂u, ∂/∂v) 正好是外法线。
// ============================================================
inline void add_sphere_mesh(HittableList& world,
                            const Point3& center, double radius,
                            int rings, int segments,
                            std::shared_ptr<Material> mat,
                            bool smooth) {

    auto dir_at = [&](int j, int i) {
        double v = pi * double(j) / double(rings);
        double u = 2.0 * pi * double(i) / double(segments);
        return Vec3(std::sin(v) * std::cos(u),
                    std::cos(v),
                    std::sin(v) * std::sin(u));
    };

    // 加一个三角形（三个参数是【方向】，位置由 center + radius*dir 算出来）
    auto add_tri = [&](const Vec3& d0, const Vec3& d1, const Vec3& d2) {
        Point3 a = center + radius * d0;
        Point3 b = center + radius * d1;
        Point3 c = center + radius * d2;

        // 退化判定 —— 和 Quad 里一样，必须【尺度无关】。
        // 极点那一圈的四边形会退化成三角形，其中一个顶点重合，这里跳过。
        Vec3 e1 = b - a;
        Vec3 e2 = c - a;
        Vec3 n  = cross(e1, e2);
        double nn = dot(n, n);
        if (nn <= 0.0 || nn < 1e-12 * dot(e1, e1) * dot(e2, e2)) return;

        if (smooth) {
            // 光滑着色：把三个顶点各自的法线也传进去
            world.add(std::make_shared<Triangle>(a, b, c, d0, d1, d2, mat));
        } else {
            // 平面着色：不传顶点法线，用面法线
            world.add(std::make_shared<Triangle>(a, b, c, mat));
        }
    };

    for (int j = 0; j < rings; ++j) {
        for (int i = 0; i < segments; ++i) {
            Vec3 d00 = dir_at(j,     i);       // A
            Vec3 d01 = dir_at(j,     i + 1);   // B
            Vec3 d11 = dir_at(j + 1, i + 1);   // C
            Vec3 d10 = dir_at(j + 1, i);       // D

            add_tri(d00, d01, d10);            // (A, B, D)
            add_tri(d01, d11, d10);            // (B, C, D)
        }
    }
}
