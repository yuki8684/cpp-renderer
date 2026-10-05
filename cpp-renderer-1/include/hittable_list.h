#pragma once

#include "hittable.h"
#include <vector>

class HittableList : public Hittable {
public:
    HittableList() = default;

    void clear() { objects.clear(); }
    void add(Hittable* object) { objects.push_back(object); }

    virtual bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const override {
        HitRecord temp_rec;
        bool hit_anything = false;
        double closest_so_far = t_max;

        for (const auto& object : objects) {
            if (object->hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }

private:
    std::vector<Hittable*> objects;
};