#pragma once

#include "rtweekend.h"
#include "ray.h"
#include "vec3.h"

struct HitRecord {
    Point3 p;      // 交点位置
    Vec3   normal; // 交点处的法线（必须是单位向量！）
    double t;      // 交点对应的光线参数
};

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray& r, double t_min, double t_max,
                     HitRecord& rec) const = 0;
};