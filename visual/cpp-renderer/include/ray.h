class Ray {
public:
    Ray() : origin(Vec3()), direction(Vec3()) {}
    Ray(const Vec3& origin, const Vec3& direction) : origin(origin), direction(direction) {}

    Vec3 at(double t) const {
        return origin + t * direction;
    }

    const Vec3& getOrigin() const { return origin; }
    const Vec3& getDirection() const { return direction; }

private:
    Vec3 origin;
    Vec3 direction;
};