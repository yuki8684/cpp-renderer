#pragma once

#include "rtweekend.h"
#include "vec3.h"
#include "ray.h"

#include <utility>    // std::swap

// ============================================================
// AABB = Axis-Aligned Bounding Box（轴对齐包围盒）
//
// 一个"六个面都垂直于 x / y / z 轴"的长方体。
// 所以它只需要两个点就能确定：
//
//     minimum = 三个坐标都取最小的那个角
//     maximum = 三个坐标都取最大的那个角
//
// 为什么叫"轴对齐"？
//   因为面垂直于坐标轴，光线和它求交时【三个轴可以分开算】——
//   每个轴退化成一维问题（光线和一个线段求交），最后取交集。
//   如果盒子能任意旋转，就必须先把光线变换到盒子的坐标系，麻烦得多。
//
// 为什么需要它？
//   这是 BVH 的地基：先用一个"便宜的"盒子测试排除掉一大批物体，
//   只对可能命中的少数物体做昂贵的精确求交。
// ============================================================
class AABB {
public:
    AABB() {}

    // --------------------------------------------------------
    // 用【任意两个对角点】构造。
    // 不要求 a 是"最小"那个、b 是"最大"那个 —— 逐轴取 min/max 会自动纠正。
    // --------------------------------------------------------
    AABB(const Point3& a, const Point3& b) {
        minimum = Point3(std::fmin(a.x(), b.x()),
                         std::fmin(a.y(), b.y()),
                         std::fmin(a.z(), b.z()));
        maximum = Point3(std::fmax(a.x(), b.x()),
                         std::fmax(a.y(), b.y()),
                         std::fmax(a.z(), b.z()));
    }

    // --------------------------------------------------------
    // 把两个盒子合并成一个"刚好能同时装下两者"的盒子。
    // 用途：HittableList 把所有子物体的包围盒并成一个大盒。
    // --------------------------------------------------------
    static AABB surrounding(const AABB& box0, const AABB& box1) {
        Point3 small(std::fmin(box0.minimum.x(), box1.minimum.x()),
                     std::fmin(box0.minimum.y(), box1.minimum.y()),
                     std::fmin(box0.minimum.z(), box1.minimum.z()));

        Point3 big(std::fmax(box0.maximum.x(), box1.maximum.x()),
                   std::fmax(box0.maximum.y(), box1.maximum.y()),
                   std::fmax(box0.maximum.z(), box1.maximum.z()));

        return AABB(small, big);
    }

    // ========================================================
    // ★ 今天的核心：光线和 AABB 求交（slab method / 板法）
    //
    // 光线 r 在参数区间 (t_min, t_max) 内，是否打到这个盒子？
    //
    // 思路：
    //   盒子 = 三个"板"(slab) 的交集
    //     x 板：x ∈ [minimum.x, maximum.x]
    //     y 板：y ∈ [minimum.y, maximum.y]
    //     z 板：z ∈ [minimum.z, maximum.z]
    //
    //   每个板会在 t 轴上给出一段区间 [t0, t1]
    //   三个区间【取交集】，交集非空就是命中。
    //
    //   交集的下界 = max(所有 t0)   ← "必须晚于最晚的进入时刻"
    //   交集的上界 = min(所有 t1)   ← "必须早于最早的离开时刻"
    //
    // ⚠️ 除零陷阱：方向分量 D 可能是 0。
    //    不要特判！IEEE 754 里 1.0/0.0 = +infinity（不是崩溃），
    //    而 (min-O)*inf 的符号恰好自动给出"永远在板内"或"永远在板外"
    //    这两种正确结果。详见 docs/devlog.md Day 8。
    // ========================================================
    bool hit(const Ray& r, double t_min, double t_max) const {
        for (int axis = 0; axis < 3; ++axis) {

            // TODO(你填 A1)：这条轴上的"方向分量的倒数"
            //   为什么要先算倒数？因为下面两个式子都要除以 D，
            //   算出 1/D 后做乘法，一次除法代替两次。
            double inv_d = 1.0 / r.direction()[axis];



            // TODO(你填 A2)：光线"进入板"和"离开板"的参数 t
            //   记住：min/max 减去的是光线【起点】的该轴坐标，
            //   光线起点的某个轴坐标 = r.origin()[axis]
            //   光线方向的某个轴坐标 = r.direction()[axis]
            //   盒子的两个面 = min_axis(axis) / max_axis(axis)
            double t0 = (min_axis(axis) - r.origin()[axis]) * inv_d;
            double t1 = (max_axis(axis) - r.origin()[axis]) * inv_d;


            // TODO(你填 A3)：如果方向分量为负，t0 和 t1 的顺序反了，交换回来。
            //   交换后保证 t0 ≤ t1（这就是"规范化区间"）
            if (inv_d < 0.0) {
                std::swap(t0, t1);
            }


            // TODO(你填 A4)：把 [t0, t1] 和 [t_min, t_max] 求交集
            //   交集的下界 = 两者【较大】的那个
            //   交集的上界 = 两者【较小】的那个
            t_min = std::fmax(t0, t_min);
            t_max = std::fmin(t1, t_max);


            // TODO(你填 A5)：如果交集已经空了，立刻返回"没打到"
            //   注意判断条件用 "≤" 还是 "<" 会有边界差异，
            //   我们用 "≤"：刚好擦过（退化成单个点）算作没命中。
            if (t_min >= t_max) return false;
        }

        return true;
    }

    // 第 axis 轴（0=x, 1=y, 2=z）的两个面
    double min_axis(int axis) const { return minimum[axis]; }
    double max_axis(int axis) const { return maximum[axis]; }

    Point3 minimum;
    Point3 maximum;
};
