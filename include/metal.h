#pragma once

#include "material.h"
#include "hittable.h"

// ============================================================
// Metal：金属材质
//
// 行为：镜面反射（入射角 = 反射角），外加一个"粗糙度" fuzz.
//
//   fuzz = 0.0   完美镜面   —— 抛光铬、镀银镜
//   fuzz = 0.1   轻微模糊   —— 不锈钢
//   fuzz = 0.4   明显磨砂   —— 喷砂铝
// ============================================================
class Metal : public Material {
public:
    // 三元运算符 a ? b : c 读作"如果 a 就用 b，否则用 c"
    // 这里的作用：不管外面传什么，fuzz 都被夹在 [0, 1] 内（防御性编程）
    Metal(const Color& albedo, double fuzz)
        : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override {

        // ------------------------------------------------------------
        // TODO(你来实现)：三步
        //
        //   ① 算出反射方向
        //        用 vec3.h 里的 reflect()
        //        第一个参数 = 入射光线的方向（记得先 unit_vector 归一化）
        //        第二个参数 = 交点法线 rec.normal
        //
        //   ② 造弹射光线
        //        起点 = 交点
        //        方向 = ①算出的反射方向  +  fuzz 倍的随机扰动
        //        （这个随机扰动就是"粗糙度"：方向被抖一下，反射就糊了）
        //
        //   ③ 反射率 = 存起来的 albedo
        //
        // 填完后把那三行 (void)... 删掉
        // ------------------------------------------------------------
        Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = Ray(rec.p, reflected + fuzz * random_unit_vector());
        attenuation = albedo;

        // 扰动可能把方向推到表面下面去 —— 那种光线不该存在，返回 false 表示吸收
        return dot(scattered.direction(), rec.normal) > 0;
    }

private:
    Color  albedo;
    double fuzz;
};
