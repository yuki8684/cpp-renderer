#pragma once

#include "hittable.h"
#include "image_texture.h"
#include "material.h"
#include "texture.h"

#include <cmath>
#include <memory>
#include <string>

// ============================================================
// 法线贴图（Day 24）
//
// 一句话：法线贴图【不改变几何】，只改变"着色时用哪个法线"。
//   三角形的顶点没动一个，但每个像素算出的小起伏让光照以为
//   表面有凹凸 —— 这就是"假几何"。
//
// 存储约定（和 UE / OpenGL 一致）：
//   R → 切线方向 T
//   G → 副切线方向 B（"绿色向上"）
//   B → 几何法线方向 N
//   编码：0.5 表示 0，所以解码是  2c − 1
//
// ★ 法线图是【线性数据】，绝对不能做 sRGB 解码！
//   只有反照率 / 自发光这类"颜色"才按 sRGB 解码。
//   混了这一步，法线会全部偏斜，画面出现莫名其妙的硬边。
// ============================================================
class NormalMapTexture : public Texture {
public:
    explicit NormalMapTexture(std::shared_ptr<Texture> src) : src(std::move(src)) {}

    static std::shared_ptr<NormalMapTexture> load(const std::string& path) {
        return std::make_shared<NormalMapTexture>(
            ImageTexture::load(path, /* srgb = */ false));
    }

    // 返回【切线空间】的单位法线（已经是 −1~1，不是 0~1）
    Color value(double u, double v, const Point3& p) const override {
        const Color c = src->value(u, v, p);          // 0~1

        // ① 解码：0.5 表示 0，所以 2c − 1
        Vec3 n(2.0 * c.x() - 1.0,
               2.0 * c.y() - 1.0,
               2.0 * c.z() - 1.0);

        // ② 归一化；长度≈0 时返回"平法线"
        //    ★ 为什么需要兜底：(128,128,128) 解码后是 (0,0,0)，代表"没有起伏"。
        //      不判断就直接除，会得到 NaN —— 而 NaN 会在 BVH / 着色里到处传播，
        //      症状是整块画面全黑或彩色噪点，极难定位。
        const double L2 = dot(n, n);
        if (L2 <= 1e-20) return Vec3(0.0, 0.0, 1.0);

        return n / std::sqrt(L2);
    }

private:
    std::shared_ptr<Texture> src;
};

// ============================================================
// NormalMappedMaterial：给任意材质"套一层"法线贴图
//
// 为什么用装饰器（decorator）而不是给 Lambertian 加参数？
//   ① 金属、玻璃、以后的新材质都能直接复用
//   ② Lambertian / Metal 的代码一行都不用改
//   ③ 不需要法线贴图的场景，行为完全不变（零开销）
//
// 真正做几何耦合（切线帧）的地方在 Triangle::shade_with_normal_map()：
//   只有几何知道 (u,v) 怎么映射到空间方向，所以切线必须由几何提供。
// ============================================================
class NormalMappedMaterial : public Material {
public:
    NormalMappedMaterial(std::shared_ptr<Material> inner,
                         std::shared_ptr<Texture> normal_tex)
        : inner(std::move(inner)), normal_tex(std::move(normal_tex)) {}

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override {
        return inner->scatter(r_in, rec, attenuation, scattered);
    }

    Color emitted() const override { return inner->emitted(); }
    bool  is_diffuse() const override { return inner->is_diffuse(); }

    Color diffuse_albedo(const HitRecord& rec) const override {
        return inner->diffuse_albedo(rec);
    }

    // ★ Triangle 命中时会通过这个接口把法线图取走，扰动 rec.normal
    std::shared_ptr<Texture> normal_map() const override { return normal_tex; }

private:
    std::shared_ptr<Material> inner;
    std::shared_ptr<Texture>  normal_tex;
};
