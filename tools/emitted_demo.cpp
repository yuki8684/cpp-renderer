// ============================================================
// emitted_demo.cpp —— 最小可运行演示（Day 16 补充）
//
// 目的：把 ray_color 的递归【剥掉光线追踪的全部外壳】，
//       只留下"能量是怎么一层层累加的"，然后用真实数字跑给你看。
//
// 为什么能剥？
//   ray_color 的数学结构只有三样东西：
//       emitted      自己发光多少
//       attenuation  打几折（乘数）
//       scatter()    要不要继续递归
//   颜色和方向只是让画面好看，对"为什么全黑"这件事毫无影响。
//   所以下面把 Color 换成单个 float，把 ray 换成"预设的命中路线"。
//
// 编译运行（项目根目录）:
//   g++ -std=c++17 -O2 -o build/emitted_demo.exe tools/emitted_demo.cpp
//   .\build\emitted_demo.exe
// ============================================================

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

// ---------- 简化版"材质" ----------
struct Mat {
    const char* name;    // 名字，方便看输出
    float emitted;       // 对应 emitted()：自己发光多少
    float albedo;        // 对应 attenuation：打几折
    bool  scatters;      // 对应 scatter() 的返回值：要不要继续弹
};

// ---------- 和 ray_color 结构【完全一样】的递归 ----------
//
//   ray_color 里是：  emitted + attenuation * ray_color(depth - 1)
//   这里就是：        m.emitted + m.albedo * trace(level - 1)
//
// route 按【光线实际命中顺序】排列；level 就是 ray_color 的 depth。
//   索引写成 route[DEPTH - level]，这样 level=DEPTH 时命中 route[0]（第一次命中）
static const int DEPTH = 3;

static float trace(const Mat* const* route, int level, int indent) {
    for (int i = 0; i < indent; ++i) std::printf("  ");

    // ---- 递归出口：和 ray_color 开头那句 if (depth <= 0) return Color(0,0,0); 一样 ----
    if (level == 0) {
        std::printf("[depth 用完] ——> 返回 0.000\n");
        return 0.0f;
    }

    const Mat& m = *route[DEPTH - level];
    std::printf("层%d %-14s emitted=%-6.2f albedo=%-6.2f scatter=%s",
                level, m.name, m.emitted, m.albedo, m.scatters ? "true " : "false");

    // ---- 不散射：路径【在这里终止】，不进入递归 ----
    if (!m.scatters) {
        std::printf("   ——> 路径终止，直接交卷返回 %.3f\n", m.emitted);
        return m.emitted;
    }

    // ---- 会散射：继续往里递归，拿到里面那一层的结果再乘 albedo ----
    std::printf("\n");
    float inner = trace(route, level - 1, indent + 1);

    for (int i = 0; i < indent; ++i) std::printf("  ");
    float result = m.emitted + m.albedo * inner;
    std::printf("   ↑ 层%d 汇总: %.2f + %.2f × %.4f = %.4f\n",
                level, m.emitted, m.albedo, inner, result);
    return result;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 一条相机光线的命中顺序：地面 → 天花板那块"灯" → 后墙
    // （route 按命中顺序排，索引 = DEPTH - level）
    const Mat floor_m   { "地面",      0.0f,  0.73f, true };
    const Mat back_m    { "后墙",      0.0f,  0.73f, true };
    const Mat lam_light { "灯(哑光)",  0.0f, 15.00f, true };   // Lambertian(15,15,15)
    const Mat emit_light{ "灯(自发光)", 15.0f,  0.00f, false }; // DiffuseLight(15,15,15)

    const Mat* routeA[DEPTH] = { &floor_m, &lam_light,  &back_m };
    const Mat* routeB[DEPTH] = { &floor_m, &emit_light, &back_m };

    std::printf("\n================ 场景 A：灯 = Lambertian(15,15,15) ================\n");
    std::printf("（albedo 15 是反射率，emitted 0 表示【不发光】）\n\n");
    float a = trace(routeA, DEPTH, 0);
    std::printf("\n  ==> 这条光线最终亮度 = %.3f   （也就是 0，纯黑）\n", a);

    std::printf("\n================ 场景 B：灯 = DiffuseLight(15,15,15) ==============\n");
    std::printf("（emitted 15 表示【自己发光】，scatter=false 表示路径到此为止）\n\n");
    float b = trace(routeB, DEPTH, 0);
    std::printf("\n  ==> 这条光线最终亮度 = %.3f   （有光了！）\n", b);

    std::printf("\n================ 关键对比 ================\n");
    std::printf("  场景 A：最内层永远是 0，乘什么都没用                → %.3f\n", a);
    std::printf("  场景 B：递归在\"灯\"那一层【就停了】，那里交出了 15   → %.3f\n", b);
    std::printf("  差 %.1f 倍，但根因只有一条：emitted 有没有非零值。\n\n", b / (a == 0.0f ? 1.0f : a));

    return 0;
}
