#pragma once

#include "hittable.h"
#include "hittable_list.h"
#include "aabb.h"

#include <algorithm>   // std::sort

class BVHNode : public Hittable {
public:
    BVHNode(HittableList list)
        : BVHNode(list.objects, 0, list.objects.size()) {}

    BVHNode(std::vector<std::shared_ptr<Hittable>>& objects,
            size_t start, size_t end) {

        AABB total_box;
        bool first = true;
        for (size_t i = start; i < end; ++i) {
            AABB temp;
            objects[i]->bounding_box(temp);
            total_box = first ? temp : AABB::surrounding(total_box, temp);
            first = false;
        }
        this->box = total_box;

        int axis = total_box.longest_axis();

        auto comparator = [axis](const std::shared_ptr<Hittable>& a,
                                 const std::shared_ptr<Hittable>& b) {
            AABB box_a, box_b;
            a->bounding_box(box_a);
            b->bounding_box(box_b);
            return box_a.minimum[axis] < box_b.minimum[axis];
        };

        size_t span = end - start;

        if (span == 1) {
            left  = right = objects[start];
        } else if (span == 2) {
            left  = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, comparator);
            size_t mid = start + (end - start) / 2;
            left  = std::make_shared<BVHNode>(objects, start, mid);
            right = std::make_shared<BVHNode>(objects, mid, end);
        }
    }

    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {
        if (!box.hit(r, t_min, t_max)) return false;

        bool hit_left = left->hit(r, t_min, t_max, rec);
        bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

        return hit_left || hit_right;
    }

    bool bounding_box(AABB& output) const override {
        output = box;
        return true;
    }

private:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB box;
};