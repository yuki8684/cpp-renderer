#pragma once

#include "quad.h"

// ============================================================
// Triangle：三角形
//
// 和 Quad 的唯一区别就是"内部判定"。
// 所以它【继承】Quad，不重写 hit()、不重写 bounding_box()、
// 也不重写平面参数的计算 —— 只覆盖 is_interior 这一个虚函数。
//
// 顶点约定：v0 = a, v1 = b, v2 = c
//   Quad 的参数化是 P = a + α·u + β·v
//   要让 P 能表示三角形内部，必须让 u、v 分别指向另外两个顶点
// ============================================================
class Triangle : public Quad {
public:
    Triangle(const Point3& a, const Point3& b, const Point3& c,
             std::shared_ptr<Material> mat)
        : Quad(a, b - a, c - a, mat) {   // TODO(T1-1)：两条边向量
    }

    // TODO(T1-2)：内部判定（三角形版）
    //   条件： α ≥ 0  且  β ≥ 0  且  α + β ≤ 1
    //   ⚠️ 用 >= / <= （闭集），不要用 > / <
    //      理由见下面「为什么用闭集」——这是今天最值得想清楚的一点
    bool is_interior(double alpha, double beta) const override {
        return alpha >= 0 && beta >= 0 && alpha + beta <= 1;
    }
};