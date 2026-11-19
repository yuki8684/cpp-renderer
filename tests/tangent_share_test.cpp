// ============================================================
// 切线帧跨三角形共享（Day 27）
//
// 要验证的四件事：
//   ① 顶点切线的基本性质：单位长度、垂直于顶点法线
//   ② 顶点切线 == 解析真值 ∂P/∂u（圆环有解析解，可以硬碰硬）
//   ③ 面内插值的权重对（和 Day 15 法线 / Day 22 UV 同一组重心权重）
//   ④ 退化兜底：没有 UV 的模型不崩、不产生垃圾数据
//
// ★ ② 是最有分辨力的：圆环的 ∂P/∂u 只跟 u 参数有关，可以精确算出来。
//   而"每个三角形独立解"的切线是分片常数的，会偏离真值 —— 这正是
//   今天要消灭的接缝。
//
// 圆环的参数方程（tools/gen_obj.ps1 里就是这么生成的）：
//   P(u,v) = ( (R + r·cos v)·cos u,  r·sin v,  (R + r·cos v)·sin u )
//   ∂P/∂u  = ( −(R + r·cos v)·sin u,  0,  (R + r·cos v)·cos u )
//   方向    = ( −sin u,  0,  cos u )        ← 只跟 u 有关
//
// 顶点索引 -> 参数：(0-based idx)  i = idx / NV,  j = idx % NV
//                   u = 2π·i/NU,    v = 2π·j/NV
// ============================================================

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "material.h"
#include "obj_loader.h"

static const double PI = 3.14159265358979323846;

// torus.obj 的生成参数（必须和 tools/gen_obj.ps1 保持一致）
static const int    TORUS_NU = 48;
static const int    TORUS_NV = 24;
static const double TORUS_R  = 1.0;      // RingR
static const double TORUS_r  = 0.38;     // TubeR

static int g_pass = 0;
static int g_fail = 0;

static void check_bool(const char* name, bool got, bool want = true) {
    if (got == want) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << (got ? "true" : "false")
                  << "  期望=" << (want ? "true" : "false") << '\n';
    }
}

static void check_int(const char* name, long long got, long long want) {
    if (got == want) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else {
        ++g_fail;
        std::cout << "  [FAIL] " << name << "    得到=" << got << "  期望=" << want << '\n';
    }
}

static void check_less(const char* name, double got, double limit, const char* unit) {
    if (got <= limit) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << got << unit << "  上限=" << limit << unit << '\n';
    }
}

static void check_vec(const char* name, const Vec3& got, const Vec3& want, double tol = 1e-9) {
    const bool ok = std::fabs(got.x() - want.x()) <= tol &&
                    std::fabs(got.y() - want.y()) <= tol &&
                    std::fabs(got.z() - want.z()) <= tol;
    if (ok) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=(" << got.x() << ", " << got.y() << ", " << got.z() << ")"
                  << "  期望=(" << want.x() << ", " << want.y() << ", " << want.z() << ")\n";
    }
}

// 两个单位向量之间的夹角（度）
static double angle_deg(const Vec3& a, const Vec3& b) {
    double c = dot(unit_vector(a), unit_vector(b));
    if (c > 1.0)  c = 1.0;
    if (c < -1.0) c = -1.0;
    return std::acos(c) * 180.0 / PI;
}

// 第 idx 个顶点处，解析的 ∂P/∂u 方向
static Vec3 analytic_T(std::size_t idx) {
    const int i = int(idx) / TORUS_NV;
    const double u = 2.0 * PI * i / TORUS_NU;
    return unit_vector(Vec3(-std::sin(u), 0.0, std::cos(u)));
}

// 测试替身：只满足 Material 接口，永远不散射
class NullMaterial : public Material {
public:
    bool scatter(const Ray&, const HitRecord&, Color&, Ray&) const override { return false; }
};

