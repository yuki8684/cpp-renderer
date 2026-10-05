#pragma once

#include "hittable.h"

// ============================================================
// 一堆物体的集合
//
// 关键设计："一组物体"本身也是一个"物体"（组合模式）。
// 所以 HittableList 继承 Hittable。
// 以后做 BVH 加速时，只要替换这个类，main 一行都不用改。
// ============================================================
class HittableList : public Hittable {
public:
    HittableList() = default;

    void add(std::shared_ptr<Hittable> object) {
        objects.push_back(std::move(object));
    }

    // 遍历所有物体，只保留"最近"的那个命中。
    //
    // 关键点：把已找到的最近距离 closest 当作 t_max 传给下一个物体，
    // 更远的物体会被自动排除 —— 既保证遮挡正确，又省下大量求交计算。
    // 必须遍历完整个列表：一旦提前 return，拿到的是"最后一个"而不是"最近的"。
    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {
        double    closest      = t_max;   // 当前已知的最近命中上限
        bool      hit_anything = false;
        HitRecord temp_rec;               // 临时报告单：只有命中才提交给 rec

        for (const auto& obj : objects) {
            if (obj->hit(r, t_min, closest, temp_rec)) {
                hit_anything = true;
                closest      = temp_rec.t;   // 收紧上限
                rec          = temp_rec;     // 提交结果
            }
        }
        return hit_anything;
    }

private:
    std::vector<std::shared_ptr<Hittable>> objects;
};
