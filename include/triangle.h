#pragma once

#include "quad.h"
#include "texture.h"     // Day 25：apply_normal_map 要调 Texture::value()（不能只靠前向声明）

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

        // ------------------------------------------------------
        // Day 25：法线贴图 —— 用切线空间把法线"掰"一下
        //
        // 顺序很关键：必须在 rec.u/v 已经算完之后，因为采样要用它们。
        // ------------------------------------------------------
        apply_normal_map(rec);
    }

    // ============================================================
    // 法线贴图里存的是【切线空间】的法线 (nx, ny, nz)，不是世界空间的。
    // 要变成世界法线，需要三个基向量：
    //     T  切线   = dP/du
    //     B  副切线 = dP/dv
    //     N  法线   = rec.normal（几何法线，已经翻向观察者）
    //
    //     世界法线 = nx·T + ny·B + nz·N
    //
    // ★ T 和 B 是【每个三角形固定的】（只跟顶点位置和 UV 有关），
    //   所以在 set_uv() 里算一次就够，不用每个交点重算。
    // ============================================================
    void apply_normal_map(HitRecord& rec) const {
        if (!has_tangent) return;

        auto nm = material_ptr()->normal_map();   // 没有法线贴图 → 直接返回（零开销）
        if (!nm) return;

        const Vec3 N = rec.normal;                // 几何法线（已翻向观察者）

        // ---- ① Gram-Schmidt：把 tangent 里平行于 N 的分量减掉 ----
        //   tangent 躺在【三角形平面】里，而 N 是【插值顶点法线】，
        //   两者不一定垂直。切线空间要求 T ⊥ N，所以先正交化。
        //   dot(tangent, N)·N 就是 tangent 在 N 方向上的投影向量。
        Vec3 T = tangent - dot(tangent, N) * N;

        //   减完长度变了（|T|·cosθ），必须重新归一化。
        //   ★ 兜底：若 T 恰好平行于 N，减完是零向量 → 再除就 NaN。
        //     这时直接 return（退回几何法线：法线贴图失效，但画面不会崩）。
        //     写法用 !(x > 0) 而不是 x <= 0：前者对 NaN 也安全。
        const double tl2 = dot(T, T);
        if (!(tl2 > 0.0)) return;
        T = T / std::sqrt(tl2);

        // ---- ② 副切线：用叉积，不用成员 bitangent ----
        //   理由有二：
        //     a) 上面 Gram-Schmidt 改了 T，解出来的 bitangent 不再垂直于新 T；
        //     b) bitangent 的【符号】取决于 UV 是否镜像，而 cross(N,T) 恒为右手系。
        //        一旦符号反了，绿通道的凹凸就整体翻过来（凸起变凹坑）。
        //   记忆：T → B → N → T 是循环序，和 x → y → z → x 一样。
        //         所以 cross(N,T) = B、cross(T,B) = N。
        const Vec3 B = cross(N, T);

        // ---- ③ 采样切线空间法线，再按 T/B/N 展开到世界空间 ----
        //   ns 的三个分量就是"沿 T 走多少、沿 B 走多少、沿 N 走多少"。
        const Color ns = nm->value(rec.u, rec.v, rec.p);   // 已经是 −1~1
        rec.normal = unit_vector(ns.x() * T + ns.y() * B + ns.z() * N);

        // ⚠️ 不动 rec.front_face：它记录的是"光线从三角形哪一侧来"这个【几何】
        //    事实；法线贴图只是着色细节。改了它，所有依赖 front_face 的判断
        //    （介质进出、背面剔除）都会跟着错乱。
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
        compute_tangent_frame();      // Day 25：UV 一定下来，切线帧也就定了
    }

    bool has_explicit_uv() const { return has_uv; }

    // ============================================================
    // 从「位置 + UV」解出切线帧
    //
    // 一个三角形的三个顶点有两套坐标：位置（3D）和 UV（2D）。
    // 它们是【同一个曲面的两种参数化】，所以：
    //
    //     e1 = P1 − P0 = du1·T + dv1·B
    //     e2 = P2 − P0 = du2·T + dv2·B
    //
    // 这是 2×2 线性方程组 —— 解出来就是 T 和 B。
    //
    // ★ 不需要旋转矩阵、不需要叉积、不需要三角函数。
    // ★ e1 / e2 直接就是 Quad 的 edge_u() / edge_v()
    //   （因为 Triangle 是 Quad(a, b−a, c−a)）
    // ============================================================
    void compute_tangent_frame() {
        has_tangent = false;
        if (!has_uv) return;

        const Vec3 e1 = edge_u();          // = P1 − P0
        const Vec3 e2 = edge_v();          // = P2 − P0

        // UV 增量的四个数，直接用 UV 结构装（它本来就是二维量）
        //   duv1 = (Δu1, Δv1)，duv2 = (Δu2, Δv2)
        const UV duv1{ uv1.u - uv0.u, uv1.v - uv0.v };
        const UV duv2{ uv2.u - uv0.u, uv2.v - uv0.v };

        // 待解的 2×2 方程组：
        //     e1 = duv1.u·T + duv1.v·B
        //     e2 = duv2.u·T + duv2.v·B
        //
        // det 是二维叉积：|det| = UV 三角形面积的 2 倍，符号 = UV 是否镜像。
        const double det = duv1.u * duv2.v - duv2.u * duv1.v;

        // 退化：det = 0 ⇔ 三个 UV 共线 ⇔ 这块三角形在纹理上被压成一条线，
        // 根本没有"u 方向"可言。
        // ★ 必须写成 !(fabs(det) > eps)：NaN 参与任何比较都是 false，
        //   写成 fabs(det) < eps 会把 NaN 误判为"有效"，让 NaN 一路传到画面上。
        if (!(std::fabs(det) > 1e-20)) return;

        // 克拉默法则
        const double inv = 1.0 / det;
        tangent   = ( e1 * duv2.v - e2 * duv1.v ) * inv;   // dP/du（未归一化）
        bitangent = ( e2 * duv1.u - e1 * duv2.u ) * inv;   // dP/dv（仅作对照；着色用 cross(N,T)）
        has_tangent = true;
    }


    private:
         Vec3 n0, n1, n2;
         bool smooth = false;

         // Day 22：顶点 UV（默认 (0,0)；未调用 set_uv 时 has_uv = false）
         UV   uv0, uv1, uv2;
         bool has_uv = false;

         // Day 25：切线帧（set_uv 时算一次，之后不变）
         Vec3 tangent, bitangent;
         bool has_tangent = false;
};