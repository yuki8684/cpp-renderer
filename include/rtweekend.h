#pragma once

// ============================================================
// 公共头：所有模块都会用到的标准库 + 全局常量
// 约定：其它头文件第一行都 #include "rtweekend.h"
// ============================================================

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <random>

// ---------- 数学常量 ----------

// ============================================================
// ★ Day 18：随机数发生器改成 thread_local
//
//   static      → 全程序只有一份 → 多线程同时用 = 数据竞争 = 未定义行为
//   thread_local → 每个线程各有一份 → 互不干扰
//
// 注意：thread_local 变量是【懒构造】的，第一次用到时才初始化，
//       所以每个新线程一开始用的是默认种子 5489 ——
//       必须在渲染循环里【显式播种】，否则所有线程会产生一样的序列。
// ============================================================
inline std::mt19937& rng() {
    thread_local std::mt19937 generator;   // 伪随机数生成器（Mersenne Twister）
    return generator;
}

// ---------- 随机数工具 ----------
inline double random_double() {
    // ⚠️ 这里也必须是 thread_local：
    //    uniform_real_distribution::operator() 在标准里【不是 const】，
    //    多个线程同时调用同一个 static 对象，严格按标准就是数据竞争（未定义行为）。
    //    虽然主流实现在里面不写状态、实际上不会炸，但不能靠"实现碰巧安全"过日子。
    thread_local std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(rng());   // [0,1) 区间随机数
}

// 重新播种（不加参数就回到默认种子，基准测试靠它保证可复现）
inline void reset_rng(unsigned seed = 5489u) {
    rng().seed(seed);
}

//---------- [min,max) 区间随机数 ----------
inline double random_double(double min, double max) {
    return min + (max - min) * random_double(); 
}

// "无穷大"：用来表示"目前还没有找到任何交点，最近命中距离是无穷远"
const double infinity = std::numeric_limits<double>::infinity();

const double pi = 3.1415926535897932385;

// 角度转弧度（Day 3 做材质/随机时要用）
inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}
