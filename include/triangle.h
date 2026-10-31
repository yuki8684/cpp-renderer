#pragma once

#include "quad.h"

// ============================================================
// UV 坐标（Day 22）
//
// 单独定义一个小结构，而不是用两个 double 满处传：
//   —— 可读性（set_uv(a, b, c) 比 6 个 double 清楚得多）
//   —— 以后要加第三维（w、layer）时只改这一处
// ============================================================
struct UV {
    double u = 0.0;
    double v = 0.0;
};

// ============================================================
// Triangle：三角形
//
// 和 Quad 的唯一区别就是"内部判定"。
// 所以它【继承】Quad，不重写 hit()、不重写 bounding_box()、
// 也不重写平面参数的计算 —— 只覆盖 is_interior 这一个虚函数。
//
// 顶点约定：v0 = a, v1 = b, v2 = c
// Quad 的参数化是 P = a + α·u + β·v
// 要让 P 能表示三角形内部，必须让 u、v 分别指向另外两个顶点
// ============================================================
class Triangle : public Quad {
public:
    Triangle(const Point3& a, const Point3& b, const Point3& c,
             std::shared_ptr<Material> mat)
        : Quad(a, b - a, c - a, mat) {   // TODO(T1-1)：两条边向量
    }


    // ---------- 光滑着色（Day 15 新增）----------
    // 光滑着色：每个顶点各自一个法线，面内插值，法线逐点变化，效果看不出三角形边界，像曲面，亮度连续渐变平滑
    // n0 / n1 / n2 分别是顶点 a / b / c 处的法线，必须是单位向量
    Triangle(const Point3& a, const Point3& b, const Point3& c, 
        const Vec3& n0, const Vec3& n1, const Vec3& n2, 
        std::shared_ptr<Material> mat)
        : Quad(a, b - a, c - a, mat),
          n0(n0), n1(n1), n2(n2), smooth(true) {}

    //   TODO(T1-2)：内部判定（三角形版）
    //   条件： α ≥ 0  且  β ≥ 0  且  α + β ≤ 1
    //   ⚠️ 用 >= / <= （闭集），不要用 > / <
    bool is_interior(double alpha, double beta) const override {
        return alpha >= 0 && beta >= 0 && alpha + beta <= 1;
    }

    void set_hit_normal(const Ray& r, HitRecord& rec, 
                        double alpha, double beta) const override {
        if (!smooth) {
            // 平面着色：法线 = 面法线；(u,v) 用 (α, β)
            // （Quad 的默认行为已经把 rec.u / rec.v 设成了 (α, β)）
            Quad::set_hit_normal(r, rec, alpha, beta);
        } else {
            // 重心坐标插值法线
            Vec3 n = (1 - alpha - beta) * n0 + alpha * n1 + beta * n2;
            rec.set_face_normal(r, unit_vector(n));
            rec.u = alpha;
            rec.v = beta;
        }

        // ------------------------------------------------------
        // TODO(T6)：如果有显式 UV，用重心坐标插值覆盖它
        //
        //   和【法线插值是同一套权重】：
        //       w_a = 1 − α − β      （顶点 a 的权重）
        //       w_b = α              （顶点 b）
        //       w_c = β              （顶点 c）
        //
        //       rec.u = w_a·uv0.u + w_b·uv1.u + w_c·uv2.u
        //       rec.v = w_a·uv0.v + w_b·uv1.v + w_c·uv2.v
        //
        //   ⚠️ 必须放在最后 —— 它要覆盖上面设的 (α, β)
        // ------------------------------------------------------
        if (has_uv) {
            double w_a = 1.0 - alpha - beta;
            rec.u = w_a * uv0.u + alpha * uv1.u + beta * uv2.u;
            rec.v = w_a * uv0.v + alpha * uv1.v + beta * uv2.v;
        }
    }

    // ============================================================
    // Day 22：显式 UV（来自 OBJ 的 `vt`）
    //
    // ⚠️ 为什么用 setter 而不是再写一个构造函数？
    //    构造函数已经有 8 个参数了，再加 3 个 UV 就是 11 个 ——
    //    调用点完全读不懂。这是"参数过多"的典型信号。
    //    真实渲染器会用 struct 打包顶点（position / normal / uv 放一个结构里），
    //    这里为了改动最小，用 setter。
    // ============================================================
    void set_uv(const UV& t0, const UV& t1, const UV& t2) {
        uv0   = t0;
        uv1   = t1;
        uv2   = t2;
        has_uv = true;
    }

    bool has_explicit_uv() const { return has_uv; }


    private:
         Vec3 n0, n1, n2;
         bool smooth = false;

         // Day 22：顶点 UV（默认 (0,0)；未调用 set_uv 时 has_uv = false）
         UV   uv0, uv1, uv2;
         bool has_uv = false;
};