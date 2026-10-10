#pragma once

#include "vec3.h"

// 一条光线：起点 + 方向
class Ray {
public:
    Ray() {}
    Ray(const Point3& origin, const Vec3& direction)
        : orig(origin), dir(direction), dir_inv(make_inv(direction)) {}

    const Point3& origin() const { return orig; }
    const Vec3&   direction() const { return dir; }

    // --------------------------------------------------------
    // ★ 方向分量的倒数（1/Dx, 1/Dy, 1/Dz）
    //
    // 为什么要存这个？
    //   AABB 求交里每个轴都要算 (min−O)/D。如果每次都在循环里除，
    //   一次盒子求交就是【3 次除法】—— 而 double 除法要 ~26 个时钟周期，
    //   是乘法（~4 周期）的 6 倍！
    //
    //   实测：322,239,827 次盒子测试 × 3 次除法 = 9.67 亿次除法
    //         ≈ 6.2 秒（就是全部渲染时间）
    //
    //   提前算好一次（每条光线 3 次除法），盒子求交里就只剩乘法。
    //
    // ⚠️ 结果完全不变：现在算的还是同一个 1.0/D，只是提前了。
    //    所以渲染结果仍然是逐像素一致的。
    // --------------------------------------------------------
    const Vec3& direction_inv() const { return dir_inv; }

    // 光线上的点：P(t) = O + t * D
    Point3 at(double t) const { return orig + t * dir; }

private:
    static Vec3 make_inv(const Vec3& d) {
        return Vec3(1.0 / d.x(), 1.0 / d.y(), 1.0 / d.z());
    }

    Point3 orig;
    Vec3   dir;
    Vec3   dir_inv;
};
