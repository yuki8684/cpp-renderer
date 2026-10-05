#pragma once

#include "vec3.h"

// ============================================================
// 材质类：表示物体的材质属性
// 包含光照、反射等相关功能
// ============================================================
class Material {
public:
    virtual ~Material() = default;

    // 计算光线与材质交互的颜色
    virtual bool scatter(const Ray& r_in, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const = 0;
};