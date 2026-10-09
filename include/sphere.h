#pragma once

#include "hittable.h"

// ============================================================
// 球体
// ============================================================
class Sphere : public Hittable {
public:
    // --------------------------------------------------------
    // 用初始化列表直接构造成员，避免"先默认构造再赋值"的额外开销
    // --------------------------------------------------------
    Sphere(const Point3& c, double r, std::shared_ptr<Material> m)
        : center(c), radius(r), mat(m) {}
    

    // --------------------------------------------------------
    // 光线与球体求交：把 P(t) = O + tD 代入 |P - C|² = r²，
    // 化为 a·t² + 2·half_b·t + c = 0（half_b = OC·D，即"半个 b"），
    // 取区间 (t_min, t_max) 内最小的根；两个根都不在区间内即未命中。
    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {
        const Vec3   oc     = r.origin() - center;
        const double a      = dot(r.direction(), r.direction());
        const double half_b = dot(oc, r.direction());
        const double c      = dot(oc, oc) - radius * radius;

        const double discriminant = half_b * half_b - a * c;
        if (discriminant < 0.0) return false;      // 光线掠过球外

        const double sqrt_d = std::sqrt(discriminant);

        // 先试较小的根；它若在相机背后或在区间外，再试较大的根
        //（相机位于球内时较小的根为负，此时正确解是较大的根）
        double root = (-half_b - sqrt_d) / a;
        if (root <= t_min || root >= t_max) {
            root = (-half_b + sqrt_d) / a;
            if (root <= t_min || root >= t_max) return false;
        }

        rec.t      = root;
        rec.p      = r.at(root);
        rec.set_face_normal(r, (rec.p - center) / radius);    // |P - C| = radius，除完即单位向量
        rec.mat    = mat;                          // 交代"我是什么材质"
        return true;
             
    }

    // --------------------------------------------------------
    // 球的包围盒：最简单的一种 —— 球心 ± 半径
    //
    // 因为球在三个方向上一样宽，所以盒子是个【立方体】。
    // 而且这个盒子是"紧"的：球正好内切于盒子，一点不浪费。
    // --------------------------------------------------------
    bool bounding_box(AABB& output) const override {
        // TODO(你填 B1)：用【球心 − 半径】和【球心 + 半径】两个对角点构造
        //   提示：三个坐标轴上的偏移量都是同一个 radius
        //   构造 Vec3(r, r, r) 就行（Point3 和 Vec3 是同一个类）
        output = AABB(center - Vec3(radius, radius, radius),
                      center + Vec3(radius, radius, radius));
        return true;
    }

private:
    // ⚠️ 成员的【声明顺序】决定了实际初始化顺序，
    //    必须和构造函数初始化列表的顺序一致，否则编译器报 [-Wreorder]
    Point3 center;
    double radius;
    std::shared_ptr<Material> mat;   // 这个球用什么材质
};
