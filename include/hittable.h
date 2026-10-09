#pragma once

#include "rtweekend.h"
#include "ray.h"
#include "vec3.h"
#include "aabb.h"      // 为了 AABB（包围盒）
#include "material.h"   // 为了 shared_ptr<Material>
                        // （material.h 用前向声明反向避免循环包含）

// ============================================================
// "事故报告单"
// 光线撞到物体之后，需要带回这些信息。
// 注意：这是"输出参数"，由 hit() 负责填满。
// ============================================================
struct HitRecord {
    Point3 p;      // 交点位置
    Vec3   normal; // 交点处的法线（必须是单位向量！）
    double t;      // 交点对应的光线参数

    bool front_face; // 光线是从物体外面打进来的吗？（还是从里面打出来的？）

    // 这个交点属于什么材质？由 Sphere::hit 在命中时填上。
    // 用 shared_ptr 而不是 Material*：
    //   一个材质可能被多个物体共享，shared_ptr 用引用计数自动管理
    //   生命周期，既不会内存泄漏，也不会变成悬空指针。
    std::shared_ptr<Material> mat;

    // 由 hit() 调用：决定 front_face，并把法线翻到"朝向光线来的一侧"
    //
    // outward_normal 是"几何上的外法线"（从球心指向交点）
    // 这个函数会根据光线方向，决定 rec.normal 是取 outward_normal 还是它的反向
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal     = front_face ? outward_normal : -outward_normal;
    }
};

// ============================================================
// 抽象基类：任何"能被光线撞到的东西"都继承它
//
// 它本身不能被实例化（有纯虚函数 = 0），
// 它存在的唯一目的，是让 main 里的循环不必知道
// 自己面对的到底是球、平面还是三角形。
// ============================================================
class Hittable {
public:
    // 虚析构：否则通过基类指针 delete 派生类对象时，派生类的析构不会被调用
    virtual ~Hittable() = default;

    // 光线 r 在开区间 (t_min, t_max) 内是否撞到自己？
    //   撞到   -> 返回 true，并把 rec 的三个字段全部填满
    //   没撞到 -> 返回 false，不要动 rec
    //
    // 这个签名的两个设计点：
    //   1. 返回 bool 而非 double：命中与否是"是/否"的问题，
    //      命中"细节"通过引用参数 rec 带出来（C++ 的"多返回值"手法）
    //   2. 带 t_min / t_max：由调用方告诉它"我只关心这个区间内的交点"
    virtual bool hit(const Ray& r, double t_min, double t_max,
                     HitRecord& rec) const = 0;

    // --------------------------------------------------------
    // 这个物体能不能被一个轴对齐包围盒框住？
    //   能   -> 把盒子写进 output，返回 true
    //   不能 -> 返回 false（比如理论上无限大的平面）
    //
    // 为什么要这个接口？
    //   BVH 要先把一堆物体框成一个盒子，才知道"这根光线有没必要
    //   走进这一块区域"。所以每个物体都得会报自己的盒子。
    //
    // 注意：它【不是】纯虚函数 —— 给了默认实现 false。
    //   这样以后加新几何体时不用马上实现它，编译器也不会报错。
    //   但代价是：如果忘了实现，整个世界的包围盒会静默地变成"没有"。
    // --------------------------------------------------------
    virtual bool bounding_box(AABB& output) const {
        (void)output;      // 消除"未使用参数"警告
        return false;
    }
};
