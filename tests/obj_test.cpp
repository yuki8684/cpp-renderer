// ============================================================
// OBJ 读取单元测试（Day 20）—— 【第一天由学员自己写】
//
// 怎么用这个文件：
//   1. 下面「组 A 的第 1 条」已经写好了，是【示范】。看懂它在干什么。
//   2. 其余每一处 // TODO(x) 都是你的活。照着示范的样子往上加。
//   3. 每填完一组就跑一次 .\tools\test.ps1
//      —— 末尾的「共 N 项测试」会随你填的条数增加。
//      如果它一直停在 1，说明你只是写了注释，没写代码（Day 20 已经踩过这个坑）。
//
// 数据结构回顾（include/obj_loader.h）：
//   struct ObjData {
//       std::vector<Point3> positions;    // 所有 v
//       std::vector<int>    tri_indices;  // 每 3 个 = 一个三角形（0-based）
//   };
//   inline ObjData load_obj(const std::string& path);
//
// ============================================================

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "obj_loader.h"
#include "quad.h"
#include "lambertian.h"
#include "hittable_list.h"

static int g_pass = 0;
static int g_fail = 0;

// ---------- 两个比较函数（脚手架，抄的，不需要改）----------

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

// 整数比较。分开写是为了失败时能打印出【具体数字】，
// 而不是只有 true/false —— 排查时数字比布尔值有用得多。
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

