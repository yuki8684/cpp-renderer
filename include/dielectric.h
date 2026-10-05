#pragma once

#include "material.h"
#include "hittable.h"

// ============================================================
// Dielectric：透明介质（玻璃、水、钻石）
//
// 和其他材质的最大区别：光线会【穿过】表面，而不是只在表面弹开。
// 所以它要用到 HitRecord::front_face —— 判断这次是从外侧打进来、
// 还是从物体内部打出去。
// ============================================================
class Dielectric : public Material {
public:
    // refraction_index = 该物体的折射率
    //     空气 1.0  |  水 1.33  |  玻璃 1.5  |  钻石 2.42
    Dielectric(double refraction_index)
        : refraction_index(refraction_index) {}

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override {

        // 玻璃不吸光：反射率全通道为 1
        attenuation = Color(1.0, 1.0, 1.0);

        // ------------------------------------------------------------
        // TODO(你填 D1)：算 eta_ratio = 入射介质折射率 ÷ 折射介质折射率
        //
        //   rec.front_face == true  → 光线从外侧射来（空气 → 玻璃）
        //                              → 1.0 / refraction_index
        //   rec.front_face == false → 光线从内侧射来（玻璃 → 空气）
        //                              → refraction_index
        //
        //   形状： rec.______ ? ______ : ______
        // ------------------------------------------------------------
        double eta_ratio = rec.front_face ? 1.0 / refraction_index : refraction_index;

        Vec3 unit_direction = unit_vector(r_in.direction());

        // 入射角的余弦 / 正弦
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        // ------------------------------------------------------------
        // TODO(你填 D2)：判断是否发生【全反射】
        //
        //   折射定律：sin θ₂ = eta_ratio × sin θ₁
        //   但 sin 不可能大于 1 —— 一旦超过 1，说明光根本出不去，全部弹回。
        //
        //   形状： ______ * sin_theta > 1.0
        // ------------------------------------------------------------
        bool cannot_refract = eta_ratio * sin_theta > 1.0;

        Vec3 direction;
        if (cannot_refract) {
            // TODO(你填 D3)：出不去 → 只能反射
            //   形状： reflect(unit_direction, rec.normal)
            direction = reflect(unit_direction, rec.normal);
        } else {
            // TODO(你填 D4)：能出去 → 折射
            //   形状： refract(unit_direction, rec.normal, ______)
            //   最后一个参数想想该传什么
            direction = refract(unit_direction, rec.normal, eta_ratio);
        }

        scattered = Ray(rec.p, direction);
        return true;
    }

private:
    double refraction_index;
};
