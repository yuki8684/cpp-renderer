// ============================================================
// 显式 PDF（Day 29）
//
// 一个概率密度函数 pdf 只有一个硬判据：
//
//      ∫ pdf(ω) dω  ==  1          （在采样空间的整个定义域上）
//
// 对半球上的方向采样，dω = sinθ dθ dφ，定义域是 θ∈[0,π/2]、φ∈[0,2π]。
//
// 这条断言能抓的东西非常多：
//   少乘 sinθ          → 积分变成 2
//   忘了归一化常数     → 积分变成 π 或 2π
//   pdf 用了 cos² 之类 → 积分对不上
//
// 其余断言验证的是**显式化之后的不变式**：
//   attenuation 不再随采样策略变化（它只是 BRDF 本身）
//   而 cosθ/pdf 补上了两者的差异
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
#include "lambertian.h"
#include "metal.h"

static const double PI_C = 3.14159265358979323846;

static int g_pass = 0;
static int g_fail = 0;

static void check_near(const char* name, double got, double want, double tol) {
    if (std::fabs(got - want) <= tol) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else {
        ++g_fail;
        std::cout << "  [FAIL] " << name << "    得到=" << got << "  期望=" << want << '\n';
    }
}

// 构造一个法线为 +z、位置在原点、UV 在中心的 HitRecord
static HitRecord make_rec(const std::shared_ptr<Material>& m) {
    HitRecord rec;
    rec.p         = Point3(0.0, 0.0, 0.0);
    rec.normal    = Vec3(0.0, 0.0, 1.0);
    rec.t         = 0.0;
    rec.u         = 0.5;
    rec.v         = 0.5;
    rec.front_face = true;
    rec.mat       = m;
    return rec;
}

// 半球数值积分：∫ pdf(ω) sinθ dθ dφ
//   (θ, φ) 用中点法则。被积函数光滑 → 误差约 1e-6 量级。
static double integrate_pdf(const Material& mat, const HitRecord& rec) {
    const int    NT = 400, NP = 800;
    const double dth = (0.5 * PI_C) / NT;
    const double dph = (2.0 * PI_C) / NP;

    const Ray r_in(Point3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -1.0));  // 任意入射方向，Lambertian 不看它

    double sum = 0.0;
    for (int i = 0; i < NT; ++i) {
        const double th = (i + 0.5) * dth;
        const double st = std::sin(th), ct = std::cos(th);
        for (int j = 0; j < NP; ++j) {
            const double ph = (j + 0.5) * dph;
            const Vec3   w(st * std::cos(ph), st * std::sin(ph), ct);   // 已是单位向量
            const Ray    scattered(rec.p, w);
            sum += mat.scattering_pdf(r_in, rec, scattered) * st * dth * dph;
        }
    }
    return sum;
}

