#pragma once

#include <cmath>
#include <iostream>
#include <rtweekend.h>

// 三维向量：一个类身兼三职（位置 / 方向 / 颜色）
class Vec3 {
public:
    Vec3() : e{0, 0, 0} {}
    Vec3(double e0, double e1, double e2) : e{e0, e1, e2} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    Vec3 operator-() const { return Vec3(-e[0], -e[1], -e[2]); }
    double  operator[](int i) const { return e[i]; }
    double& operator[](int i)       { return e[i]; }

    Vec3& operator+=(const Vec3& v) {
        e[0] += v.e[0]; e[1] += v.e[1]; e[2] += v.e[2];
        return *this;
    }
    Vec3& operator*=(double t) {
        e[0] *= t; e[1] *= t; e[2] *= t;
        return *this;
    }
    Vec3& operator/=(double t) { return *this *= 1.0 / t; }

    double length() const { return std::sqrt(length_squared()); }
    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

private:
    double e[3];
};

// 语义别名：同样的数据结构，不同的含义
using Point3 = Vec3;   // 位置
using Color  = Vec3;   // 颜色

// ---------- 流输出 ----------
inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
    return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

// ---------- 逐元素运算 ----------
inline Vec3 operator+(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() + v.x(), u.y() + v.y(), u.z() + v.z());
}
inline Vec3 operator-(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() - v.x(), u.y() - v.y(), u.z() - v.z());
}
inline Vec3 operator*(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() * v.x(), u.y() * v.y(), u.z() * v.z());
}

// ---------- 标量缩放 ----------
inline Vec3 operator*(double t, const Vec3& v) {
    return Vec3(t * v.x(), t * v.y(), t * v.z());
}
inline Vec3 operator*(const Vec3& v, double t) { return t * v; }
inline Vec3 operator/(const Vec3& v, double t) { return (1.0 / t) * v; }

// ---------- 几何运算 ----------
inline double dot(const Vec3& u, const Vec3& v) {
    return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

inline Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(
        u.y() * v.z() - u.z() * v.y(),
        u.z() * v.x() - u.x() * v.z(),
        u.x() * v.y() - u.y() * v.x()
    );
}

// 归一化：长度变 1，只保留方向信息
inline Vec3 unit_vector(const Vec3& v) {
    return v / v.length();
}

//在单位球体内随机取点
inline Vec3 random_unit_vector() {
    while (true) {
        Vec3 p(random_double(-1,1), random_double(-1,1), random_double(-1,1));
        double len2 = p.length_squared();//长度平方,len>1,在球体外,等于1，在球面，len<1e-8,太接近原点
        if(len2 > 1.0 || len2 < 1e-8) continue; //在球体外或太接近原点，重新取点
        return unit_vector(p);
    }
}

// ============================================================
// 反射：给定入射方向 v（必须【指向表面内部】）和单位法线 n，
//       返回反射方向。
//
// 公式：反射 = v − 2(v·n)n
// 推导：把 v 拆成"切向分量"和"法向分量"，
//       反射时【切向不变、法向翻转】：
//           切向 = v − (v·n)n
//           法向 = (v·n)n
//       反射 = 切向 − 法向 = v − (v·n)n − (v·n)n = v − 2(v·n)n
//
// ⚠️ 约定：v 必须指向表面【内部】（也就是光线前进的方向）。
//          如果 v 指向外侧，结果会完全错误。
// ============================================================
inline Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2 * dot(v, n) * n;
}

// 折射：uv = 单位入射方向（指向表面内部），n = 单位法线，
//       eta_ratio = 入射介质折射率 / 折射介质折射率
inline Vec3 refract(const Vec3& uv, const Vec3& n, double eta_ratio) {
    // ① 入射角的余弦（取负号是因为 uv 朝里、n 朝外，夹角是钝角）
    double cos_theta = std::fmin(dot(-uv, n), 1.0);

    // ② 折射后的【切向】分量
    //    uv + cos_theta*n 刚好把法向分量消掉，只剩切向
    //    再乘 eta_ratio —— 因为 sin θ₂ = eta_ratio × sin θ₁，
    //    而切向分量的长度正好就是 sin θ
    Vec3 r_out_perp = eta_ratio * (uv + cos_theta * n);

    // ③ 折射后的【法向】分量
    //    折射光线也必须是单位长度，所以 |法向| = √(1 − |切向|²)
    //    （用 fabs 兜底：全反射时 1−|切向|² 会是负数，正常流程里不会走到这）
    Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;

    return r_out_perp + r_out_parallel;
}