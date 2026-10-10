#pragma once

#include "ray.h"
#include "aabb.h"

class HitRecord;

class Hittable {
public:
    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const = 0;
    virtual bool bounding_box(AABB& output) const = 0;
};