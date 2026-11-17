// ============================================================
// 法线贴图 / 切线帧 单元测试（Day 25）
//
// 这个文件能测得很"硬"，因为切线帧是【精确的代数结果】：
//
//   三角形 a=(0,0,0) b=(1,0,0) c=(0,1,0)，UV 分别是 (0,0) (1,0) (0,1)
//
//   几何法线 N = cross(b−a, c−a) = (0,0,1)          → 朝 +z
//   切线     T = dP/du           = (1,0,0)          → 朝 +x
//   副切线   B = dP/dv           = (0,1,0)          → 朝 +y
//
//   所以"切线空间法线 (nx,ny,nz)"变换到世界后就是：
//       世界法线 = nx·(1,0,0) + ny·(0,1,0) + nz·(0,0,1) = (nx, ny, nz)
//
//   → 直接比对分量，不需要任何容差争论！
// ============================================================

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "ray.h"
#include "hittable.h"
#include "material.h"
#include "texture.h"
#include "quad.h"
#include "triangle.h"
#include "normal_map.h"

static int g_pass = 0;
static int g_fail = 0;

static void check_bool(const char* name, bool got, bool want) {
    if (got == want) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << (got ? "true" : "false")
                  << "  期望=" << (want ? "true" : "false") << '\n';
    }
}

static void check_vec(const char* name, const Vec3& got, const Vec3& want,
                      double tol = 1e-9) {
    bool ok = std::fabs(got.x() - want.x()) <= tol &&
              std::fabs(got.y() - want.y()) <= tol &&
              std::fabs(got.z() - want.z()) <= tol;
    if (ok) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=(" << got.x() << ", " << got.y() << ", " << got.z() << ")"
                  << "  期望=(" << want.x() << ", " << want.y() << ", " << want.z() << ")\n";
    }
}

static void check_near(const char* name, double got, double want, double tol = 1e-9) {
    if (std::fabs(got - want) <= tol) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << got << "  期望=" << want << '\n';
    }
}

// ---------- 测试替身（stub）----------

// 永远返回同一个值的纹理 —— 用来构造"完全平坦"或"固定倾斜"的法线图
class ConstantTexture : public Texture {
public:
    explicit ConstantTexture(const Color& c) : c(c) {}
    Color value(double, double, const Point3&) const override { return c; }
private:
    Color c;
};

// 只提供 normal_map() 的材质；scatter 永远返回 false（测试里不追踪光线）
class StubMaterial : public Material {
public:
    explicit StubMaterial(std::shared_ptr<Texture> nm) : nm(nm) {}
    bool scatter(const Ray&, const HitRecord&, Color&, Ray&) const override { return false; }
    std::shared_ptr<Texture> normal_map() const override { return nm; }
private:
    std::shared_ptr<Texture> nm;
};

// 把 0..1 的编码颜色变成切线空间法线（2c−1）
static Color enc(double nx, double ny, double nz) {
    return Color(0.5 * nx + 0.5, 0.5 * ny + 0.5, 0.5 * nz + 0.5);
}

// ★ 关键：解码（2c−1）发生在 NormalMapTexture 里，不在 Triangle 里。
//   所以测试必须包一层 NormalMapTexture，否则拿到的是【未解码】的 0~1 颜色。
//   （第一版测试就错在这里，得到了 normalize((0.5,0.5,1.0)) = (0.408,0.408,0.816)）
static std::shared_ptr<Texture> nmap(double nx, double ny, double nz) {
    return std::make_shared<NormalMapTexture>(
        std::make_shared<ConstantTexture>(enc(nx, ny, nz)));
}

