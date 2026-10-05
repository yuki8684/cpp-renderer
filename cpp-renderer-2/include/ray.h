#pragma once

#include "vec3.h"

// 光线类，用于表示光线的起点和方向
class Ray {
public:
    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction)
        : orig(origin), dir(direction) {}

    Point3 origin() const { return orig; }
    Vec3 direction() const { return dir; }

    // 根据参数 t 计算光线上的点
    Point3 at(double t) const {
        return orig + t * dir;
    }

private:
    Point3 orig; // 光线的起点
    Vec3 dir;    // 光线的方向
};