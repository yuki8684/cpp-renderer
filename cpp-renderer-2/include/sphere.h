#pragma once

#include "hittable.h"
#include "vec3.h"

class Sphere : public Hittable {
public:
    Sphere() = default;
    Sphere(Point3 center, double radius) : center(center), radius(radius) {}

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override;

private:
    Point3 center;
    double radius;
};