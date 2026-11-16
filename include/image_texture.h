#pragma once

#include "image.h"
#include "texture.h"

#include <cmath>
#include <memory>
#include <string>

// ============================================================
// ImageTexture：把一张图片变成 Texture（Day 24）
//
// 和 Day 22 的程序化棋盘格相比，唯一新增的两件事：
//   ① 双线性插值（Bilinear）—— 否则贴图放大后是硬块
//   ② v 轴翻转 —— PPM 第 0 行在顶部，OBJ 的 v=0 在底部
//
// 重采样方式：**平铺（repeat）**，超出 [0,1] 的 UV 自动绕回，
// 和 UE 里 Texture Sampler 的默认 Wrap 模式一致。
// ============================================================

// TODO(你填 D6)：把任意整数索引绕回 [0, n)
//
//   ✔ wrap_index(-1, 8) == 7
//   ✔ wrap_index( 8, 8) == 0
//   ✔ wrap_index(-9, 8) == 7
//
//   ⚠️ C++ 的 % 对负数返回【负数】，所以不能直接 i % n 就完事。
//      （Day 22 的棋盘格用 int() 截断也踩过同类坑，那次是 -0.3 → 0）
inline int wrap_index(int i, int n) {
    i %= n;
    if (i < 0) i += n;
    return i;   // TODO(你填 D6)
}

class ImageTexture : public Texture {
public:
    explicit ImageTexture(std::shared_ptr<const Image> image)
        : image(std::move(image)) {}

    // 加载失败时返回一张"洋红"的纹理：渲染出来一眼就知道出事了
    static std::shared_ptr<ImageTexture> load(const std::string& path,
                                              bool srgb = true) {
        auto loaded = std::make_shared<Image>(load_ppm(path, srgb));
        if (!loaded->valid()) {
            std::cerr << "[纹理] 加载失败，用洋红代替: " << path << '\n';
        } else {
            std::cerr << "[纹理] " << path << "  "
                      << loaded->width << "x" << loaded->height
                      << (srgb ? "  (sRGB→线性)" : "  (按线性数据读)")
                      << '\n';
        }
        return std::make_shared<ImageTexture>(loaded);
    }

    Color value(double u, double v, const Point3& /* p */) const override {
        if (!image || !image->valid()) return Color(1.0, 0.0, 1.0);

        // ------------------------------------------------------
        // TODO(你填 D7)：双线性采样（Bilinear）
        //
        //   ① 连续坐标
        //        x = u * w - 0.5
        //        y = (1 - v) * h - 0.5
        //      ★ 为什么要 -0.5：像素中心落在 0.5, 1.5, ... 处，
        //        不减就会整体偏移半个像素（贴图看起来"错位/发虚"）。
        //      ★ 为什么要 1 - v：PPM 第 0 行在【顶部】，
        //        而 OBJ 的 v=0 在【底部】 —— 不翻转贴图就上下颠倒。
        //
        //   ② 取四个邻居
        //        x0 = floor(x), fx = x - x0
        //        y0 = floor(y), fy = y - y0
        //        四个纹素：(x0,y0) (x0+1,y0) (x0,y0+1) (x0+1,y0+1)
        //        用下面的 texel() 取 —— 它已经处理了 wrap 绕回
        //
        //   ③ 先沿 u 插值两行，再沿 v 插值：
        //        bottom = (1-fx)*c00 + fx*c10
        //        top    = (1-fx)*c01 + fx*c11
        //        结果   = (1-fy)*bottom + fy*top
        // ------------------------------------------------------
        double x = u * image->width - 0.5;
        double y = (1.0 - v) * image->height - 0.5;

        int x0 = int(std::floor(x)); double fx = x - x0;
        int y0 = int(std::floor(y)); double fy = y - y0;

        const Color& c00 = texel(x0, y0);
        const Color& c10 = texel(x0 + 1, y0);
        const Color& c01 = texel(x0, y0 + 1);
        const Color& c11 = texel(x0 + 1, y0 + 1);

        Color bottom = (1.0 - fx) * c00 + fx * c10;
        Color top    = (1.0 - fx) * c01 + fx * c11;
        return (1.0 - fy) * bottom + fy * top;
    }

    int width()  const { return image ? image->width  : 0; }
    int height() const { return image ? image->height : 0; }

private:
    const Color& texel(int x, int y) const {
        // 下标按平铺方式绕回（wrap），保证接缝处也连续
        const int xx = wrap_index(x, image->width);
        const int yy = wrap_index(y, image->height);
        return image->pixels[std::size_t(yy) * std::size_t(image->width) + std::size_t(xx)];
    }

    std::shared_ptr<const Image> image;
};
