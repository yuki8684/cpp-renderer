#pragma once

#include "aabb.h"
#include "hittable.h"
#include "vec3.h"

#include <memory>

// ============================================================
// Quad：平行四边形（quadrilateral）
//
// 这是渲染器的【第二个几何图元】—— 前面 13 天一直只有球。
//
// 为什么先做平行四边形，而不是三角形？
//   1. 三个参数就能定义清楚：一个角 Q、两条边向量 u / v
//   2. 三角形只是在"内部判定"上更严格一点点（见 is_interior）
//   3. 矩形是【面光源】的基础 → Cornell Box 需要它
//
// 几何约定：
//
//        Q+v ────────────── Q+u+v
//         │                  │
//         │                  │  v
//         │                  │
//         Q ────── u ─────── Q+u
//
//   形状内的任一点都能写成：   P = Q + α·u + β·v     (0 ≤ α ≤ 1, 0 ≤ β ≤ 1)
//
//   (α, β) 叫做这个点的【平面内坐标】。
//   有了它，判断"在不在形状里"就变成两次比较 —— 而且换形状只改这一个函数。
// ============================================================
class Quad : public Hittable {
public:
    Quad(const Point3& Q, const Vec3& u, const Vec3& v,
         std::shared_ptr<Material> mat)
        : Q(Q), u(u), v(v), mat(mat) {

        // ------------------------------------------------------------
        // 下面四行把平面参数【算一次、缓存起来】。
        // 求交时就不用重复算了 —— 这是"空间换时间"的经典用法。
        //
        // TODO(你填 Q1~Q4)：四行
        //
        //   Vec3   n = cross(u, v);            // 未归一化的法线
        //   normal   = unit_vector(n);         // 单位法线（HitRecord 要求单位长）
        //   D        = dot(normal, Q);         // 平面常数（因为 Q 就在平面上）
        //   w        = n / dot(n, n);          // 平面坐标用的辅助向量
        //
        //   ⚠️ w 的分母是 dot(n, n)，用的是【未归一化】的 n，不是 normal！
        //      （推导见 docs/devlog.md Day 13「原理 4」）
        // ------------------------------------------------------------

        Vec3 n = cross(u, v);//叉积，得到向量，垂直于u，v，为归一化

        double len2 = dot(n, n);// n = |u * v |，len2等于平行四边行面积平方
        double uu   = dot(u, u);
        double vv   = dot(v, v);

        // 如果u平行于v， 叉积得到的向量为0，unit_vector(n)除以0得到NAN，整个渲染崩溃
        //len2 = |u|²|v|²sin²θ，uu·vv = |u|²|v|²，len2 / (uu·vv) = sin²θ，因为|sinθ| < 1e-6，所以|sin²θ|小于1e-12
        if(len2 <= 0.0 || len2 < 1e-12 * uu * vv) {
            degenerate = true;
            normal     = Vec3(0, 1, 0);
            D          = 0.0;
            w          = Vec3(0, 0, 0);
            bbox       = AABB(Q, Q);
            return;
        }

        normal = unit_vector(n);
        D      = dot(normal, Q);
        w      = n / len2;
        area   = std::sqrt(len2);   // 面积 = |u × v|


        set_bounding_box();
    }

    // ============================================================
    // TODO(你填 Q5~Q8)：求交
    // ============================================================
    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {

        if(degenerate) return false;

        // ---------- 第 1 步：射线 × 无限平面 ----------
        //     平面方程（隐式）：  normal · P = D
        //     把 P = O + t·d 代进去：
        //     normal · (O + t·d) = D
        //     normal·O + t(normal·d) = D
        //     t = (D − normal·O) / (normal·d)
        //
        // TODO(你填 Q5)：分母
        double denom = dot(normal, r.direction());

        //   TODO(你填 Q6)：平行判定
        //   分母接近 0 → 射线平行于平面 → 永远打不中（或完全躺在平面里）
        //   ⚠️ 要取绝对值：光线也可以朝平面的【反方向】平行
        //   形状：  if (std::fabs(denom) < 1e-8) return false;

        if(std::fabs(denom) < 1e-8) return false;
        


        //    TODO(你填 Q7)：解出 t，并检查它是否落在 (t_min, t_max) 内
        //    形状：  double t = (D - dot(normal, r.origin())) / denom;
        double t = (D - dot(normal, r.origin())) / denom;
        if(t <= t_min || t >= t_max) return false;
        

        // ---------- 第 2 步：交点落在平行四边形里面吗？ ----------
        // 交点位置
        Point3 intersection = r.at(t);

        //   TODO(你填 Q8)：算平面内坐标 α（alpha）和 β（beta）
        //
        //   令 p = 交点 − Q
        //   则   alpha = dot(w, cross(p, v))
        //        beta  = dot(w, cross(u, p))
        //
        //   ⚠️ 注意两个叉积的【自变量顺序】不一样：cross(p,v) 和 cross(u,p)
        //      写反会得到一个符号相反的坐标
        //
        //   推导见 docs/devlog.md Day 13「原理 4」
        Vec3     p   = intersection - Q;
        double alpha = dot(w, cross(p, v));
        double beta  = dot(w, cross(u, p));

        //   TODO(你填 Q9)：调用内部判定
        //   形状：  if (!is_interior(alpha, beta)) return false;
        if(!is_interior(alpha, beta)) return false;
        

        // ---------- 第 3 步：命中，填记录单 ----------
        rec.t      = t;
        rec.p      = intersection;
        rec.mat    = mat;
        
        set_hit_normal(r, rec, alpha, beta);
        return true;
    }

