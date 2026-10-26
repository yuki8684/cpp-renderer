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

        // ------------------------------------------------------------
        // TODO(你来实现)：三步
        //
        //   ① 算弹射方向
        //        方向 = 表面法线 + 一个随机单位向量
        //        （回想上节课：+ 法线 是为了保证方向一定在球的【外侧】）
        //
        //   ② 造一条弹射光线
        //        起点 = 交点
        //        方向 = 上面算出来的方向
        //        （提示：Ray 的构造函数要 (起点, 方向) 两个参数）
        //
        //   ③ 填反射率
        //        就是构造时传进来、存在成员 albedo 里的那个颜色
        //
        // 三个都写完后，把下面三行占位删掉，并把 return false 改成 return true。
        // ------------------------------------------------------------
        // ------------------------------------------------------------
        // ★ Day 19 对照实验：均匀半球 vs 余弦加权
        //
        //   两种采样的【期望完全相同】（画面亮度一致），差的只是方差。
        //   关键是把 pdf 补偿正确 —— 否则亮度会错。
        // ------------------------------------------------------------
        if (g_uniform_hemisphere) {
            // 均匀半球：pdf = 1/(2π)
            //   估计量权重 w = f·cos/pdf = (albedo/π)·cos / (1/2π) = 2·albedo·cos
            Vec3   dir   = random_on_hemisphere(rec.normal);
            double cos_t = dot(dir, rec.normal);     // dir 已经是单位向量
            scattered    = Ray(rec.p, dir);
            attenuation  = 2.0 * cos_t * albedo;     // ← 补偿 pdf，否则亮度会差 π 倍
            return true;
        }

        // 默认：余弦加权（pdf = cosθ/π）
        //   权重 w = f·cos/pdf = (albedo/π)·cos / (cos/π) = albedo
        //   ★ cos 和 π 被【完全约掉】了 —— 这就是为什么这里只写 albedo
        Vec3 direction = rec.normal + random_unit_vector();
        scattered = Ray(rec.p, direction);
        attenuation = albedo;
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