// 一个方向上的 pdf
static double pdf_at(const Material& mat, const HitRecord& rec, double theta_deg) {
    const double th = theta_deg * PI_C / 180.0;
    const Vec3   w(std::sin(th), 0.0, std::cos(th));
    const Ray    r_in(Point3(0.0, 0.0, 1.0), Vec3(0.0, 0.0, -1.0));
    const Ray    scattered(rec.p, w);
    return mat.scattering_pdf(r_in, rec, scattered);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 显式 PDF（Day 29） =====\n";

    const Color albedo(0.6, 0.4, 0.2);

    // ========================================================
    // 组 U1~U3：余弦加权采样（默认）
    //   pdf(ω) = cosθ / π
    // ========================================================
    std::cout << "\n--- 组 U1~U3：余弦加权  pdf = cosθ/π ---\n";

    g_uniform_hemisphere = false;
    auto lam = std::make_shared<Lambertian>(albedo);
    HitRecord rec = make_rec(lam);

    check_near("U1 pdf(θ=0°)  == 1/π", pdf_at(*lam, rec, 0.0), 1.0 / PI_C, 1e-12);
    check_near("U2 pdf(θ=60°) == cos60°/π = 0.5/π", pdf_at(*lam, rec, 60.0),
               0.5 / PI_C, 1e-12);

    const double I_cos = integrate_pdf(*lam, rec);
    std::cout << "        ∫ cosθ/π dω = " << I_cos << "   （应为 1）\n";
    check_near("U3 ∫ pdf dω == 1（余弦分布已归一化）", I_cos, 1.0, 1e-4);

    // ========================================================
    // 组 U4~U5：均匀半球采样
    //   pdf(ω) = 1/(2π)   处处相同
    // ========================================================
    std::cout << "\n--- 组 U4~U5：均匀半球  pdf = 1/(2π) ---\n";

    g_uniform_hemisphere = true;
    check_near("U4 pdf(θ=0°)  == 1/(2π)", pdf_at(*lam, rec, 0.0), 1.0 / (2.0 * PI_C), 1e-12);
    check_near("U4 pdf(θ=80°) == 1/(2π)（与角度无关）", pdf_at(*lam, rec, 80.0),
               1.0 / (2.0 * PI_C), 1e-12);

    const double I_uni = integrate_pdf(*lam, rec);
    std::cout << "        ∫ 1/(2π) dω = " << I_uni << "   （应为 1）\n";
    check_near("U5 ∫ pdf dω == 1（均匀分布已归一化）", I_uni, 1.0, 1e-4);

    g_uniform_hemisphere = false;   // 恢复默认

    // ========================================================
    // 组 U6：镜面材质没有 pdf
    // ========================================================
    std::cout << "\n--- 组 U6：镜面是 δ 分布 ---\n";

    auto met = std::make_shared<Metal>(Color(0.9, 0.9, 0.9), 0.0);
    HitRecord mrec = make_rec(met);
    check_near("U6 Metal 的 scattering_pdf == 0（δ 分布没有有限 pdf）",
               pdf_at(*met, mrec, 0.0), 0.0, 0.0);

    // ========================================================
    // 组 U7~U9：显式化的核心不变式
    //   attenuation 只是 BRDF，不再吃 cos/pdf
    // ========================================================
    std::cout << "\n--- 组 U7~U9：attenuation 不再随采样策略变化 ---\n";

    auto do_scatter = [&](bool uniform_hs, Color& atten, Ray& out) {
        g_uniform_hemisphere = uniform_hs;
        HitRecord r = make_rec(lam);
        bool ok = lam->scatter(Ray(Point3(0, 0, 1), Vec3(0, 0, -1)), r, atten, out);
        g_uniform_hemisphere = false;
        return ok;
    };

    Color atten_cos(0, 0, 0), atten_uni(0, 0, 0);
    Ray   out_cos, out_uni;
    do_scatter(false, atten_cos, out_cos);
    do_scatter(true,  atten_uni, out_uni);

    // 余弦采样下：attenuation 应为 albedo/π
    check_near("U7 余弦采样的 attenuation(R) == albedo.r/π",
               atten_cos.x(), albedo.x() / PI_C, 1e-12);
    check_near("U8 均匀半球的 attenuation(R) == albedo.r/π（和采样方式无关！）",
               atten_uni.x(), albedo.x() / PI_C, 1e-12);

    // 关键恒等式：attenuation · cosθ / pdf == albedo
    //   余弦采样： (albedo/π) · cos / (cos/π) = albedo
    {
        const double th = 30.0 * PI_C / 180.0;
        const Vec3   w(std::sin(th), 0.0, std::cos(th));
        const double cos_t = w.z();
        const Ray    scattered(rec.p, w);
        const double pdf   = lam->scattering_pdf(Ray(rec.p, Vec3(0, 0, -1)), rec, scattered);
        const double tp    = atten_cos.x() * cos_t / pdf;
        std::cout << "        θ=30°: attenuation·cos/pdf = " << tp
                  << "   albedo.r = " << albedo.x() << '\n';
        check_near("U9 attenuation·cosθ/pdf == albedo（R 通道，θ=30°）",
                   tp, albedo.x(), 1e-12);
    }

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) std::cout << "ALL PASSED\n";
    return g_fail == 0 ? 0 : 1;
}