    bool bounding_box(AABB& output) const override {
        output = bbox;
        return true;
    }


    // 参数名省略掉：签名保留（子类覆盖时对得上），
    // 但不引入变量，天然没有 -Wunused-parameter 警告
    virtual void set_hit_normal(const Ray& r, HitRecord& rec,
                                double /* alpha */, double /* beta */) const {
        rec.set_face_normal(r, normal);
    }

    // ============================================================
    // TODO(你填 Q10)：内部判定
    //
    //   给定平面内坐标 (a, b)，这个点在平行四边形里吗？
    //
    //   平行四边形：  0 ≤ a ≤ 1  且  0 ≤ b ≤ 1
    //
    //   ★ 换成别的 2D 形状只需要改这个函数：
    //     三角形：  a > 0 && b > 0 && a + b < 1
    //     圆盘：    a*a + b*b < r*r
    //
    //   这就是为什么它被写成 virtual —— 明天做三角形时会覆盖它。
    // ============================================================
    virtual bool is_interior(double a, double b) const {
        return a >= 0.0 && a <= 1.0 && b >= 0.0 && b <= 1.0;

    }

    // ============================================================
    // ★ Day 17 新增：在平行四边形上【均匀】采样一个点，NEE光源采样
    //
    // 为什么要这个东西？
    //   以前光源是靠"随机乱弹碰巧撞上"。NEE 改成"主动朝光源打"，
    //   就需要一个函数能回答：随便给我一个光源上的点吧。
    //
    // pdf 用的是【面积测度】：均匀采样时 pdf = 1 / 面积
    //   （每单位面积的命中概率一样）
    //
    // 均匀采样的做法：α、β 各自 [0,1) 均匀随机
    //   p = Q + α·u + β·v
    //   因为 (α, β) 在单位正方形上均匀，映射到平行四边形后仍然均匀
    //   （平行四边形的面积元 |u×v| 是常数，雅可比行列式恒等于 |u×v|）
    // ============================================================
    void sample_area(Point3& p, Vec3& n, double& pdf) const {
        p   = Q + random_double() * u + random_double() * v;
        n   = normal;
        pdf = (area > 0.0) ? (1.0 / area) : 0.0;
    }

    // 这个面发出的光（材质的 emitted()）
    // 调用材质的emitted（）虚函数，返回发光颜色
    Color emitted_color() const { return mat->emitted(); }

private:
    // 四个角：Q, Q+u, Q+v, Q+u+v
    // 包围盒 = 这两个对角点对各自形成的盒子的【并集】
    void set_bounding_box() {
        AABB diagonal1(Q, Q + u + v);
        AABB diagonal2(Q + u, Q + v);
        bbox = AABB::surrounding(diagonal1, diagonal2);
    }

    Point3 Q;      // 起始角
    Vec3   u, v;   // 两条边向量
    Vec3   w;      // 平面坐标辅助向量 n / (n·n)
    Vec3   normal; // 单位法线
    double D = 0.0; // 平面常数：normal · P = D
    double area = 0.0;  // 面积 |u × v|（Day 17）

    std::shared_ptr<Material> mat;
    AABB bbox;
    bool degenerate = false;//u ∥ v 时法线算不出来，必须禁用 hit()
};
