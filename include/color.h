#pragma once

#include "vec3.h"
#include <cmath>

// ============================================================
// color.h —— 颜色编码
//
// 职责：把渲染器内部的【线性光强】变成能写进文件的【字节】。
//
// 关键概念：存储空间 ≠ 光强空间
//   线性光强 L（物理量）
//       │
//       │  ← 这里要开一次方（gamma 校正）
//       ▼
//   存储值 s = L^(1/2.2) ≈ sqrt(L)
//       │
//       │  × 255 取整
//       ▼
//   字节 0~255
//
//   显示器再平方一次，就还原成 L 了。
// ============================================================

// TODO(你填 G1)：线性光强 → gamma 空间
//
//   要求：
//     - 输入 linear
//     - 如果 linear > 0，返回 sqrt(linear)
//     - 否则返回 0      ← ⚠️ 负数不能开方！会得到 NaN
//
//   想一想：linear 可能是负数吗？
//     （提示：ray_color 里 attenuation 和颜色都是正的，
//       但浮点误差可能造出 -1e-18 这种"负零"）
inline double linear_to_gamma(double linear) {
    double s = linear > 0 ? std::sqrt(linear) : 0;
    return s;
}

// ============================================================
// TODO(你填 D1)：sRGB 编码值 → 线性光强（解码）
//
//   要求：
//     - s <= 0 返回 0；s >= 1 返回 1（先截断，pow 才不会外推成 NaN）
//     - 分段：s <= 0.04045  →  s / 12.92
//             s >  0.04045  →  pow((s + 0.055) / 1.055, 2.4)
//
//   ★ 为什么暗部要单独用一段直线？
//     纯幂函数在 0 附近斜率无穷大，会把编码噪声放大成可见的脏点；
//     暗部的【相对】精度又恰好是人眼最敏感的，所以 sRGB 特意换了直线段。
// ============================================================
inline double srgb_to_linear(double s) {
    s = std::fmax(0.0, std::fmin(s, 1.0));   // 先截断到 [0,1]，pow 才不会外推成 NaN
    if (s <= 0.04045) {
        return s / 12.92;
    } else {
        return std::pow((s + 0.055) / 1.055, 2.4);
    }
}

// TODO(你填 D2)：线性光强 → sRGB 编码值（编码，解码的逆运算）
//
//   要求：
//     - l <= 0 返回 0；l >= 1 返回 1
//     - 分段：l <= 0.0031308 → 12.92 * l
//             l >  0.0031308 → 1.055 * pow(l, 1/2.4) - 0.055
//
//   ⚠️ 解码用 2.4，编码用 1/2.4。写反了画面会明显偏暗或偏亮，
//      但不会报错 —— 这是色彩空间最常见的静默 bug。
inline double linear_to_srgb(double l) {
    l = std::fmax(0.0, std::fmin(l, 1.0));   // 先截断到 [0,1]，pow 才不会外推成 NaN
    if (l <= 0.0031308) {
        return 12.92 * l;
    } else {
        return 1.055 * std::pow(l, 1.0 / 2.4) - 0.055;
    }
}

// TODO(你填 G2)：线性光强 → 0~255 字节
//
//   要求：
//     1. 先 gamma 校正
//     2. 超过 0.999 就截到 0.999      ← 防止算出 256（非法字节）
//     3. 乘以 255.999 再转 int（截断）
inline int to_byte(double linear_component) {

    double gamma = linear_to_gamma(linear_component);

    // 上限截断：gamma 理论上 ≤ 1，但浮点误差可能略微超出。
    // 不截的话 255.999 × 1.0001 ≈ 256.03 → 会写出非法的 256 字节。
    double clamped = std::fmin(gamma, 0.999);

    // ⚠️ 变量别命名成 byte —— <windows.h> 会带进 typedef unsigned char byte;
    //    用那个名字会遮蔽掉全局类型（能编译、无警告，但容易让人掉坑）
    int value = static_cast<int>(clamped * 255.999);
    return value;

}