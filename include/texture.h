#pragma once

#include "vec3.h"

#include <cmath>
#include <memory>

// ============================================================
// 纹理（Day 22）
//
// 一句话：纹理是「把 (u,v) 映射到颜色」的函数。
//
//   Texture::value(u, v, p) -> Color
//
// 三个参数为什么都要：
//   u, v  —— 表面参数坐标（来自 OBJ 的 vt，或者 Quad 的 (α, β)）
//   p     —— 交点世界坐标（有些纹理不需要 UV，比如 3D 棋盘格/噪声，
//            直接用位置查表，这样可以避免"接缝"和"拉伸"两大难题）
// ============================================================
class Texture {
public:
    virtual ~Texture() = default;
    virtual Color value(double u, double v, const Point3& p) const = 0;
};

// ============================================================
// 纯色（把颜色包装成纹理）
//
// 为什么需要它？因为材质只想跟"纹理"打交道，不想写两套逻辑：
//   albedo 是常量 → 包成 SolidColor
//   albedo 是贴图 → 用别的 Texture
// 这样材质代码只有一份。
// ============================================================
class SolidColor : public Texture {
public:
    SolidColor(const Color& c) : color_value(c) {}

    Color value(double /* u */, double /* v */, const Point3& /* p */) const override {
        return color_value;
    }

private:
    Color color_value;
};

// ============================================================
// 3D 棋盘格 —— 不需要 UV
//
// ============================================================
// 3D 棋盘格算法核心原理
// 目的：将连续的三维空间坐标 (x, y, z) 离散化，映射为交错的棋盘格。
// ============================================================
// 1. 缩放 (* inv_size)：
//    通过乘以格子边长的倒数，将物理坐标映射到以“格子”为单位的尺度。
//    例如格子大小为 2.0，坐标 3.5 会变成 1.75。
// 
// 2. 向下取整 (std::floor)：
//    算出该点属于“第几个格子”。必须用 std::floor 而非 int() 强转，
//    因为 int(-0.3) 会向零截断变成 0，而 floor(-0.3) 才是正确的 -1。
//    这样能保证跨越正负边界（原点）时，格子完美接缝。
// 
// 3. 坐标相加 (x + y + z)：
//    这是升维版棋盘格的魔法。在三维空间中，每跨越一个格子的边界，
//    其对应的坐标（x, y 或 z）必然加减 1。相加后的奇偶性必然反转。
//    这保证了任意相邻的两个方块，颜色必定相反。
// 
// 4. 奇偶判定 ((x + y + z) % 2 == 0)：
//    和为偶数涂颜色 A (even)，和为奇数涂颜色 B (odd)。
// 
// 优点：无需 UV 坐标，无接缝，无极点拉伸，是测试几何体最完美的
//       万用程序化纹理。
// ============================================================
//
// 好处：任何几何体都能贴，不存在接缝，也不存在 "球极区被压缩" 的拉伸。
// 用途：★ 调试几何和材质时最好的"万能测试图案"——
//       格子大小不均匀的地方，一定是 UV 或几何出了问题。
// ============================================================
class CheckerTexture3D : public Texture {
public:
    CheckerTexture3D(double size,
                     std::shared_ptr<Texture> even,
                     std::shared_ptr<Texture> odd)
        : inv_size(1.0 / size), even(even), odd(odd) {}

    Color value(double /* u */, double /* v */, const Point3& p) const override {
        int x = int(std::floor(p.x() * inv_size));
        int y = int(std::floor(p.y() * inv_size));
        int z = int(std::floor(p.z() * inv_size));
        bool is_even = ((x + y + z) % 2) == 0;
        return is_even ? even->value(0, 0, p) : odd->value(0, 0, p);
    }

private:
    double inv_size;
    std::shared_ptr<Texture> even;
    std::shared_ptr<Texture> odd;
};

// ============================================================
// UV 棋盘格 —— 真的用 (u, v)
//
// 这是今天的【教学主力】：棋盘格的边缘是直线，只要 UV 有一点错，
// 格子就会歪、会撕裂、会拉成条 —— 一眼就能看出来。
//
// 对比 3D 棋盘：UV 棋盘能暴露接缝和拉伸，3D 棋盘不能。
// ============================================================
class CheckerTextureUV : public Texture {
public:
    // nu / nv = 沿 u / v 方向要画几格
    CheckerTextureUV(double nu, double nv,
                     std::shared_ptr<Texture> even,
                     std::shared_ptr<Texture> odd)
        : nu(nu), nv(nv), even(even), odd(odd) {}

    Color value(double u, double v, const Point3& p) const override {
        // ------------------------------------------------------
        // TODO(T7)：算出这个 (u,v) 落在哪一格，是奇格还是偶格
        //
        //   ① 把 [0,1] 的 u 映射到 [0, nu] 格： gu = u * nu
        //   ② 取整数格号：                      iu = floor(gu)
        //   ③ v 同理：                          iv = floor(v * nv)
        //   ④ 奇偶判定：  格号之和为偶数 → even，奇数 → odd
        //
        //   ⚠️ 要用 std::floor，不能用 int() 直接截断：
        //      int(-0.3) 是 0，floor(-0.3) 是 -1。接缝处 u 可能略小于 0。
        int iu = std::floor(u * nu);
        int iv = std::floor(v * nv);

        bool is_even = ((iu + iv) % 2) == 0;

        return is_even ? even->value(u, v, p) : odd->value(u, v, p);
        // ------------------------------------------------------
    }

private:
    double nu, nv;
    std::shared_ptr<Texture> even;
    std::shared_ptr<Texture> odd;
};
