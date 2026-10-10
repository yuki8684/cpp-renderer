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

    // --------------------------------------------------------
    // 整堆物体的包围盒 = 所有子物体包围盒的【并集】
    //
    // 做法：从头扫一遍，每次把当前结果和新物体的盒子"合并"成一个更大的盒。
    //       第一个盒子要特殊处理（还不能和任何东西并）。
    //
    // ⚠️ 只要有一个子物体算不出盒子（返回 false），
    //    整堆就算不出——因为漏装一个物体的话，盒子就不"保守"了，
    //    BVH 会错得很难看。
    // --------------------------------------------------------
    bool bounding_box(AABB& output) const override {
        if (objects.empty()) return false;

        AABB temp_box;
        bool first_box = true;

        for (const auto& obj : objects) {
            if (!obj->bounding_box(temp_box)) return false;

            // TODO(你填 B2)：合并盒子
            //   第一个物体：直接拿来当结果
            //   之后的物体：用 AABB::surrounding(现有的 output, 新来的 temp_box)
            //             得到一个"刚好装下两者"的大盒
            output = first_box ? temp_box : AABB::surrounding(output, temp_box);

            first_box = false;
        }
        return true;
    }

    // ⚠️ 这里故意是 public 的。
    //   原因：BVHNode 要拿到这个数组去【排序 + 对半分】。
    //   RTIOW 也是这么做的。代价是破坏了封装（外面可以乱改这个数组），
    //   好处是省事。真要严谨的话可以改成只给 friend 开放。
    std::vector<std::shared_ptr<Hittable>> objects;
};