// 位置比较（三个分量逐个比，容差 1e-9）
static void check_point(const char* name, const Point3& got, const Point3& want) {
    double dx = got.x() - want.x();
    double dy = got.y() - want.y();
    double dz = got.z() - want.z();
    bool ok = (dx > -1e-9 && dx < 1e-9) &&
              (dy > -1e-9 && dy < 1e-9) &&
              (dz > -1e-9 && dz < 1e-9);
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

// 退化三角形计数：面积为零的三角形在渲染里啥也看不到，
// 但会白白占一个图元位置，还会让包围盒退化。
static int count_degenerate(const ObjData& d) {
    int n = 0;
    for (std::size_t i = 0; i + 2 < d.tri_indices.size(); i += 3) {
        const Point3& a = d.positions[std::size_t(d.tri_indices[i + 0])];
        const Point3& b = d.positions[std::size_t(d.tri_indices[i + 1])];
        const Point3& c = d.positions[std::size_t(d.tri_indices[i + 2])];
        Vec3 e1 = b - a, e2 = c - a;
        double nn = dot(cross(e1, e2), cross(e1, e2));
        if (nn <= 0.0) ++n;
    }
    return n;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "\n===== OBJ 读取单元测试 =====\n";

    // ========================================================
    // 组 A：cube.obj —— 顶点数 / 索引数
    //
    // 为什么测这个：数量错了说明"读漏了行"或"三角化次数不对"，
    // 是最便宜的体检。画面完全看不出来少一个三角形。
    //
    // cube.obj 的内容：8 个 v，6 个 f，每个 f 是四边形（4 个索引）
    //   → 每个四边形切成 2 个三角形 → 6 × 2 = 12 个三角形
    //   → 每个三角形 3 个索引 → 12 × 3 = 36
    // ========================================================
    std::cout << "\n--- 组 A: cube 数量 ---\n";
    {
        ObjData d = load_obj("models/cube.obj");

        // ★ 示范：这就是一条完整的测试
        check_int("cube 顶点数 = 8", (long long)d.positions.size(), 8);

        // TODO(A1)：cube 的 tri_indices 应该是 36 个。仿照上面那条写。
        check_int("cube 索引数 = 36", (long long)d.tri_indices.size(), 36);

        // TODO(A2)：三角化后应该是 12 个三角形。
        //   提示：三角形个数 = tri_indices.size() / 3
        check_int("cube 三角形数 = 12", (long long)d.tri_indices.size() / 3, 12);
    }

    // ========================================================
    // 组 B：cube.obj —— 具体坐标
    //
    // 为什么测这个：数量对不代表读对了。如果 F 函数把 x 和 y
    // 搞反了，顶点数依然是 8 —— 但坐标全错。
    // 期望值来自 models/cube.obj 的前 1 行和最后 1 行 v（自己去看文件确认）。
    // ========================================================
    std::cout << "\n--- 组 B: cube 坐标 ---\n";
    {
        ObjData d = load_obj("models/cube.obj");

        // TODO(B1)：cube 的【第 1 个】顶点应该是 (-1, -1, -1)
        //   用 check_point("cube 顶点 0", d.positions[0], Point3(-1,-1,-1));
        check_point("cube 顶点 0", d.positions[0], Point3(-1, -1, -1));

        // TODO(B2)：cube 的【第 8 个】顶点（下标 7）应该是 (-1, 1, 1)
        //   注意：下标是 7，不是 8。想想为什么。
        check_point("cube 顶点 7", d.positions[7], Point3(-1, 1, 1));
    }

    // ========================================================
    // 组 C：icosahedron.obj 和 torus.obj —— 数量
    // ========================================================
    std::cout << "\n--- 组 C: 另两个模型的数量 ---\n";
    {
        // icosahedron：12 个 v，20 个 f，每个 f 已经是三角形（3 个索引）
        //   → 三角化后还是 20 个三角形 → 60 个索引
        ObjData ico = load_obj("models/icosahedron.obj");
        // TODO(C1)：ico 顶点数 = 12
        // TODO(C2)：ico 索引数 = 60
        check_int("ico 顶点数 = 12", (long long)ico.positions.size(), 12);
        check_int("ico 索引数 = 60", (long long)ico.tri_indices.size(), 60);

        // torus：1152 个 v，2304 个 f，每个 f 已是三角形
        ObjData tor = load_obj("models/torus.obj");
        // TODO(C3)：torus 顶点数 = 1152
        // TODO(C4)：torus 索引数 = 6912
        check_int("torus 顶点数 = 1152", (long long)tor.positions.size(), 1152);
        check_int("torus 索引数 = 6912", (long long)tor.tri_indices.size(), 6912);
    }

    // ========================================================
    // 组 D：★ 最重要的两条 —— 不变量
    //
    // 这两条不是"数数"，而是"检查有没有越界/退化"。
    // 它们是唯一能抓出下面两类错误的测试：
    //   ① 忘了把 OBJ 的 1-based 索引减 1  → 索引会跑到 8（越界）
    //   ② 模型生成器写错（如 $R/$r 撞名）→ 出现面积为 0 的三角形
    // 组 A/B/C 全都发现不了这两类。
    // ========================================================
    std::cout << "\n--- 组 D: 不变量 ---\n";
    {
        ObjData d = load_obj("models/torus.obj");

        // TODO(D1)：检查 torus 的【所有】索引都在 [0, positions.size()) 内。
        //   写法：
        //       bool ok = true;
        //       for (int v : d.tri_indices) {
        //           if (v < 0 || v >= (int)d.positions.size()) { ok = false; }
        //       }
        //       check_bool("torus 索引全部合法", ok, true);
        //   你可以直接照抄上面这段，但要能说出"为什么这个循环能抓到忘了减 1 的 bug"
        bool ok = true;
        for(int v : d.tri_indices) {
            if(v < 0 || v >= (int)d.positions.size()) { ok = false; }
        }
        check_bool("torus 索引全部合法", ok, true);

        // TODO(D2)：退化三角形数必须是 0。
        //   提示：check_int("torus 无退化三角形", (long long)count_degenerate(d), 0);
        check_int("orus 无退化三角形", (long long)count_degenerate(d), 0);
    }     

    // ========================================================
    // 组 E：异常路径
    //
    // 为什么测这个：读文件是最容易崩的地方。一个不存在的路径
    // 不应该让程序段错误退出，而应该安静地返回空数据。
    // 面试官很爱问"你的加载器遇到坏文件怎么办"。
    // ========================================================
    std::cout << "\n--- 组 E: 文件不存在 ---\n";
    {
        ObjData d = load_obj("models/这个文件不存在.obj");

        // TODO(E1)：顶点数必须是 0（而不是崩溃）
        // TODO(E2)：索引数必须是 0
        check_int("顶点数 0", (long long)d.positions.size(), 0);
        check_int("索引数 0", (long long)d.tri_indices.size(), 0);
    }

    // ---------- 汇总 ----------
    std::cout << "\n===== 共 " << (g_pass + g_fail) << " 项测试，"
              << g_pass << " 通过，" << g_fail << " 失败 =====\n";
    if (g_fail == 0 && g_pass > 1) {
        std::cout << "ALL PASSED\n";
    } else if (g_pass <= 1) {
        std::cout << ">>> 你还没开始填，或者只填了注释没写代码。\n";
    } else {
        std::cout << ">>> 有失败项，看上面的「得到 / 期望」找原因。\n";
    }
    return g_fail == 0 ? 0 : 1;
}
