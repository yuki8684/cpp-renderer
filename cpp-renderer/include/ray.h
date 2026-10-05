struct Ray {
    Point3 origin; // 光线的起点
    Vec3 direction; // 光线的方向

    Ray() : origin(Point3(0, 0, 0)), direction(Vec3(0, 0, 0)) {}
    Ray(const Point3& o, const Vec3& d) : origin(o), direction(d) {}

    Point3 at(double t) const {
        return origin + t * direction; // 根据参数 t 计算光线上的点
    }
};