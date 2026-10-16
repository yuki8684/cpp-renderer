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