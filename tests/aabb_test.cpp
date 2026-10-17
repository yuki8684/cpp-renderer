// ============================================================
// AABB 单元测试（独立可执行文件）
//
// 为什么要把测试从 main.cpp 里独立出来？
//   1. 测的是"算法"本身，和渲染流程无关 —— 不该混在 main.cpp 里
//   2. 编译只要 1 秒，不用等 13 秒的渲染
//   3. 改完 aabb.h 可以立刻单独验证，不用管其他还没写完的代码
//
// 编译运行（在项目根目录）:
//   g++ -std=c++17 -O2 -I include -o build/aabb_test.exe tests/aabb_test.cpp
//   .\build\aabb_test.exe
//
// 或者用脚本:  .\tools\test.ps1
// ============================================================

#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "vec3.h"
#include "ray.h"
#include "aabb.h"

static int g_pass = 0;
static int g_fail = 0;

static void check(const char* name, bool got, bool want) {
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

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== AABB 单元测试 =====\n";

    // 一个棱长为 2、中心在原点的立方体：x / y / z 都在 [-1, 1]
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));

    // ---------- 基本情形 ----------
    check("正前方命中",  box.hit(Ray(Point3(0, 0, -5), Vec3( 0, 0,  1)), 0.001, infinity), true);
    check("正前方打偏",  box.hit(Ray(Point3(2, 0, -5), Vec3( 0, 0,  1)), 0.001, infinity), false);
    check("从背后命中",  box.hit(Ray(Point3(0, 0,  5), Vec3( 0, 0, -1)), 0.001, infinity), true);
    check("背对盒子",    box.hit(Ray(Point3(0, 0,  5), Vec3( 0, 0,  1)), 0.001, infinity), false);
    check("起点在盒内",  box.hit(Ray(Point3(0, 0,  0), Vec3( 0, 0,  1)), 0.001, infinity), true);

    // ---------- 斜射 ----------
    // 起点 (1.5, 0.5, -5)，方向 (-0.5, 0, 1)
    //   x 板: D 为负 → 交换后 t ∈ [1, 5]
    //   y 板: D_y = 0，起点 y = 0.5 在板内 → 无约束
    //   z 板: t ∈ [4, 6]
    //   交集 = [max(0.001, 1, 4), min(inf, 5, 6)] = [4, 5] → 命中
    check("斜射命中",       box.hit(Ray(Point3(1.5, 0.5, -5), Vec3(-0.5, 0, 1)), 0.001, infinity), true);
    // 起点抬到 y = 1.5（超出 y 板）→ y 板给出空集 → 不命中
    check("斜射从上方掠过", box.hit(Ray(Point3(1.5, 1.5, -5), Vec3(-0.5, 0, 1)), 0.001, infinity), false);

    // ---------- 除零专项（D = 0 的三种命运）----------
    check("Dx=Dy=0 从盒内射出", box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 0.001, infinity), true);
    check("Dx=Dy=0 从盒外射出", box.hit(Ray(Point3(2, 0, -5), Vec3(0, 0, 1)), 0.001, infinity), false);
    // 起点【恰好】落在 x = 1 这个面上 → 0 × inf = NaN，靠 NaN 的比较规则化解
    check("起点恰在面上",       box.hit(Ray(Point3(1, 0, -5), Vec3(0, 0, 1)), 0.001, infinity), true);
    check("起点恰在角上",       box.hit(Ray(Point3(1, 1, -5), Vec3(0, 0, 1)), 0.001, infinity), true);
    // 反向的零：-0.0。1/(-0.0) = -infinity，会走 swap 分支
    check("方向为负零",         box.hit(Ray(Point3(0, 0, -5), Vec3(-0.0, 0, 1)), 0.001, infinity), true);

    // ---------- t 区间限制 ----------
    // 盒子在 t ∈ [4, 6] 处
    check("t_min 之前才到",  box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 10.0,  infinity), false);
    check("t_max 之后才到",  box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 0.001, 1.0),     false);
    check("窗口刚好覆盖",    box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 0.001, 7.0),     true);
    check("窗口刚好卡掉",    box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 6.0,   7.0),     false);

    // ---------- 退化盒子：某个轴厚度为 0 ----------
    // Day 8 这里曾是【已知局限】：厚度 0 的轴会得到 [t, t]（单点），而我们的判据是
    //    t_离开 <= t_进入 就算没命中 → 所以这种"薄片"永远打不中。
    // 当时只有球，球永远不会产生厚度 0 的盒子，所以只把这件事记了下来。
    //
    // ★ Day 13 需求变了：加入 Quad 之后，包围盒必然有一个轴是 0 厚度。
    //   若还按老期望值，后果不是"四边形不出现"，而是
    //   【四边形只在用 BVH 时才整片消失】—— 裸遍历根本不查包围盒，所以完全正常。
    //   这是最难查的一类 bug：错在"加速结构"，不在"几何体"，而且没有任何报错。
    //   所以 AABB 构造函数里加了 pad_to_minimums()，把 0 厚度撑到 0.0001。
    //
    //   → 期望值从 false 改成 true，是【需求驱动】的改动，不是为了让测试变绿。
    //     （把红色直接改成实际值、不追问原因，等于把警报器拆了。）
    AABB flat(Point3(-1, -1, 0), Point3(1, 1, 0));
    check("薄片(厚度0)已被 padding 撑开，可命中",
          flat.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 0.001, infinity), true);
    check("薄片从另一侧也能命中",
          flat.hit(Ray(Point3(0, 0, 5), Vec3(0, 0, -1)), 0.001, infinity), true);
    check("padding 后 z 下界 = -0.00005",
          std::fabs(flat.minimum.z() + 0.00005) < 1e-12, true);
    check("padding 后 z 上界 = +0.00005",
          std::fabs(flat.maximum.z() - 0.00005) < 1e-12, true);
    // padding 只能【撑大】，绝不能【缩小】—— 偏大只是慢，偏小会丢命中
    check("padding 不动已经很厚的轴 (x)",
          std::fabs(flat.minimum.x() - (-1.0)) < 1e-12, true);
    check("padding 不动已经很厚的轴 (y)",
          std::fabs(flat.maximum.y() - ( 1.0)) < 1e-12, true);

    // ---------- 极端退化：三个轴全是 0 厚度（一个点）----------
    AABB dot_box(Point3(0, 0, 0), Point3(0, 0, 0));
    check("点盒子三个轴都被撑成 0.0001",
          std::fabs(dot_box.maximum.x() - dot_box.minimum.x() - 0.0001) < 1e-12 &&
          std::fabs(dot_box.maximum.y() - dot_box.minimum.y() - 0.0001) < 1e-12 &&
          std::fabs(dot_box.maximum.z() - dot_box.minimum.z() - 0.0001) < 1e-12, true);
    check("点盒子可被命中",
          dot_box.hit(Ray(Point3(0, 0, -5), Vec3(0, 0, 1)), 0.001, infinity), true);

    // ---------- surrounding（并集）----------
    AABB left(Point3(-3, -1, -1), Point3(-1, 1, 1));
    AABB right(Point3( 1, -1, -1), Point3( 3, 1, 1));
    AABB both = AABB::surrounding(left, right);
    check("并集 min.x", both.minimum.x() == -3.0, true);
    check("并集 max.x", both.maximum.x() ==  3.0, true);
    check("并集 min.y", both.minimum.y() == -1.0, true);
    check("并集 max.y", both.maximum.y() ==  1.0, true);

    // ---------- 构造函数的自动纠序 ----------
    AABB swapped(Point3(3, 3, 3), Point3(-1, -1, -1));   // 故意"反着"传
    check("自动纠序 min", swapped.minimum.x() == -1.0, true);
    check("自动纠序 max", swapped.maximum.x() ==  3.0, true);

    std::cout << "\n  === 结果：" << g_pass << " 通过, " << g_fail << " 失败 ===\n\n";

    return g_fail == 0 ? 0 : 1;
}
