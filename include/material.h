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

// 纹理只需要名字（下面要用 shared_ptr<Texture> 返回它）
class Texture;

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


    //返回材质发出的光，默认不发光，即黑色
    virtual Color emitted() const {return Color(0, 0, 0);}

    // ============================================================
    // ★ Day 17 新增：NEE（光源采样）需要的两个能力查询
    //
    // 为什么不直接在 main 里 dynamic_cast 成 Lambertian？
    //   ① 渲染热路径上不应该有 RTTI 查询（慢）
    //   ② "我能不能做直接光采样"本来就是【材质自己的知识】：
    //      镜面金属和玻璃是 delta 分布（只有一个反射方向），
    //      根本不存在"在光源上采一点"这回事
    //
    // 默认 false / 黑色：纯光源、金属、玻璃都不参与 NEE
    // ============================================================
    // 判断是否是漫反射材质
    virtual bool is_diffuse() const { return false; }
    // 纹理需要 (u,v) 和位置，所以把整张 HitRecord 传进去
    // 在这个交点处，你的漫反射基础颜色（反照率）是什么，基类默认返回黑色
    // /* rec */ 就是在告诉编译器：“我知道有这个参数，但我这里故意不用它，别警告我
    virtual Color diffuse_albedo(const HitRecord& /* rec */) const { return Color(0, 0, 0); }

    // ============================================================
    // ★ Day 29：显式的概率密度函数 pdf
    //
    // 问题：scatter() 填出来的 attenuation 到底代表什么？
    //
    //   正确答案：attenuation = f · cosθ / pdf
    //     f     = BRDF
    //     cosθ  = 散射方向与法线的余弦
    //     pdf   = 【这个采样策略】抽到该方向的概率密度（立体角测度）
    //
    //   以前 pdf 被"藏"在 scatter() 里面 —— 因为余弦采样时
    //   cosθ 和 π 恰好约掉，attenuation 就等于 albedo，看起来像是"不需要 pdf"。
    //   这是一种巧合，不是物理。
    //
    // 为什么必须显式拿出来：
    //   ① 要和别的采样策略合并（MIS）时，你必须能回答
    //      "方向 w 用这套策略被抽到的概率是多大"
    //   ② 换采样策略时不用再手推 attenuation（Day 19 就是手推的）
    //
    // 返回值单位是【单位立体角】，和 pdf_ω 一致。
    //
    // ★ 镜面 / 玻璃返回 0：它们是 δ 分布，只有一个方向，
    //   "概率密度"是无穷大 —— 没有有限的 pdf 可言。
    //   积分器对 pdf == 0 的材质按约定取 cosθ/pdf = 1。
    // ============================================================
    virtual double scattering_pdf(const Ray& /* r_in */, const HitRecord& /* rec */,
                                  const Ray& /* scattered */) const {
        return 0.0;
    }

    // ★ Day 24：这个材质有没有法线贴图？
    //   默认 nullptr —— 绝大多数材质没有，几何那边直接跳过扰动。
    //   几何在命中时调用它，用切线空间把法线"掰"一下，见 Triangle::set_hit_normal()。
    virtual std::shared_ptr<Texture> normal_map() const { return nullptr; }
};
