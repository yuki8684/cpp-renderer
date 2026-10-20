#pragma once

#include "material.h"

// ============================================================
// DiffuseLight：纯光源（面积光）
//
// "面积光"的意思是：它是一块有面积的发光面，不是一个点。
// 这一点决定了阴影是【软】的 —— 后面的 Cornell Box 就靠它。
//
// 它只发光，不散射。
// ============================================================
class DiffuseLight : public Material {
public:
    //光源颜色
    DiffuseLight(const Color& emit) : emit(emit) {}

    // TODO(T2-1)：光源不散射，直接返回 false
    //   参数名全部省略 —— 反正一个都用不到，也不会有 unused 警告
    bool scatter(const Ray&, const HitRecord&, Color&, Ray&) const override {
        return false;
    }

    // TODO(T2-2)：返回构造时存下来的自发光颜色
    Color emitted() const override {
        return emit;
    }

private:
    Color emit;
};