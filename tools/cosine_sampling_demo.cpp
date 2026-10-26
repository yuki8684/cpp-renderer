// ============================================================
// cosine_sampling_demo.cpp —— 余弦加权采样到底是不是"余弦加权"？
//
// 目的：用 100 万个样本，实测两种半球采样器的 E[cos θ]，
//       和解析解对比，把"余弦加权"这件事钉死。
//
// 理论值（法线方向取 +y，θ 是采样方向与法线的夹角）：
//
//   余弦加权采样（pdf = cos θ / π）：
//      E[cos θ] = ∫ cosθ · (cosθ/π) dω = (1/π) ∫ cos²θ dω = (1/π)(2π/3) = 2/3 ≈ 0.6667
//
//   均匀半球采样（pdf = 1/(2π)）：
//      E[cos θ] = (1/2π) ∫ cosθ dω = (1/2π)(π) = 1/2 = 0.5
//
// 编译运行（项目根目录）:
//   g++ -std=c++17 -O2 -Wall -Wextra -I include -o build/cosine_demo.exe tools/cosine_sampling_demo.cpp
//   .\build\cosine_demo.exe
// ============================================================

#include <cstdio>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"

// -------- 均匀半球采样：把下半球的点翻上来 --------
// pdf 是常数 1/(2π)
inline Vec3 random_on_hemisphere(const Vec3& n) {
    Vec3 v = random_unit_vector();
    return (dot(v, n) > 0.0) ? v : -v;
}

// -------- 余弦加权采样：RTIOW 用了十几天的写法 --------
// n + 随机单位球面点，再归一化
inline Vec3 cosine_hemisphere_rtiow(const Vec3& n) {
    Vec3 d = n + random_unit_vector();
    double len2 = d.length_squared();
    if (len2 < 1e-12) return n;          // 极小概率撞到反方向，退化成一个方向
    return unit_vector(d);
}

struct Result {
    double cos_mean;    // E[cos θ]
    double w_mean;      // E[权重]
    double w_std;       // 权重的标准差
    double grazing;     // cos < 0.1 的样本占比
};

// -------- 统计：同时量 cos 的均值和"估计量权重"的分布 --------
// Lambertian 的 BRDF 是 f = albedo/π。取 albedo = 1 → f = 1/π。
// 估计量权重 w = f · cosθ / pdf
template <typename Sampler, typename PdfFn>
static Result measure(Sampler sample, PdfFn pdf_of_cos, int N) {
    const Vec3 n(0, 1, 0);

    double sc = 0.0;                 // sum cos
    double sw = 0.0, sw2 = 0.0;      // sum w, sum w^2
    int grazing = 0;

    for (int i = 0; i < N; ++i) {
        Vec3   d   = sample(n);
        double c   = dot(d, n);
        double pdf = pdf_of_cos(c);
        double w   = (1.0 / pi) * c / pdf;

        sc += c;
        sw += w;
        sw2 += w * w;
        if (c < 0.1) ++grazing;      // 掠射方向：几乎不贡献，但一样花钱
    }

    Result r;
    r.cos_mean = sc / N;
    r.w_mean   = sw / N;
    double var = sw2 / N - r.w_mean * r.w_mean;
    r.w_std    = std::sqrt(var > 0.0 ? var : 0.0);
    r.grazing  = double(grazing) / N;
    return r;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    const int N = 1000000;
    reset_rng();

    // ---------- A：余弦加权（pdf = cos/π）----------
    Result a = measure(cosine_hemisphere_rtiow,
                       [](double c) { return c / pi; }, N);

    // ---------- B：均匀半球（pdf = 1/2π）----------
    Result b = measure(random_on_hemisphere,
                       [](double) { return 1.0 / (2.0 * pi); }, N);

    std::printf("\n========== %d 个样本 ==========\n\n", N);

    std::printf("【A】余弦加权采样   pdf = cos(theta)/pi\n");
    std::printf("      E[cos theta]         = %.5f   (理论 2/3 = %.5f)\n", a.cos_mean, 2.0 / 3.0);
    std::printf("      E[权重]              = %.5f   (理论 1.0)\n", a.w_mean);
    std::printf("      权重的标准差          = %.6f   <-- 几乎为 0\n", a.w_std);
    std::printf("      掠射区 (cos<0.1) 占比 = %.2f%%\n\n", a.grazing * 100.0);

    std::printf("【B】均匀半球采样   pdf = 1/(2*pi)\n");
    std::printf("      E[cos theta]         = %.5f   (理论 1/2 = %.5f)\n", b.cos_mean, 0.5);
    std::printf("      E[权重]              = %.5f   (理论 1.0)\n", b.w_mean);
    std::printf("      权重的标准差          = %.6f   (理论 1/sqrt3 = %.6f)\n", b.w_std, 1.0 / std::sqrt(3.0));
    std::printf("      掠射区 (cos<0.1) 占比 = %.2f%%\n\n", b.grazing * 100.0);

    std::printf("========== 结论 ==========\n");
    std::printf("1) RTIOW 的  n + random_unit_vector()\n");
    std::printf("   实测 E[cos] = %.4f  ≈  2/3  →  【它确实是余弦加权】\n", a.cos_mean);
    std::printf("2) 余弦加权的估计量权重是【常数】，标准差 ≈ 0；\n");
    std::printf("   均匀半球的权重在 0~2 之间抖，标准差 %.3f。\n", b.w_std);
    std::printf("   两者的权重【期望都是 1.0】—— 所以画面亮度一样，只有噪声不同。\n");
    std::printf("3) 均匀半球有 %.1f%% 的样本落在掠射区（几乎不贡献），\n", b.grazing * 100.0);
    std::printf("   余弦加权只有 %.1f%% —— 浪费少约 %.0f 倍。\n\n",
                a.grazing * 100.0, b.grazing / (a.grazing > 0 ? a.grazing : 1e-9));
    return 0;
}
