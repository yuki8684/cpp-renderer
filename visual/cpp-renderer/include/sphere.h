#pragma once

#include "hittable.h"
#include "vec3.h"
#include <memory>

class Sphere : public Hittable {
public:
    Sphere() = default;
    Sphere(Vec3 center, double radius, std::shared_ptr<Material> material)
        : center(center), radius(radius), material(material) {}

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        Vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;
        auto discriminant = half_b * half_b - a * c;

        if (discriminant < 0) {
            return false;
        }
        auto sqrt_d = sqrt(discriminant);
        auto root = (-half_b - sqrt_d) / a;
        if (root < t_min || root > t_max) {
            root = (-half_b + sqrt_d) / a;
            if (root < t_min || root > t_max) {
                return false;
            }
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.material = material;

        return true;
    }

    virtual bool bounding_box(AABB& output) const override {
        output = AABB(center - Vec3(radius, radius, radius), center + Vec3(radius, radius, radius));
        return true;
    }

private:
    Vec3 center;
    double radius;
    std::shared_ptr<Material> material;
};