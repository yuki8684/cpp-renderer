// ============================================================
// P6 二进制 PPM 读取（Day 28）
//
// 要验证的四件事：
//   ① P6 和 P3 读出来【完全一样】—— 这是唯一真正的判据
//   ② 1 字节 / 2 字节样本的两种分支都能走通
//   ③ 2 字节是【大端】：256 和 1 不能互换
//   ④ 截断的文件要报错，不能静默留黑
//
// ★ 测 ① 的办法是"同一张图两种编码"：
//     models/uvgrid.ppm      P3 (ASCII)   686,802 B
//     models/uvgrid_p6.ppm   P6 (binary)  196,623 B
//   逐像素比，任何一处错位/端序错都会立刻炸出来。
//
// 测试素材全部由 tools/gen_texture.ps1 生成：
//   uvgrid.ppm       256x256  maxval 255
//   uvgrid_p6.ppm    256x256  maxval 255   （和上面同一张图）
//   p6_max15.ppm       4x2    maxval 15    （1 字节/样本）
//   p6_max65535.ppm    4x2    maxval 65535 （2 字节/样本，大端）
//   p6_truncated.ppm   8x8    头部说 192 字节数据，实际只有 100
// ============================================================

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "rtweekend.h"
#include "color.h"
#include "image.h"

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

static long long file_size(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return -1;
    return static_cast<long long>(f.tellg());
}

