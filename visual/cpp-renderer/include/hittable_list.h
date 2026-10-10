#pragma once

#include "hittable.h"
#include <vector>
#include <memory>

class HittableList : public Hittable {
public:
    HittableList() {}

    HittableList(std::shared_ptr<Hittable> object) {
        add(object);
    }

    void clear() {
        objects.clear();
    }

    void add(std::shared_ptr<Hittable> object) {
        objects.push_back(object);
    }

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

    virtual bool bounding_box(AABB& output) const override {
        if (objects.empty()) return false;

        AABB temp_box;
        bool first_box = true;

        for (const auto& object : objects) {
            if (object->bounding_box(temp_box)) {
                output = first_box ? temp_box : AABB::surrounding(output, temp_box);
                first_box = false;
            }
        }
        return true;
    }

private:
    std::vector<std::shared_ptr<Hittable>> objects;
};