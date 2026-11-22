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
    // aspect_ratio : 画面宽高比。默认 16:9，Day 30 起可以在出大图时覆盖
    // ------------------------------------------------------------
    Camera(Point3 lookfrom, Point3 lookat, Vec3 vup,
           double vfov, double focus_dist, double lens_radius,
           double aspect_ratio = 16.0 / 9.0) {

        // ★ Day 30：把机位参数存下来，动画时"只换位置、其余不变"
        //   —— 这比在动画循环里重新手写一遍构造参数安全得多：
        //     漏抄一个数（比如 lens_radius）画面就会莫名其妙地糊掉。
        this->lookfrom     = lookfrom;
        this->lookat       = lookat;
        this->vup          = vup;
        this->vfov         = vfov;
        this->focus_dist   = focus_dist;
        this->lens_radius  = lens_radius;
        this->aspect_ratio = aspect_ratio;

        // ---------- 第一步：算三个基向量 ----------
        // TODO(你填 C1)：三个基向量
        Vec3 w = unit_vector(lookfrom - lookat); // 相机背后的方向
        Vec3 u = unit_vector(cross(vup, w));     // 相机右方向（用 cross）
        Vec3 v = cross(w,u);                     // 相机上方向（用 cross）

        // ---------- 第二步：算视口 ----------
        double theta = degrees_to_radians(vfov);
        double h = std::tan(theta / 2.0);      // ← 半高的 tan 值
        // ⚠️ 视口要放在【对焦平面】上：距离相机 focus_dist，尺寸按同样倍率放大。
        //    这样 FOV 完全不变，但对焦平面能推到场景中间 ——
        //    否则"对焦距离 = 1"会把十几米外的景物全糊掉。
        double viewport_height = 2.0 * h * focus_dist;
        double viewport_width  = aspect_ratio * viewport_height;

        origin     = lookfrom;                 //把相机位置赋给origin

        horizontal = viewport_width * u;       // 横边：方向沿相机的右方向
        vertical   = viewport_height * v;      // 竖边：方向沿相机的上方向

        lower_left_corner = origin
                          - horizontal / 2.0
                          - vertical   / 2.0
                          - focus_dist * w;    // 沿着w（相机背后方向）走focus_dist 个单位
    }

    // ★ Day 30：同一个机位参数，只把位置换掉 —— 相机动画的基础
    Camera moved_to(const Point3& p) const {
        return Camera(p, lookat, vup, vfov, focus_dist, lens_radius, aspect_ratio);
    }

    const Point3& position() const { return lookfrom; }   // 现在在哪
    const Point3& target()   const { return lookat; }     // 看向哪（动画的支点）
    double        fov()      const { return vfov; }

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

    // Day 30：留一份原始参数，供 moved_to() 重建相机
    Point3 lookfrom      = Point3(0, 0, 0);
    Point3 lookat        = Point3(0, 0, -1);
    Vec3   vup           = Vec3(0, 1, 0);
    double vfov          = 90.0;
    double focus_dist    = 1.0;
    double aspect_ratio  = 16.0 / 9.0;
};
