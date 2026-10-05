#pragma once

#include "rtweekend.h"
#include "ray.h"
#include "vec3.h"

// ============================================================
// 前向声明（forward declaration）
//
// 我们只需要"告诉编译器有这么个类型"，不需要给出它的内容。
// 为什么？因为下面 scatter() 的参数是 const HitRecord&（引用），
// 引用只需要知道类型名，不需要知道它长什么样。
//
// 这样就避免了循环包含：
//   material.h 需要 HitRecord
//   hittable.h 里的 HitRecord 需要 Material
//   如果互相 #include 就死循环了。
// ============================================================
struct HitRecord;

// ============================================================
// 材质抽象基类
//
// 任何材质都必须回答两个问题：
//   ① 光线打在你身上，会往哪儿弹？   -> 填 scattered
//   ② 回来的时候打几折？             -> 填 attenuation
//
// 这个类本身不能被实例化（有纯虚函数 = 0）。
// 具体的材质（Lambertian / Metal / Dielectric）各自继承它。
// ============================================================
class Material {
public:
    virtual ~Material() = default;

    // 输入：r_in 射进来的光线，rec 命中信息（交点位置、法线）
    // 输出：attenuation 反射率（打几折，是个颜色）
    //       scattered   弹射出去的光线
    // 返回：这次有没有发生散射（返回 false 表示光被吸收了）
    virtual bool scatter(const Ray& r_in, const HitRecord& rec,
                         Color& attenuation, Ray& scattered) const = 0;
};
