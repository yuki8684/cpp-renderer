#pragma once

#include "material.h"
#include "hittable.h"   // ← 这里需要 HitRecord 的【完整定义】，
                        //   因为下面要读 rec.normal 和 rec.p
#include "texture.h"    // Day 22：albedo 可能是一张贴图

#include <memory>

// ============================================================
// Lambertian：哑光材质（石膏、纸、未上漆的木头、墙）
//
// 特征：光线打上去朝四面八方散射，所以从任何角度看颜色都差不多。
// ============================================================
class Lambertian : public Material {
public:
    // Day 22：两种构造方式
    //   ① 纯色      Lambertian(Color(0.5,0.5,0.5))    ← 自动包成 SolidColor
    //   ② 贴图      Lambertian(tex)                  ← 任意 Texture
    // 这样老代码一行都不用改。
    Lambertian(const Color& albedo)
        : tex(std::make_shared<SolidColor>(albedo)) {}

    Lambertian(std::shared_ptr<Texture> t) : tex(t) {}

    // 参数名省略：哑光散射不看入射方向（金属需要看，所以 Metal 那里留着）
    bool scatter(const Ray&, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override {

        // ★ Day 22：每一处用到 albedo 的地方都改成 tex->value(rec.u, rec.v, rec.p)
        //   —— 同一个材质就能在不同位置反射不同颜色（就是“贴图”）。
        const Color albedo = tex->value(rec.u, rec.v, rec.p);

        // ============================================================
        // ★ Day 29：attenuation 现在【只是 BRDF】= albedo / π
        //
        //   Day 19 时它代表 f·cosθ/pdf，pdf 被"藏"在这个函数里 ——
        //   因为余弦采样时 cosθ 和 π 恰好约掉，看起来像是"不需要 pdf"。
        //   那只是巧合：换到均匀半球分支就必须手推 2·cosθ·albedo（Day 19 你推过）。
        //
        //   现在 cosθ/pdf 统一由 ray_color() 补上：
        //       余弦采样   (albedo/π) · cosθ / (cosθ/π)   = albedo
        //       均匀半球   (albedo/π) · cosθ / (1/(2π))   = 2·cosθ·albedo
        //
        //   ★ 于是两个分支的 attenuation【完全相同】，只剩采样方式不同。
        //     这就是显式化最大的好处：attenuation 和采样策略解耦了。
        // ============================================================
        attenuation = albedo / pi;

        if (g_uniform_hemisphere) {
            // 分支 A：均匀半球采样（pdf = 1/(2π)，见下面的 scattering_pdf）
            scattered = Ray(rec.p, random_on_hemisphere(rec.normal));
            return true;
        }

        // 分支 B：余弦加权采样（默认，pdf = cosθ/π，见下面的 scattering_pdf）
        //   ⚠️ rec.normal + random_unit_vector() 不是单位向量，
        //      所以 scattering_pdf 里必须先 unit_vector 再点乘。
        scattered = Ray(rec.p, rec.normal + random_unit_vector());
        return true;
    }

    // ============================================================
    // ★ Day 29：显式 PDF —— 「这个方向被当前采样策略抽到的概率密度」
    //
    //   单位是【单位立体角】，和 pdf_ω 一致。
    //
    //   ★ 镜面 / 玻璃不实现它（用 Material 基类的默认值 0）：
    //     它们是 δ 分布，只有一个方向，概率密度无穷大 —— 没有有限的 pdf 可言。
    //     积分器对 pdf == 0 的材质按约定取 cosθ/pdf = 1。
    //
    //   ★ 自检恒等式：∫ pdf(ω) dω == 1（在半球上，dω = sinθ dθ dφ）
    //     测试 tests/pdf_test.cpp 的 U3 / U5 就是在数值验证这一条。
    // ============================================================
    double scattering_pdf(const Ray&, const HitRecord& rec,
                          const Ray& scattered) const override {
        const double cos_t = dot(unit_vector(scattered.direction()), rec.normal);
        if (cos_t <= 0.0) return 0.0;      // 散射到法线背面 → 概率为 0

        if (g_uniform_hemisphere) {
            // 均匀铺满整个半球：半球总立体角 = 2π → 概率密度 = 1/(2π)
            return 1.0 / (2.0 * pi);
        }
        // 余弦加权：密度正比于 cosθ，归一化后就是 cosθ/π
        return cos_t / pi;
    }

    // ★ Day 17：哑光是漫反射，可以做 NEE（主动朝光源采样）
    bool is_diffuse() const override { return true; }
    Color diffuse_albedo(const HitRecord& rec) const override {
        return tex->value(rec.u, rec.v, rec.p);
    }

private:
    // Day 22：从“一个颜色”变成“一张纹理”
    //   纯色只是纹理的一个特例（SolidColor），所以代码路径只有一条。
    std::shared_ptr<Texture> tex;
};