// 发一条从正上方垂直射下来的光线，返回命中记录的 normal
// （ray 方向 (0,0,−1)，起点在三角形正上方）
static bool shoot(const Triangle& tri, HitRecord& rec, double& t_out) {
    Ray r(Point3(0.25, 0.25, 1.0), Vec3(0, 0, -1));
    bool hit = tri.hit(r, 0.001, 1000.0, rec);
    t_out = hit ? rec.t : -1.0;
    return hit;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 法线贴图 / 切线帧单元测试 =====\n";

    // 三角形在 z=0 平面：a=(0,0,0) b=(1,0,0) c=(0,1,0)
    // UV：a=(0,0) b=(1,0) c=(0,1)
    // → N=(0,0,1)  T=(1,0,0)  B=(0,1,0)
    const Point3 A(0, 0, 0), B(1, 0, 0), C(0, 1, 0);

    // ========================================================
    // 组 N1~N4：切线空间 → 世界空间 的精确对应
    // ========================================================
    std::cout << "\n--- 组 N1~N4: 切线帧变换 ---\n";
    {
        struct Case { const char* name; double nx, ny, nz; Vec3 want; };
        const Case cases[] = {
            {"N1 平坦法线 (0,0,1) -> 几何法线 (0,0,1)",      0, 0, 1, Vec3(0, 0, 1)},
            {"N2 切线     (1,0,0) -> T            (1,0,0)",  1, 0, 0, Vec3(1, 0, 0)},
            {"N3 副切线   (0,1,0) -> B            (0,1,0)",  0, 1, 0, Vec3(0, 1, 0)},
        };

        for (const auto& c : cases) {
            auto mat = std::make_shared<StubMaterial>(nmap(c.nx, c.ny, c.nz));

            Triangle tri(A, B, C, mat);
            tri.set_uv(UV{0, 0}, UV{1, 0}, UV{0, 1});

            HitRecord rec; double t;
            if (!shoot(tri, rec, t)) { std::cout << "  [FAIL] 没命中: " << c.name << '\n'; ++g_fail; continue; }
            check_vec(c.name, rec.normal, c.want);
        }

        // N4：整条对角线方向 (1,0,1)/sqrt2
        {
            const double s = 1.0 / std::sqrt(2.0);
            auto mat = std::make_shared<StubMaterial>(nmap(s, 0, s));
            Triangle tri(A, B, C, mat);
            tri.set_uv(UV{0, 0}, UV{1, 0}, UV{0, 1});
            HitRecord rec; double t;
            shoot(tri, rec, t);
            check_vec("N4 斜法线 (1,0,1)/√2 -> (1,0,1)/√2", rec.normal, Vec3(s, 0, s));
        }
    }

    // ========================================================
    // 组 N5：换一套 UV，切线跟着转
    //
    //   这次让 u 沿 +y、v 沿 +x：
    //       a=(0,0) b=(0,1) c=(1,0)
    //       du/dv 解出来 T=(0,1,0)  B=(1,0,0)
    //   所以切线空间法线 (1,0,0) 现在的世界方向是 (0,1,0)。
    //
    //   ★ 这一条证明：法线贴图【跟着 UV 走】，不是跟着世界轴走。
    //     这就是为什么"UV 换了朝向，贴图效果自动跟着换"。
    // ========================================================
    std::cout << "\n--- 组 N5: UV 换向后切线跟着转 ---\n";
    {
        auto mat = std::make_shared<StubMaterial>(nmap(1, 0, 0));

        Triangle tri(A, B, C, mat);
        tri.set_uv(UV{0, 0}, UV{0, 1}, UV{1, 0});    // u 沿 +y

        HitRecord rec; double t;
        if (!shoot(tri, rec, t)) { std::cout << "  [FAIL] 没命中\n"; ++g_fail; }
        else check_vec("N5 UV 转了 90 度 -> (1,0,0) 变成 (0,1,0)", rec.normal, Vec3(0, 1, 0));
    }

    // ========================================================
    // 组 N6~N7：法线贴图【不改变几何】
    // ========================================================
    std::cout << "\n--- 组 N6~N7: 几何不变 ---\n";
    {
        // 无法线贴图
        auto plain = std::make_shared<StubMaterial>(nullptr);
        Triangle t0(A, B, C, plain);
        t0.set_uv(UV{0, 0}, UV{1, 0}, UV{0, 1});
        HitRecord r0; double tA;
        shoot(t0, r0, tA);

        // 有法线贴图（大幅倾斜）
        auto tilted = std::make_shared<StubMaterial>(nmap(1, 0, 0));
        Triangle t1(A, B, C, tilted);
        t1.set_uv(UV{0, 0}, UV{1, 0}, UV{0, 1});
        HitRecord r1; double tB;
        shoot(t1, r1, tB);

        check_near("N6 命中距离 t 完全一致（几何未变）", tB, tA);
        check_bool("N7 命中位置 p 完全一致",
                   std::fabs(r1.p.x() - r0.p.x()) < 1e-12 &&
                   std::fabs(r1.p.y() - r0.p.y()) < 1e-12 &&
                   std::fabs(r1.p.z() - r0.p.z()) < 1e-12, true);
    }

    // ========================================================
    // 组 N8：扰动后的法线仍是单位向量
    // ========================================================
    std::cout << "\n--- 组 N8: 扰动后仍是单位向量 ---\n";
    {
        // 故意给一个没归一化的切线空间法线 (1, 1, 1)
        auto mat = std::make_shared<StubMaterial>(nmap(1, 1, 1));
        Triangle tri(A, B, C, mat);
        tri.set_uv(UV{0, 0}, UV{1, 0}, UV{0, 1});
        HitRecord rec; double t;
        shoot(tri, rec, t);

        const double L = rec.normal.length();
        check_near("N8 |normal| == 1", L, 1.0);
        const double s = 1.0 / std::sqrt(3.0);
        check_vec("N8 (1,1,1) 被归一化成 (1,1,1)/√3", rec.normal, Vec3(s, s, s));
    }

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) { std::cout << "ALL PASSED\n"; }
    return g_fail == 0 ? 0 : 1;
}
