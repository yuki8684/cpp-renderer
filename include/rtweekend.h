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

// ---------- 随机数工具 ----------
inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0,1.0);
    static std::mt19937 generator;
    return distribution(generator);// [0,1) 区间随机数
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
