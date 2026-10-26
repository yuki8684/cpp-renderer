#pragma once

#include "material.h"
#include "hittable.h"   // ← 这里需要 HitRecord 的【完整定义】，
                        //   因为下面要读 rec.normal 和 rec.p

// ============================================================
// Lambertian：哑光材质（石膏、纸、未上漆的木头、墙）
//
// 特征：光线打上去朝四面八方散射，所以从任何角度看颜色都差不多。
// ============================================================
class Lambertian : public Material {
public:
    Lambertian(const Color& albedo) : albedo(albedo) {}

    // 参数名省略：哑光散射不看入射方向（金属需要看，所以 Metal 那里留着）
    bool scatter(const Ray&, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override {

        // ============================================================
        // ★ Day 19：pdf 补偿 —— 你自己推
        //
        //   attenuation 这个变量【物理上代表什么】？
        //
        //       attenuation = f · cosθ / pdf
        //
        //   其中：
        //       f     = albedo / π        （Lambertian 的 BRDF）
        //       cosθ  = 弹射方向与法线的夹角余弦
        //       pdf   = 这种采样方式的概率密度（单位立体角）
        //
        //   ★ 关键：换了采样方式，pdf 就变了，attenuation 也必须跟着变。
        //     写错的后果是画面整体变亮/变暗，但不会报错 ——
        //     这是重要性采样最常见的 bug。
        // ============================================================

        if (g_uniform_hemisphere) {
            // ---------- 分支 A：均匀半球采样 ----------
            //
            //   采样：random_on_hemisphere(rec.normal)，方向在半球上处处均匀
            //   pdf 是"单位立体角上的概率密度"：pdf(ω) = 1 / (半球总立体角) = 1 / (2π)
            //   pdf ：1 / (2π)
            //
            // TODO(你填)：把 attenuation 算出来
            Vec3   dir   = random_on_hemisphere(rec.normal);
            double cos_t = dot(dir, rec.normal);     // dir 已经是单位向量
            scattered    = Ray(rec.p, dir);

            attenuation  = operator*(2 * cos_t, albedo);           // ← 你推

            return true;
        }

        // ---------- 分支 B：余弦加权采样（默认）----------
        //
        //   采样：rec.normal + random_unit_vector()，归一化后
        //   pdf ：cosθ / π
        //
        // TODO(你填)：把 attenuation 算出来
        //   ⚠️ 提示：这里 cosθ 和 π 都会被约掉，答案很短。
        Vec3 direction = rec.normal + random_unit_vector();
        scattered = Ray(rec.p, direction);

        attenuation  = albedo;               // ← 你推

        return true;    // 成功散射
    }

    // ★ Day 17：哑光是漫反射，可以做 NEE（主动朝光源采样）
    bool is_diffuse() const override { return true; }
    Color diffuse_albedo() const override { return albedo; }

private:
    // 反射率（albedo，拉丁语"白度"）
    //
    //   (1, 1, 1)        全反射       -> 白
    //   (0.5, 0.5, 0.5)  反射一半     -> 灰（就是 Day 3 那个写死的 0.5）
    //   (0.9, 0.3, 0.3)  只反射红光   -> 红
    //   (0.2, 0.4, 0.9)  蓝通道占优   -> 蓝
    Color albedo;
};
