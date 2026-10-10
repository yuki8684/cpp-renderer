#pragma once

#include "hittable.h"
#include "hittable_list.h"
#include "aabb.h"

#include <algorithm>   // std::sort

// ============================================================
// BVH = Bounding Volume Hierarchy（包围盒层次结构）
//
// 一句话：把"一堆物体"递归地分成两堆、两堆再分两堆……
//         每一堆都用一个包围盒框住，形成一棵二叉树。
//
//                       根节点（全世界的大盒）
//                    ┌──────────┴──────────┐
//             左半堆的盒                 右半堆的盒
//           ┌────┴────┐              ┌────┴────┐
//        左左        左右          右左        右右
//         │                                  │
//       叶子（几个球）                     叶子（几个球）
//
// 求交时从根往下：
//   先试大盒子 —— 没中就直接跳过【整个子树】（可能省掉几百个物体）
//   中了才往下走，直到叶子
//
// 复杂度：O(N) → O(log N)
//
// ⚠️ 它继承 Hittable！所以从外面看，"一棵 BVH 树"和"一个球"没有区别 ——
//    main.cpp 里只要把 HittableList 换成 BVHNode，其他一行都不用改。
//    （这就是 Day 2 那个"组合模式"设计埋下的伏笔）
// ============================================================
class BVHNode : public Hittable {
public:
    // --------------------------------------------------------
    // 从"物体列表"建树。
    //
    // ⚠️ 参数是【按值】传的（会拷贝一份）—— 因为建树过程要排序数组，
    //    不想把调用方的列表弄乱。
    //    拷贝成本很低：里面存的是 shared_ptr，只是引用计数 +1。
    // --------------------------------------------------------
    BVHNode(HittableList list)
        : BVHNode(list.objects, 0, list.objects.size()) {}

    // --------------------------------------------------------
    // 核心构造函数：给 objects 数组的 [start, end) 区间建一棵子树
    // --------------------------------------------------------
    BVHNode(std::vector<std::shared_ptr<Hittable>>& objects,
            size_t start, size_t end) {

        // ========== 第 1 步：算出这一堆物体的总包围盒 ==========
        AABB total_box;
        bool first = true;
        for (size_t i = start; i < end; ++i) {
            AABB temp;
            objects[i]->bounding_box(temp);

            // TODO(你填 V1)：把 temp 并进 total_box
            //   和昨天 HittableList::bounding_box 里【一模一样】的模式：
            //     第一个直接赋值，之后的求并集
            total_box = first ? temp : AABB::surrounding(total_box, temp);
            first = false;
        }
        this->box = total_box;   // 存下来，求交时要用

        // ========== 第 2 步：选最长的那条轴 ==========
        // TODO(你填 V2)：调用 AABB 的哪个函数能拿到"最长轴"的编号？
        int axis = total_box.longest_axis();   // 0 = x, 1 = y, 2 = z

        // ========== 第 3 步：沿这条轴排序 ==========
        // 这个 lambda 把 axis 【捕获】进来：
        //   方括号里写 axis，表示"把外面那个轴编号复制进来一份给我用"
        auto comparator = [axis](const std::shared_ptr<Hittable>& a,
                                 const std::shared_ptr<Hittable>& b) {
            AABB box_a, box_b;
            a->bounding_box(box_a);
            b->bounding_box(box_b);
            return box_a.minimum[axis] < box_b.minimum[axis];   // 按该轴的最小坐标比大小
        };

        size_t span = end - start;   // 这一堆里有几个物体

        // ========== 第 4 步：递归 ==========
        if (span == 1) {
            // 只剩 1 个物体 → 左右都指向它（叶子）
            left  = right = objects[start];
        } else if (span == 2) {
            // 剩 2 个物体 → 一人一个（叶子）
            left  = objects[start];
            right = objects[start + 1];
        } else {
            // 3 个以上 → 排序、对半分、递归

            // TODO(你填 V3)：沿 axis 轴排序 [start, end) 这一段
            //   std::sort(起点, 终点, 比较函数)
            //   起点 = objects.begin() + start
            //   终点 = objects.begin() + end
            std::sort(objects.begin() + start, objects.begin() + end, comparator);

            // TODO(你填 V4)：对半分的位置
            //   要把 [start, end) 分成 [start, mid) 和 [mid, end)
            //   提示：物体数的一半，再加上起点 start
            size_t mid = start + (end - start) / 2;

            // TODO(你填 V5)：递归建左右子树
            //   子节点类型是 shared_ptr<Hittable>，所以用
            //     std::make_shared<BVHNode>(objects, ______, ______)
            //   左子树的区间是 [start, mid)，右子树是 [mid, end)
            left  = std::make_shared<BVHNode>(objects, start, mid);
            right = std::make_shared<BVHNode>(objects, mid, end);
        }
    }

    // --------------------------------------------------------
    // 求交：BVH 全部的速度都来自这里
    //
    // 流程：
    //   ① 先试【自己这个大盒子】—— 没中就把整棵子树跳过
    //   ② 问左子树
    //   ③ 问右子树（但把 t_max 收紧到左边的命中距离）
    // --------------------------------------------------------
    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {
        // TODO(你填 V6)：先试自己这个大盒子
        //   没打中 → 整棵子树都不可能有交点，直接返回 false
        //   提示：AABB::hit(r, t_min, t_max) 返回 bool
        if (!box.hit(r, t_min, t_max)) return false;

        // ② 问左子树
        bool hit_left = left->hit(r, t_min, t_max, rec);

        // TODO(你填 V7)：问右子树，但 t_max 要"收紧"
        //
        //   为什么：如果左子树已经命中（距离 = rec.t），
        //   那右子树只需要找【比 rec.t 更近】的交点 ——
        //   更远的都会被左边挡住，没必要考虑。
        //
        //   所以 t_max 要传：
        //      左边命中了  →  rec.t     （只找更近的）
        //      左边没命中  →  t_max     （保持原样）
        //
        //   又是那个三目运算符：  条件 ? 甲 : 乙
        bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

        return hit_left || hit_right;
    }

    // --------------------------------------------------------
    // 自己这棵子树的包围盒，就是建树时算好的那个 box
    // （直接返回，不用重算 —— 这是建树时"顺手记下来"的回报）
    // --------------------------------------------------------
    bool bounding_box(AABB& output) const override {
        output = box;
        return true;
    }

private:
    std::shared_ptr<Hittable> left;
    std::shared_ptr<Hittable> right;
    AABB box;
};
