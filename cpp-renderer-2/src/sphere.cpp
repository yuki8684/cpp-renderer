#include "sphere.h"
#include "ray.h"
#include "vec3.h"
#include <cmath>

// ============================================================
// 球体类的实现
// ============================================================

Sphere::Sphere(Point3 center, double radius) : center(center), radius(radius) {}

// 检测光线与球体的交点
bool Sphere::hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
    Vec3 oc = r.origin() - center;
    double a = dot(r.direction(), r.direction());
    double b = 2.0 * dot(oc, r.direction());
    double c = dot(oc, oc) - radius * radius;
    double discriminant = b * b - 4 * a * c;

    if (discriminant > 0) {
        double temp = (-b - sqrt(discriminant)) / (2.0 * a);
        if (temp < t_max && temp > t_min) {
            rec.t = temp;
            rec.p = r.at(rec.t);
            rec.normal = (rec.p - center) / radius;
            return true;
        }
        temp = (-b + sqrt(discriminant)) / (2.0 * a);
        if (temp < t_max && temp > t_min) {
            rec.t = temp;
            rec.p = r.at(rec.t);
            rec.normal = (rec.p - center) / radius;
            return true;
        }
    }
    return false;
}