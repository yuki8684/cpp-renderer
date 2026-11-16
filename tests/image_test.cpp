// ============================================================
// 图片 / 纹理 单元测试（Day 24）
//
// 这是第一个针对【外部文件格式】的测试文件。
// 前面的测试都在验"算得对不对"，这里多了一类：
//   验"读进来的东西和文件里写的一致"。
//
// 分四组：
//   I1~I4  PPM 读取（格式解析）
//   I5~I7  wrap_index（负数绕回）
//   I8~I9  sRGB 编解码（往返自证）
//   I10    读取失败不崩
//   I11~I14 ★ 纹理方向（四个角异色 → 一次锁死 u/v 两个轴的方向）
// ============================================================

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "image.h"
#include "image_texture.h"
#include "texture.h"

static int g_pass = 0;
static int g_fail = 0;

// ---------- 比较函数（脚手架）----------

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

static void check_int(const char* name, long long got, long long want) {
    if (got == want) {
        ++g_pass;
        std::cout << "  [PASS] " << name << '\n';
    } else {
        ++g_fail;
        std::cout << "  [FAIL] " << name
                  << "    得到=" << got << "  期望=" << want << '\n';
    }
}

// 打印一个颜色，失败时用来定位（光看 true/false 是查不出原因的）
static void dump(const char* tag, const Color& c) {
    std::cout << "        " << tag << " = R"
              << c.x() << " G" << c.y() << " B" << c.z() << '\n';
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== 图片 / 纹理单元测试 =====\n";

    // ========================================================
    // 组 I1~I4：PPM 文本格式读取
    //
    // 测试素材 models/uvgrid.ppm 由 tools/gen_texture.ps1 生成：
    //   256x256，8x8 棋盘格
    //   图片空间（第 0 行在顶部）四个角：
    //     左上 RED  /  右上 GREEN
    //     左下 BLUE /  右下 YELLOW
    // ========================================================
    std::cout << "\n--- 组 I1~I4: PPM 读取 ---\n";
    {
        Image img = load_ppm("models/uvgrid.ppm", /* srgb = */ true);

        check_bool("I1 uvgrid.ppm 加载成功", img.valid(), true);

        check_bool("I2 尺寸 == 256 x 256",
                   img.width == 256 && img.height == 256, true);

        check_int("I3 pixels.size() == 65536",
                  (long long)img.pixels.size(), 256 * 256);

        // 像素 0 = 文件第 0 行第 0 列 = 图片【左上角】= RED(230,40,40)
        // sRGB 解码后大约是 (0.79, 0.02, 0.02)
        const Color& p0 = img.pixels[0];
        dump("pixel[0]", p0);
        check_bool("I4 左上角像素是红色（R>0.7, G<0.1, B<0.1）",
                   p0.x() > 0.7 && p0.y() < 0.1 && p0.z() < 0.1, true);
    }

    // ========================================================
    // 组 I5~I7：wrap_index（负数取模）
    //
    // C++ 的 % 对负数返回负数，所以不能直接 i % n。
    // 这三个用例覆盖：越界正数 / 越界负数 / 跨一整圈
    // ========================================================
    std::cout << "\n--- 组 I5~I7: wrap_index ---\n";
    {
        check_int("I5 wrap_index(-1, 8) == 7", wrap_index(-1, 8), 7);
        check_int("I6 wrap_index( 8, 8) == 0", wrap_index(8, 8), 0);
        check_int("I7 wrap_index(-9, 8) == 7", wrap_index(-9, 8), 7);
    }

    // ========================================================
    // 组 I8~I9：sRGB 编解码
    //
    // I9 是今天最漂亮的一条：srgb_to_linear 和 linear_to_srgb
    // 是互逆函数，所以"先编码再解码"必须回到原值。
    // 解码公式只要有一处写错（比如 2.4 和 1/2.4 写反），往返就不会闭合。
    // ========================================================
    std::cout << "\n--- 组 I8~I9: sRGB 编解码 ---\n";
    {
        check_bool("I8 srgb_to_linear 端点：0 -> 0，1 -> 1",
                   srgb_to_linear(0.0) == 0.0 && srgb_to_linear(1.0) == 1.0,
                   true);

        // 往返：decode(encode(x)) == x
        const double xs[] = {0.001, 0.5, 0.999};
        double worst = 0.0;
        for (double x : xs) {
            double back = srgb_to_linear(linear_to_srgb(x));
            double err  = std::fabs(back - x);
            if (err > worst) { worst = err; }
        }
        std::cout << "        往返最大误差 = " << worst << '\n';
        check_bool("I9 srgb_to_linear(linear_to_srgb(x)) == x（误差 < 1e-9）",
                   worst < 1e-9, true);
    }

    // ========================================================
    // 组 I10：读取失败不崩
    // ========================================================
    std::cout << "\n--- 组 I10: 读取失败 ---\n";
    {
        Image bad = load_ppm("models/这个文件不存在.ppm", true);
        check_bool("I10 文件不存在时 valid() == false（且不崩）",
                   bad.valid(), false);
    }

    // ========================================================
    // 组 I11~I14：★ 纹理方向
    //
    // 这是整个文件里最有价值的一组。
    //
    // 贴图四个角是【不同颜色】，所以一次采样就能同时验证：
    //   · u 轴方向（左/右）
    //   · v 轴方向（上/下，靠的是 1-v 翻转）
    //
    //   u=0,v=0 → 左下 → BLUE
    //   u=0,v=1 → 左上 → RED
    //   u=1,v=0 → 右下 → YELLOW
    //   u=1,v=1 → 右上 → GREEN
    //
    // 采样点用 0.0625 / 0.9 而不是 0 / 1：
    //   边界上会同时混到相邻格子，颜色不确定；
    //   取格子内部才是"纯"的角标记色。
    // ========================================================
    std::cout << "\n--- 组 I11~I14: 纹理方向（四个角）---\n";
    {
        auto tex = ImageTexture::load("models/uvgrid.ppm", /* srgb = */ true);
        const Point3 p(0, 0, 0);

        const Color cBL = tex->value(0.0625, 0.0625, p);   // 左下
        const Color cTL = tex->value(0.0625, 0.9,    p);   // 左上
        const Color cBR = tex->value(0.9,    0.0625, p);   // 右下
        const Color cTR = tex->value(0.9,    0.9,    p);   // 右上

        dump("左下(0.06,0.06)", cBL);
        dump("左上(0.06,0.90)", cTL);
        dump("右下(0.90,0.06)", cBR);
        dump("右上(0.90,0.90)", cTR);

        // 蓝：B 最强
        check_bool("I11 左下角是蓝色（1-v 与 u 都正确）",
                   cBL.z() > 0.5 && cBL.x() < 0.2 && cBL.y() < 0.2, true);

        // 红：R 最强，G/B 都很低
        check_bool("I12 左上角是红色（若看到蓝，说明 v 没翻）",
                   cTL.x() > 0.5 && cTL.y() < 0.2 && cTL.z() < 0.2, true);

        // 黄：R 和 G 都高，B 低
        check_bool("I13 右下角是黄色",
                   cBR.x() > 0.5 && cBR.y() > 0.3 && cBR.z() < 0.2, true);

        // 绿：G 最强
        check_bool("I14 右上角是绿色（若看到红，说明 u 反了）",
                   cTR.y() > 0.4 && cTR.x() < 0.2 && cTR.z() < 0.2, true);
    }

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0) {
        std::cout << "ALL PASSED\n";
    } else {
        std::cout << ">>> 有失败项，看上面的「得到 / 期望」和颜色打印找原因。\n";
    }
    return g_fail == 0 ? 0 : 1;
}
