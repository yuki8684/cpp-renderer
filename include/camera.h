#pragma once

#include "ray.h"
#include "rtweekend.h"      // ← 需要 random_in_unit_disk / degrees_to_radians

// 相机：现在可以放在任意位置、看向任意方向
class Camera {
public:
    // ------------------------------------------------------------
    // lookfrom : 相机位置
    // lookat   : 看向哪里
    // vup      : 世界坐标里的"上"（一般填 (0,1,0)）
    // vfov     : 垂直视野角（度）。90 = 广角，30 = 长焦
    // focus_dist : 对焦距离。只有在这个距离上的物体会【完全清晰】
    // lens_radius : 光圈半径。0 = 针孔（全清晰），越大越糊
    // ------------------------------------------------------------
    Camera(Point3 lookfrom, Point3 lookat, Vec3 vup,
           double vfov, double focus_dist, double lens_radius) {

        const double aspect_ratio = 16.0 / 9.0;

        // ---------- 第一步：算三个基向量 ----------
        // TODO(你填 C1)：三个基向量
        Vec3 w = unit_vector(lookfrom - lookat);                       // 相机背后的方向
        Vec3 u = unit_vector(cross(vup, w));                       // 相机右方向（用 cross）
        Vec3 v = cross(w,u);                       // 相机上方向（用 cross）

        // ---------- 第二步：算视口 ----------
        double theta = degrees_to_radians(vfov);
        double h = std::tan(theta / 2.0);      // ← 半高的 tan 值
        // ⚠️ 视口要放在【对焦平面】上：距离相机 focus_dist，尺寸按同样倍率放大。
        //    这样 FOV 完全不变，但对焦平面能推到场景中间 ——
        //    否则"对焦距离 = 1"会把十几米外的景物全糊掉。
        double viewport_height = 2.0 * h * focus_dist;
        double viewport_width  = aspect_ratio * viewport_height;

        origin = lookfrom;                     // ← origin 现在真的有意义了

        horizontal = viewport_width * u;       // 横边：方向沿相机的右方向
        vertical   = viewport_height * v;      // 竖边：方向沿相机的上方向

        lower_left_corner = origin
                          - horizontal / 2.0
                          - vertical   / 2.0
                          - focus_dist * w;   // 往前推 focus_dist 个单位

        this->lens_radius = lens_radius;
    }

    // ⚠️ 像素坐标参数改名为 s, t，避免和基向量 u, v 冲突
    Ray get_ray(double s, double t) const {
        Vec3   offset     = lens_radius * random_in_unit_disk();
        Point3 ray_origin = origin + offset;

        return Ray(ray_origin,
                   lower_left_corner + s * horizontal + t * vertical - ray_origin);
    }

private:
    Point3 origin;
    Point3 lower_left_corner;
    Vec3   horizontal;
    Vec3   vertical;
    double lens_radius;
};
