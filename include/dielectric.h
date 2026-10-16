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

        // ---------- 1. 这次是"进入"还是"离开"？ ----------
        //   front_face == true  → 从外侧射来（空气 → 玻璃）→ 1.0 / n
        //   front_face == false → 从内侧射来（玻璃 → 空气）→ n
        // 两者互为倒数，所以必须先判断方向。
        double eta_ratio = rec.front_face ? 1.0 / refraction_index : refraction_index;

        Vec3 unit_direction = unit_vector(r_in.direction());

        // 入射角的余弦 / 正弦
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        // ---------- 2. 全反射判据 ----------
        //   折射定律：sin θ₂ = eta_ratio × sin θ₁
        //   但 sin 不可能大于 1 —— 一旦超过，说明光根本出不去，全部弹回。
        bool cannot_refract = eta_ratio * sin_theta > 1.0;

        // ---------- 3. 决定走哪条路 ----------
        // 过去：只有"全反射"才反射，其余全部折射。
        //       结果玻璃表面 0% 反光，看起来像一块透明的空气。
        //
        // 现在（Day 12）：再加上【菲涅尔】—— 即使能折射出去，
        //       仍有 R(θ) 的比例会被表面反射回去。
        //
        // ------------------------------------------------------------
        // TODO(你填 S2)：把判定条件改成
        //
        //     cannot_refract  ||  reflectance(cos_theta, eta_ratio) > random_double()
        //
        // 读法：“出不去 或者 菲涅尔掷骰子命中” → 走反射分支
        //
        // ★ 这里在干什么？——用一个随机数决定命运
        //     reflectance(...) 返回 0~1 的概率（比如 0.04）
        //     random_double()  返回 [0,1) 的均匀随机数
        //     → 0.04 > random_double() 在【4% 的概率】下成立
        //     → 光线有 4% 概率走反射、96% 走折射
        //
        //   这正是蒙特卡洛方法：不去算“能量怎么分给两条路”，
        //   而是【随机走一条】，用大量样本让平均值收敛。
        //
        // ⚠️ 别把两个条件的位置写反：cannot_refract 在前，菲涅尔在后。
        //    先判“物理上必须反射”，再判“概率上该反射”。
        // ------------------------------------------------------------
        Vec3 direction;
        if (cannot_refract || reflectance(cos_theta, eta_ratio) > random_double()) {

            direction = reflect(unit_direction, rec.normal);

        } else {

            direction = refract(unit_direction, rec.normal, eta_ratio);

        }

        scattered = Ray(rec.p, direction);
        return true;
    }

private:
    double refraction_index;

    // ============================================================
    // TODO(你填 S1)：Schlick 菲涅尔近似
    //
    //   回答：“光以角度 θ 打在这个界面上，多大比例被【反射】？”
    //
    //   公式：
    //       R(θ) = R₀ + (1 − R₀) × (1 − cosθ)⁵
    //
    //       R₀   = ( (1 − n) / (1 + n) )²          ← n 是相对折射率
    //
    //   三个步骤：
    //     1. 算 base = (1 - refraction_index) / (1 + refraction_index)
    //        然后 base = base * base            ← 平方，得到 R₀
    //     2. 算 (1 - cosine) 的 5 次方         ← 用 std::pow(1 - cosine, 5)
    //     3. 返回 R₀ + (1 - R₀) * 第 2 步的结果
    //
    //   验算（玻璃 n = 1.5）：
    //     R₀ = ((1-1.5)/(1+1.5))² = (−0.2)² = 0.04
    //     垂直入射 cosine = 1  →  (1−1)⁵ = 0  →  R = 0.04   （反射 4%）
    //     掠射     cosine → 0  →  (1−0)⁵ = 1  →  R = 1.0    （反射 100%）
    //   两个极端都和现实对得上，这就是这个近似厉害的地方。
    //
    //   ⚠️ 为什么加 static：
    //      它只用参数，不用任何成员变量。
    //      static 成员函数【没有 this】，所以它根本无法访问 refraction_index ——
    //      这恰恰把“无隐藏依赖”这件事写进了类型里。
    //      （注意：参数和成员【同名】，但参数会遮蔽成员；
    //        而且 static 函数根本看不到成员，所以只可能是参数。)


    // ============================================================
    static double reflectance(double cosine, double refraction_index) {
        double base = (1.0 - refraction_index) / (1.0 + refraction_index);
        double R0 = base * base;
        double one_minus_cosine_5 = std::pow(1.0 - cosine, 5);
        double R = R0 + (1.0 - R0) * one_minus_cosine_5;
        return R;

        

    }
};
