#pragma once

#include "hittable.h"
#include "hittable_list.h"
#include "aabb.h"

#include <algorithm>   // std::sort

// ============================================================
// BVH = Bounding Volume Hierarchy（包围盒层次结构）
//        —— 扁平化版本（Day 10 重写）
//
// 一句话：把"一堆物体"递归地分成两堆、两堆再分两堆……
//         每一堆都用一个包围盒框住，形成一棵二叉树。
//
//                       根节点（全世界的大盒）
//                    ┌──────────┴──────────┐
//             左半堆的盒                 右半堆的盒
//           ┌────┴────┐              ┌────┴────┐
//        左左        左右          右左        右右
//
// 求交时从根往下：先试大盒子 —— 没中就直接跳过【整个子树】。
//
// ------------------------------------------------------------
// ⚠️ Day 9 的版本用 shared_ptr 把节点串成一棵树，算法正确，
//    但实测每次节点访问要 ~75 个 CPU 周期，其中：
//        虚函数调用 + 分支预测失败   ~40 周期
//        指针跳转带来的缓存未命中     ~20 周期
//        盒子测试本身的算术           ~15 周期
//
//    这个版本把三个问题一起解决：
//      ① 所有节点存进【一个连续数组】，用整数【下标】互相引用，不用指针
//      ② 去掉每个节点的虚函数调用，用 if (是不是叶子) 代替
//      ③ 节点压到 64 字节 —— 正好一条缓存线，一次访存读全
// ------------------------------------------------------------
//
// ⚠️ 它继承 Hittable！所以从外面看，"一棵 BVH 树"和"一个球"没有区别 ——
//    main.cpp 里只要把 HittableList 换成 BVH，其他一行都不用改。
// ============================================================
class BVH : public Hittable {
public:
    // --------------------------------------------------------
    // 节点：内部节点和叶子节点【共用同一个结构体】
    //
    //   内部节点（count == 0）：left / right = 两个子节点的【数组下标】
    //   叶子节点（count > 0）：left / right = 物体的【数组下标】
    //                          （count == 2 时两个都有效；
    //                            count == 1 时 right 等于 left）
    //
    // 内存布局（关键！）：
    //   AABB          = 2 个 Point3 = 6 个 double = 48 字节
    //   left/right/count = 3 个 int              = 12 字节
    //                                    合计     = 60 字节
    //   按 8 字节对齐后 → 64 字节 = 正好一条缓存线 ✓
    // --------------------------------------------------------
    struct Node {
        AABB box;      // 48
        int  left;     //  4
        int  right;    //  4
        int  count;    //  4   0 = 内部节点
    };

    // --------------------------------------------------------
    // 建树。
    //
    // ⚠️ 参数按值传（故意拷贝一份）—— 建树要排序，不想弄乱调用方的列表。
    //    这里用 std::move 把拷贝来的 vector 直接"搬"成成员，不多拷贝一次。
    // --------------------------------------------------------
    BVH(HittableList list) : objects(std::move(list.objects)) {
        if (objects.empty()) return;

        // 预留空间：N 个物体最多产生 2N 个节点。
        // ⚠️ 提前 reserve 很重要 —— 否则递归过程中 vector 反复扩容搬家。
        //    （我们用下标寻址，所以扩容不会导致错误，但会多很多次内存拷贝）
        nodes.reserve(objects.size() * 2);

        build(0, objects.size());
    }