// 用三角形自己的 3 个顶点 + UV 独立解一次切线（Day 25 的 2×2），
// 不经过 compute_vertex_tangents —— 用来量化"共享前的接缝"
static bool tri_own_tangent(const ObjData& d, std::size_t i, Vec3& T_out) {
    const Point3& a = d.positions[std::size_t(d.tri_indices[i + 0])];
    const Point3& b = d.positions[std::size_t(d.tri_indices[i + 1])];
    const Point3& c = d.positions[std::size_t(d.tri_indices[i + 2])];
    const UV& u0 = d.tri_uvs[i + 0];
    const UV& u1 = d.tri_uvs[i + 1];
    const UV& u2 = d.tri_uvs[i + 2];

    const Vec3 e1 = b - a, e2 = c - a;
    const double du1 = u1.u - u0.u, dv1 = u1.v - u0.v;
    const double du2 = u2.u - u0.u, dv2 = u2.v - u0.v;
    const double det = du1 * dv2 - du2 * dv1;
    if (!(std::fabs(det) > 1e-20)) return false;

    T_out = ( dv2 * e1 - dv1 * e2) / det;      // 未归一化
    return true;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 切线帧跨三角形共享（Day 27） =====\n";

    ObjData d = load_obj("models/torus.obj");
    if (d.tri_indices.empty()) {
        std::cout << "  [FAIL] 读不到 models/torus.obj\n";
        return 1;
    }

    const std::size_t nv = d.positions.size();
    std::cout << "  torus: " << nv << " 顶点, "
              << d.tri_indices.size() / 3 << " 三角形, "
              << d.tri_uvs.size() / 3 << " 带 UV 的三角形\n";

    // ========================================================
    // 组 S1~S3：顶点切线的基本性质
    // ========================================================
    std::cout << "\n--- 组 S1~S3：基本性质 ---\n";

    check_int("S1 d.tangents.size() == 1152",
              static_cast<long long>(d.tangents.size()),
              static_cast<long long>(nv));

    if (d.tangents.size() != nv) {
        std::cout << "  （后面全部跳过 —— 切线数组大小就不对）\n";
        std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
                  << g_pass << " 通过，" << g_fail << " 失败 =====\n";
        return 1;
    }

    double max_unit_err = 0.0;
    double max_perp_err = 0.0;
    bool   any_zero     = false;
    for (std::size_t i = 0; i < nv; ++i) {
        const double L = d.tangents[i].length();
        // ⚠️ 零向量【不能跳过】—— 跳过了 |L−1| 就是 0，测试会假绿。
        //    零向量的 |L−1| = 1，会如实报红。
        max_unit_err = std::fmax(max_unit_err, std::fabs(L - 1.0));
        if (!(L > 0.0)) { any_zero = true; continue; }
        max_perp_err = std::fmax(max_perp_err, std::fabs(dot(d.tangents[i], d.normals[i])));
    }

    check_bool("S2 没有零向量（每个顶点都累加到了东西）", !any_zero);
    check_less("S3 全部是单位向量（| |t| − 1 | 的最大值）", max_unit_err, 1e-12, "");
    // ⚠️ dot(零向量, n) 恒为 0，会假绿 → 把 any_zero 一起纳入判据
    check_bool("S4 全部垂直于顶点法线（且无零向量）",
               !any_zero && max_perp_err <= 1e-9);

    // ========================================================
    // 组 S5~S6：和解析真值对比（今天最有分辨力的两条）
    // ========================================================
    std::cout << "\n--- 组 S5~S6：和解析真值 ∂P/∂u 对比 ---\n";

    double max_shared_dev = 0.0;
    for (std::size_t i = 0; i < nv; ++i) {
        if (!(d.tangents[i].length() > 0.0)) continue;   // 零向量算不了夹角
        max_shared_dev = std::fmax(max_shared_dev, angle_deg(d.tangents[i], analytic_T(i)));
    }
    std::cout << "        共享切线 vs 解析真值：最大夹角 = " << max_shared_dev << " 度\n";
    // 实测（正确实现）：最大 0.215 度，平均 0.121 度。阈值取 0.5 度留余量。
    // ⚠️ 没有可比的向量时（全零）不能算“夹角 0 度”—— 一起纳入判据
    check_bool("S5 顶点切线 == ∂P/∂u（夹角 ≤ 0.5 度，且无零向量）",
               !any_zero && max_shared_dev <= 0.5);

    // 对照：每个三角形独立解的切线，在同一顶点上偏离真值多少
    double max_own_dev = 0.0;
    for (std::size_t i = 0; i + 2 < d.tri_indices.size(); i += 3) {
        Vec3 T;
        if (!tri_own_tangent(d, i, T)) continue;
        // 用三角形的第一个顶点作为对比点
        const std::size_t ia = std::size_t(d.tri_indices[i + 0]);
        max_own_dev = std::fmax(max_own_dev, angle_deg(T, analytic_T(ia)));
    }
    std::cout << "        每三角形独立解 vs 解析真值：最大夹角 = " << max_own_dev << " 度\n";
    std::cout << "        → 实测：独立解 3.7559 度 → 共享后 0.215 度，好 17.4 倍\n";
    check_bool("S6 共享确实比每三角形独立解更准（或至少不差）",
               max_shared_dev <= max_own_dev + 1e-9);

    // ========================================================
    // 组 S7~S9：面内插值
    // ========================================================
    std::cout << "\n--- 组 S7~S9：面内插值 ---\n";

    auto mat = std::make_shared<NullMaterial>();

    const std::size_t ia = std::size_t(d.tri_indices[0]);
    const std::size_t ib = std::size_t(d.tri_indices[1]);
    const std::size_t ic = std::size_t(d.tri_indices[2]);

    Triangle tri(d.positions[ia], d.positions[ib], d.positions[ic],
                 d.normals[ia], d.normals[ib], d.normals[ic], mat);
    tri.set_uv(d.tri_uvs[0], d.tri_uvs[1], d.tri_uvs[2]);
    tri.set_vertex_tangents(d.tangents[ia], d.tangents[ib], d.tangents[ic]);

    check_vec("S7 顶点 a 处 (α=0, β=0) → tan0", tri.tangent_at(0.0, 0.0), d.tangents[ia], 1e-12);
    check_vec("S8 顶点 b 处 (α=1, β=0) → tan1", tri.tangent_at(1.0, 0.0), d.tangents[ib], 1e-12);
    check_vec("S9 顶点 c 处 (α=0, β=1) → tan2", tri.tangent_at(0.0, 1.0), d.tangents[ic], 1e-12);

    // 重心处：三个权重都是 1/3
    const Vec3 want_centroid = (1.0 / 3.0) * (d.tangents[ia] + d.tangents[ib] + d.tangents[ic]);
    check_vec("S10 重心 (1/3, 1/3) → (tan0+tan1+tan2)/3",
              tri.tangent_at(1.0 / 3.0, 1.0 / 3.0), want_centroid, 1e-12);

    // ========================================================
    // 组 S11~S12：退化兜底
    // ========================================================
    std::cout << "\n--- 组 S11~S12：退化兜底 ---\n";

    ObjData plain = load_obj("models/cube.obj");       // 没有 vt
    check_bool("S11 cube.obj 没有 UV → tangents 为空（不崩）", plain.tangents.empty());

    ObjData withuv = load_obj("models/cube_uv.obj");   // 有 vt
    check_int("S12 cube_uv.obj 有 UV → tangents 有 8 个",
              static_cast<long long>(withuv.tangents.size()), 8);

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) std::cout << "ALL PASSED\n";
    return g_fail == 0 ? 0 : 1;
}
