#pragma once

#include <cmath>
#include <iostream>

class Vec3 {
public:
    double e[3];

    Vec3() : e{0, 0, 0} {}
    Vec3(double x, double y, double z) : e{x, y, z} {}

    inline double x() const { return e[0]; }
    inline double y() const { return e[1]; }
    inline double z() const { return e[2]; }

    inline const Vec3& operator+() const { return *this; }
    inline Vec3 operator-() const { return Vec3(-e[0], -e[1], -e[2]); }

    inline double operator[](int i) const { return e[i]; }
    inline double& operator[](int i) { return e[i]; }

    inline Vec3& operator+=(const Vec3 &v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    inline Vec3& operator*=(const double t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    inline Vec3& operator/=(const double t) {
        return *this *= (1 / t);
    }

    inline double length() const {
        return std::sqrt(length_squared());
    }

    inline double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    inline static Vec3 random() {
        return Vec3(drand48(), drand48(), drand48());
    }

    inline static Vec3 random(double min, double max) {
        return Vec3(min + (max - min) * drand48(),
                    min + (max - min) * drand48(),
                    min + (max - min) * drand48());
    }
};

inline std::ostream& operator<<(std::ostream &out, const Vec3 &v) {
    return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

inline Vec3 operator+(const Vec3 &u, const Vec3 &v) {
    return Vec3(u) += v;
}

inline Vec3 operator-(const Vec3 &u, const Vec3 &v) {
    return Vec3(u) -= v;
}

inline Vec3 operator*(const Vec3 &u, const Vec3 &v) {
    return Vec3(u.x() * v.x(), u.y() * v.y(), u.z() * v.z());
}

inline Vec3 operator*(double t, const Vec3 &v) {
    return Vec3(t * v.x(), t * v.y(), t * v.z());
}

inline Vec3 operator*(const Vec3 &v, double t) {
    return t * v;
}

inline Vec3 operator/(const Vec3 &v, double t) {
    return (1 / t) * v;
}

inline double dot(const Vec3 &u, const Vec3 &v) {
    return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

inline Vec3 cross(const Vec3 &u, const Vec3 &v) {
    return Vec3(u.y() * v.z() - u.z() * v.y(),
                 u.z() * v.x() - u.x() * v.z(),
                 u.x() * v.y() - u.y() * v.x());
}

inline Vec3 unit_vector(const Vec3 &v) {
    return v / v.length();
}