// 逐像素比较两张图，返回三个通道里最大的绝对差
static double max_abs_diff(const Image& a, const Image& b) {
    if (a.pixels.size() != b.pixels.size()) return 1.0e9;
    double m = 0.0;
    for (std::size_t i = 0; i < a.pixels.size(); ++i) {
        m = std::fmax(m, std::fabs(a.pixels[i].x() - b.pixels[i].x()));
        m = std::fmax(m, std::fabs(a.pixels[i].y() - b.pixels[i].y()));
        m = std::fmax(m, std::fabs(a.pixels[i].z() - b.pixels[i].z()));
    }
    return m;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== P6 二进制 PPM（Day 28） =====\n";

    // ========================================================
    // 组 P1~P5：同一张图，两种编码
    // ========================================================
    std::cout << "\n--- 组 P1~P5：P6 必须和 P3 读出来一模一样 ---\n";

    Image p3 = load_ppm("models/uvgrid.ppm", /* srgb = */ true);
    Image p6 = load_ppm("models/uvgrid_p6.ppm", /* srgb = */ true);

    check_bool("P1 P6 加载成功且 valid()", p6.valid());
    check_int("P2 P6 宽度 == 256", p6.width, 256);
    check_int("P3 P6 像素数 == 65536",
              static_cast<long long>(p6.pixels.size()), 65536);

    if (p6.valid() && p3.valid()) {
        const double d = max_abs_diff(p3, p6);
        std::cout << "        P3 与 P6 的最大像素差 = " << d << '\n';
        check_bool("P4 P3 与 P6 逐像素完全相同（差 < 1e-12）", d < 1e-12);
    } else {
        std::cout << "  [FAIL] P4 跳过 —— 有一张图没读进来\n";
        ++g_fail;
    }

    const long long sz3 = file_size("models/uvgrid.ppm");
    const long long sz6 = file_size("models/uvgrid_p6.ppm");
    if (sz3 > 0 && sz6 > 0) {
        const double ratio = double(sz3) / double(sz6);
        std::cout << "        P3 = " << sz3 << " B, P6 = " << sz6 << " B, 比值 = "
                  << ratio << "x\n";
        check_bool("P5 P6 至少比 P3 小 3 倍", ratio >= 3.0);
    } else {
        std::cout << "  [FAIL] P5 读不到文件大小\n";
        ++g_fail;
    }

    // ========================================================
    // 组 P6~P7：maxval 的两种情形
    // ========================================================
    std::cout << "\n--- 组 P6~P7：1 字节 vs 2 字节样本 ---\n";

    // maxval = 15  ->  1 字节/样本
    //   数据（4x2，行优先）：
    //     ( 0, 1, 2) ( 3, 4, 5) ( 6, 7, 8) ( 9,10,11)
    //     (12,13,14) (15, 0, 1) ( 2, 3, 4) ( 5, 6, 7)
    {
        const int want[24] = { 0, 1, 2,  3, 4, 5,  6, 7, 8,  9,10,11,
                              12,13,14, 15, 0, 1,  2, 3, 4,  5, 6, 7 };
        Image im = load_ppm("models/p6_max15.ppm", /* srgb = */ false);
        bool ok = im.valid() && im.width == 4 && im.height == 2 &&
                  im.pixels.size() == 8;
        if (!ok) {
            std::cout << "  [FAIL] P6 p6_max15.ppm 尺寸不对\n";
            ++g_fail;
        } else {
            double worst = 0.0;
            for (int i = 0; i < 8; ++i) {
                const Color& c = im.pixels[std::size_t(i)];
                const double got[3] = { c.x(), c.y(), c.z() };
                for (int k = 0; k < 3; ++k) {
                    worst = std::fmax(worst, std::fabs(got[k] - want[i * 3 + k] / 15.0));
                }
            }
            std::cout << "        最大误差 = " << worst << "（应为 s/15）\n";
            check_bool("P6 maxval=15：24 个样本全部 == s/15", worst < 1e-12);
        }
    }

    // maxval = 65535 -> 2 字节/样本，大端
    //   数据（4x2）：
    //     (65535,    0,  256) (   1,  257,    2) ( 512,    3,  513) (   4, 1024,    5)
    //     (65535,  255,    1) (  16,   17,65535) (  32,   33,   34) (  64,  128,  256)
    //   ★ 256 (0x01 0x00) 和 1 (0x00 0x01) 是故意放的：
    //     端序读反的话它们会互换，这条断言立刻红。
    {
        const int want[24] = { 65535,   0, 256,     1, 257,   2,   512,   3, 513,     4, 1024,   5,
                               65535, 255,   1,    16,  17, 65535,     32,  33,  34,    64,  128, 256 };
        Image im = load_ppm("models/p6_max65535.ppm", /* srgb = */ false);
        bool ok = im.valid() && im.width == 4 && im.height == 2 &&
                  im.pixels.size() == 8;
        if (!ok) {
            std::cout << "  [FAIL] P7 p6_max65535.ppm 尺寸不对\n";
            ++g_fail;
        } else {
            double worst = 0.0;
            for (int i = 0; i < 8; ++i) {
                const Color& c = im.pixels[std::size_t(i)];
                const double got[3] = { c.x(), c.y(), c.z() };
                for (int k = 0; k < 3; ++k) {
                    worst = std::fmax(worst, std::fabs(got[k] - want[i * 3 + k] / 65535.0));
                }
            }
            std::cout << "        最大误差 = " << worst << "（应为 s/65535）\n";
            check_bool("P7 maxval=65535：大端 2 字节全部正确", worst < 1e-9);
        }
    }

    // ========================================================
    // 组 P8~P9：出错路径
    // ========================================================
    std::cout << "\n--- 组 P8~P9：出错路径 ---\n";

    Image trunc = load_ppm("models/p6_truncated.ppm", true);
    if (!p6.valid()) {
        // P6 还读不进来的时候，截断文件"当然"也被拒 —— 那条断言会假绿。
        std::cout << "  [FAIL] P8 无法判定 —— P6 还没实现，截断文件被拒不能算「检测到截断」\n";
        ++g_fail;
    } else {
        check_bool("P8 截断的 P6 文件 → valid() == false（不崩）", !trunc.valid());
    }

    Image bad = load_ppm("CMakeLists.txt", true);
    check_bool("P9 不是 PPM 的文件 → valid() == false（不崩）", !bad.valid());

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) std::cout << "ALL PASSED\n";
    return g_fail == 0 ? 0 : 1;
}
