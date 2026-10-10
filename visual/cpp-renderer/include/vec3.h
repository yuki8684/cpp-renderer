class Vec3 {
public:
    Vec3() : e{0, 0, 0} {}
    Vec3(double x, double y, double z) : e{x, y, z} {}

    double x() const { return e[0]; }
    double y() const { return e[1]; }
    double z() const { return e[2]; }

    Vec3 operator+(const Vec3& v) const {
        return Vec3(e[0] + v.e[0], e[1] + v.e[1], e[2] + v.e[2]);
    }

    Vec3 operator-(const Vec3& v) const {
        return Vec3(e[0] - v.e[0], e[1] - v.e[1], e[2] - v.e[2]);
    }

    Vec3 operator*(double t) const {
        return Vec3(e[0] * t, e[1] * t, e[2] * t);
    }

    Vec3 operator/(double t) const {
        return Vec3(e[0] / t, e[1] / t, e[2] / t);
    }

    double length() const {
        return std::sqrt(length_squared());
    }

    double length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    static Vec3 random() {
        return Vec3(random_double(), random_double(), random_double());
    }

    static Vec3 random(double min, double max) {
        return Vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }

private:
    double e[3];

    static double random_double() {
        return rand() / (RAND_MAX + 1.0);
    }

    static double random_double(double min, double max) {
        return min + (max - min) * random_double();
    }
};