    // ========================================================
    // ★ 求交：迭代遍历（显式栈），不再递归、不再有虚函数调用
    // ========================================================
    bool hit(const Ray& r, double t_min, double t_max,
             HitRecord& rec) const override {
        if (nodes.empty()) return false;

        bool hit_anything = false;

        // 显式栈：树深度是 O(log N)，481 个物体最多 ~10 层，
        // 每层最多压 2 个 → 64 个格子绰绰有余。
        int stack[64];
        int sp = 0;
        stack[sp++] = 0;              // 从根节点（下标 0）开始

        while (sp > 0) {
            const Node& node = nodes[stack[--sp]];

            // --------------------------------------------------
            // ① 先试这个节点的盒子
            //
            // TODO(你填 F1)：盒子没打中 → 跳过【这一个】节点
            //
            // ⚠️ 这里必须用 continue 而不是 return false！
            //    return false 会把栈里还没处理完的节点全部丢掉 ✗
            // --------------------------------------------------
            if (!node.box.hit(r, t_min, t_max)) continue;

            if (node.count == 0) {
                // ==============================================
                // ② 内部节点：把两个子节点推进栈
                //
                // TODO(你填 F4)：推进 node.left 和 node.right
                //   写法：stack[sp++] = _____;
                // ==============================================
                stack[sp++] = node.left;
                stack[sp++] = node.right;

            } else {
                // ==============================================
                // ③ 叶子节点：把里面的物体挨个测一遍
                // ==============================================
                for (int k = 0; k < node.count; ++k) {
                    // count == 1 时 left 和 right 相同，取哪个都行
                    int obj = (k == 0) ? node.left : node.right;

                    // TODO(你填 F2)：测这个物体是否命中
                    //     objects[obj]->hit(r, t_min, t_max, rec)  返回 bool
                    if (objects[obj]->hit(r, t_min, t_max, rec)) {
                        // TODO(你填 F3)：命中之后做两件事
                        //   ① 记录"至少中了一个"
                        //   ② 收紧 t_max —— 只找比这次更近的（距离在 rec.t 里）
                        hit_anything = true;
                        t_max = rec.t;
                    }
                }
            }
        }

        return hit_anything;
    }

    // --------------------------------------------------------
    // 整棵树的包围盒 = 根节点的盒子（建树时算好的）
    // --------------------------------------------------------
    bool bounding_box(AABB& output) const override {
        if (nodes.empty()) return false;
        output = nodes[0].box;
        return true;
    }

private:
    // ========================================================
    // 递归建树：给 objects[start, end) 这一段建一棵子树，
    // 返回【新节点在 nodes 数组里的下标】。
    //
    // 算法和 Day 9 完全一样（算盒子 → 选最长轴 → 排序 → 对半分），
    // 只是把"造新对象"换成"往数组里追加一个节点"。
    // ========================================================
    int build(size_t start, size_t end) {
        // 先占一个位置（内容稍后再填）
        int index = (int)nodes.size();
        nodes.emplace_back();

        // ---------- 第 1 步：算这一堆物体的总包围盒 ----------
        AABB total_box;
        bool first = true;
        for (size_t i = start; i < end; ++i) {
            AABB temp;
            objects[i]->bounding_box(temp);
            total_box = first ? temp : AABB::surrounding(total_box, temp);
            first = false;
        }
        nodes[index].box = total_box;

        size_t span = end - start;

        // ---------- 递归出口：1~2 个物体 → 叶子 ----------
        if (span <= 2) {
            nodes[index].count = (int)span;
            nodes[index].left  = (int)start;
            nodes[index].right = (span == 2) ? (int)(start + 1) : (int)start;
            return index;
        }

        // ---------- 第 2 步：选最长轴 ----------
        int axis = total_box.longest_axis();

        // ---------- 第 3 步：沿这条轴排序 ----------
        std::sort(objects.begin() + start, objects.begin() + end,
                  [axis](const std::shared_ptr<Hittable>& a,
                         const std::shared_ptr<Hittable>& b) {
                      AABB box_a, box_b;
                      a->bounding_box(box_a);
                      b->bounding_box(box_b);
                      return box_a.minimum[axis] < box_b.minimum[axis];
                  });

        // ---------- 第 4 步：对半分，递归 ----------
        size_t mid = start + span / 2;

        // ⚠️ 注意：递归之后 nodes 可能已经扩容（搬过家）了，
        //    所以【绝对不能用引用或指针】记住 nodes[index]，
        //    必须每次都用 nodes[index] 重新寻址。
        int left_index  = build(start, mid);
        int right_index = build(mid,   end);

        nodes[index].count = 0;                 // 0 = 内部节点
        nodes[index].left  = left_index;
        nodes[index].right = right_index;
        return index;
    }

    // ============ 数据 ============
    std::vector<Node> nodes;                        // 所有节点，连续排列
    std::vector<std::shared_ptr<Hittable>> objects; // 所有物体（叶子按【下标】引用）
};
