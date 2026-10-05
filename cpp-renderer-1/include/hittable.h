#pragma once

#include "rtweekend.h"
#include "ray.h"
#include "vec3.h"

struct HitRecord {
    Point3 p;      // Intersection point
    Vec3   normal; // Normal at the intersection point (must be a unit vector!)
    double t;      // Parameter corresponding to the intersection point
};

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray& r, double t_min, double t_max,
                     HitRecord& rec) const = 0;
};