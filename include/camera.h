#pragma once

#include "ray.h"

// 相机：把归一化像素坐标 (u, v) 变成一条光线
class Camera {
public:
    Camera() {
        const double aspect_ratio    = 16.0 / 9.0;// 宽高比
        const double viewport_height = 2.0;
        const double viewport_width  = aspect_ratio * viewport_height;
        const double focal_length    = 1.0;

        origin     = Point3(0, 0, 0);
        horizontal = Vec3(viewport_width, 0, 0);// 水平向量
        vertical   = Vec3(0, viewport_height, 0);// 垂直向量

        // 视口左下角 = 原点 - 半宽 - 半高 - 焦距（相机在 +z 看向 -z）
        lower_left_corner = origin
                          - horizontal / 2.0
                          - vertical   / 2.0
                          - Vec3(0, 0, focal_length);
    }

    // u, v ∈ [0,1]，返回从相机原点穿过视口上该点的光线
    Ray get_ray(double u, double v) const {
        return Ray(origin,
                   lower_left_corner + u * horizontal + v * vertical - origin);
    }

private:
    Point3 origin;
    Point3 lower_left_corner;
    Vec3   horizontal;
    Vec3   vertical;
};
