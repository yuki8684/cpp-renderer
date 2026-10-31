#pragma once

#include "hittable_list.h"
#include "triangle.h"
#include "vec3.h"

#include <fstream>    // std::ifstream —— 读文件
#include <sstream>    // std::istringstream —— 把一行拆成一个个词
#include <string>
#include <vector>

// ============================================================
// OBJ 文件读取（Day 20）
//
// OBJ 是纯文本格式，每行一条指令：
//
//     v  x y z       顶点位置
//     vn x y z       顶点法线（今天先不管）
//     vt u v         纹理坐标（今天先不管）
//     f  i j k ...   面（顶点索引）
//     #              注释
//     o / g / s / mtllib / usemtl   分组/材质，今天都跳过
//
// ★ 三个大坑：
//   ① 索引是【1-based】—— 文件里的第 1 个 v 用索引 1 表示
//   ② 索引是【全局累积】的，不是每行重新计数
//   ③ f 行可能是多边形（四边形、五边形），必须三角化
// ============================================================

struct ObjData {
    std::vector<Point3> positions;     // 所有 v
    std::vector<int>    tri_indices;   // 每 3 个一组，指向 positions（0-based）
    std::vector<Vec3>   normals;       // Day 21：每个顶点的法线（由面法线加权平均得到）

    // Day 22：UV
    std::vector<UV>     uvs;           // 所有 vt
    std::vector<UV>     tri_uvs;       // 每 3 个一组，对应 tri_indices（无 vt 时为空）
};

// ============================================================
// Day 21 · 用面法线算出顶点法线（面积加权平均）
//
// 为什么需要它：
//   OBJ 里可以写 `vn`（顶点法线），但大量文件根本没写。
//   没有顶点法线就只能平面着色 —— 二十面体永远是"20 个平面"。
//
// 三个步骤：
//   ① 每个三角形算一个面法线（未归一化的叉积）
//   ② 把面法线累加到它的三个顶点上
//   ③ 每个顶点归一化
//
// ★ 关键：步骤 ① 用的是【未归一化】的叉积。
//   因为 |cross(b-a, c-a)| = 2 × 三角形面积 —— 长度自带面积权重。
//   所以"累加"这一步本身就是面积加权平均，不需要再乘面积、也不需要除以面积。
// ============================================================
inline void compute_vertex_normals(ObjData& d) {
    // 每个顶点一个累加器，初值全零
    //   assign(n, v) = 把容器改成"n 个 v"（比 clear + 循环 push 简洁）
    d.normals.assign(d.positions.size(), Vec3(0.0, 0.0, 0.0));

    if (d.positions.empty()) return;

    for (std::size_t i = 0; i + 2 < d.tri_indices.size(); i += 3) {
        std::size_t ia = std::size_t(d.tri_indices[i + 0]);
        std::size_t ib = std::size_t(d.tri_indices[i + 1]);
        std::size_t ic = std::size_t(d.tri_indices[i + 2]);

        // ------------------------------------------------------
        // TODO(T3-1)：算这个三角形的【未归一化】面法线
        //
        //   三个顶点：
        //       const Point3& a = d.positions[ia];
        //       const Point3& b = d.positions[ib];
        //       const Point3& c = d.positions[ic];
        //
        //   面法线 = cross(b - a, c - a)
        //
        //   ⚠️ 不要 unit_vector！长度就是权重，归一化会把权重扔掉
        const Point3& a = d.positions[ia];
        const Point3& b = d.positions[ib];
        const Point3& c = d.positions[ic];
        Vec3 n = cross(b - a, c - a);

        // ------------------------------------------------------
        // TODO(T3-2)：把 n 累加到三个顶点上
        //
        //   ⚠️ 三个都要加，包括等于别人的情况（共享顶点会重复累加，这是对的）
        //   写法：d.normals[ia] = d.normals[ia] + n;（三行）
        // ------------------------------------------------------
        d.normals[ia] = d.normals[ia] + n;
        d.normals[ib] = d.normals[ib] + n;
        d.normals[ic] = d.normals[ic] + n;

    }

    // --------------------------------------------------------
    // TODO(T3-3)：归一化，并防除零
    //
    //   for (std::size_t i = 0; i < d.normals.size(); ++i) {
    //       double L2 = dot(d.normals[i], d.normals[i]);
    //       if (L2 > 0.0) { ______ }             // 除以长度
    //       else          { d.normals[i] = Vec3(0.0, 1.0, 0.0); }   // 兜底
    //   }
    //
    //   提示：长度 = std::sqrt(L2)；Vec3 支持 v / double
    //   为什么要兜底：如果一个顶点不属于任何有效三角形（全是退化面），
    //   累加结果是零向量，归一化会除零 → NaN → 整个画面变黑/变彩条
    // --------------------------------------------------------
    for (std::size_t i = 0; i < d.normals.size(); ++i) {
        double L2 = dot(d.normals[i], d.normals[i]);
        if(L2 > 0.0) {d.normals[i] = d.normals[i] / std::sqrt(L2);}
        else         {(d.normals[i] = Vec3(0.0, 1.0, 0.0));}

    }
}

// ============================================================
// 读文件
//
// ⚠️ 下面这几个是今天的新语法，我写好了，你看懂就行：
//
//   std::ifstream in(path);        以只读方式打开文件
//   if (!in)                       打开失败（路径错 / 文件不存在）
//   std::getline(in, line)         每次读一整行，读完返回 false
//   std::istringstream ss(line);   把 line 包成一个"可以逐词读"的流
//   ss >> tag;                     读出一个用空格分隔的词
//   std::istringstream ts(tok);
//   std::getline(ts, first, '/');  以 '/' 为分隔，读出一段
// ============================================================
inline ObjData load_obj(const std::string& path) {
    ObjData data;

    std::ifstream in(path);
    if (!in) {
        std::cerr << "[OBJ] 打不开文件: " << path << '\n';
        return data;
    }

    std::string line;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;               // 注释

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag.empty()) continue;

        if (tag == "v") {
            // ------------------------------------------------------
            // TODO(T1)：读三个 double，存进 data.positions
            //
            //   这一行长这样：  v -0.52573 0.85065 0
            //   ss 已经在 "v" 之后了，接着 >> 就能依次读到三个数。
            //
            //   ⚠️ 用 Point3(x, y, z) 存（位置，不是方向）
            //
            double x, y, z;
            ss >> x >> y >> z;
            data.positions.push_back(Point3(x, y, z));
            // ------------------------------------------------------

        } else if (tag == "vt") {
            // ------------------------------------------------------
            // TODO(T5)：读两个 double，存进 data.uvs
            //
            //   这一行长这样：   vt 0.5 0.25     ← u v
            //   ⚠️ 和 v 一样：ss 已经在 "vt" 之后了，接着 >> 就能读到两个数
            //   ⚠️ 存成 UV{u, v}（在 triangle.h 里定义）
            //
            //   ★ 注意：OBJ 的 v 轴通常是【向上】的，而纹理图的行是自上而下。
            //     有些加载器会写 1 - v 翻转，我们的棋盘格是对称的，暂不处理。
            double tu, tv;
            ss >> tu >> tv;
            data.uvs.push_back(UV{tu, tv});
            // ------------------------------------------------------

        } else if (tag == "f") {
            // ------------------------------------------------------
            // TODO(T2)：解析一个面
            //
            //   面这一行有四种写法（后面三种今天先当第一种处理）：
            //       f 1 2 3
            //       f 1/1 2/2 3/3          顶点/纹理
            //       f 1//1 2//2 3//3       顶点//法线
            //       f 1/1/1 2/2/2 3/3/3    顶点/纹理/法线
            //
            //   做法：把每个 token 按 '/' 切开，只取【第 1 段】= 顶点索引
            //
            //   T2-1：收集这个面的所有顶点索引（已转成 0-based）
            //         → 存进局部变量 std::vector<int> face;
            //
            //   T2-2：把 face 三角化，结果追加到 data.tri_indices
            //         多边形用【扇形切分】：(0,1,2) (0,2,3) (0,3,4) ...

            // ------------------------------------------------------
            std::vector<int> face;
            std::vector<int> face_uv;   // Day 22：每个角的第 2 段（UV 索引）

            std::string tok;
            while (ss >> tok) {
                std::istringstream ts(tok);
                std::string first;
                std::getline(ts, first, '/');       // 只取 '/' 之前那一段

                // TODO(T2-1)：把 first 转成 int，再 -1 变成 0-based，push 进 face
                //   字符串转 int 用 std::stoi(first)
                //   ⚠️ 先判断 first 非空，某些文件会写 f 1//0 这种
                if(!first.empty()) {

                    face.push_back(std::stoi(first) - 1);
                }

                // Day 22：继续读第 2 段（UV 索引）
                //   流 ts 现在停在第一个 '/' 的后面：
                //     "1/5/7"  → 第 2 段是 "5"
                //     "1//7"   → 第 2 段是 ""（空）
                //     "1"      → 流已结束，第 2 段也是 ""
                std::string second;
                std::getline(ts, second, '/');
                if (!second.empty()) {
                    face_uv.push_back(std::stoi(second) - 1);
                }
            }

            // TODO(T2-2)：扇形三角化，把每三个索引追加到 data.tri_indices
            //   如果 face.size() < 3 就跳过（不是面）
            if(face.size() < 3) {
               continue;
            }

            // Day 22：只有 UV 个数和顶点个数对得上，才认为这个面带 UV
            bool face_has_uv = (face_uv.size() == face.size()) && !data.uvs.empty();

            for(std::size_t k = 1; k  + 1 < face.size(); k++) {
               data.tri_indices.push_back(face[0]);
               data.tri_indices.push_back(face[k]);
               data.tri_indices.push_back(face[k + 1]);

               // 三角形第 i 个角 ⇔ 多边形顶点 face[i] ⇔ 多边形 UV 索引 face_uv[i]
               if (face_has_uv) {
                   data.tri_uvs.push_back(data.uvs[std::size_t(face_uv[0])]);
                   data.tri_uvs.push_back(data.uvs[std::size_t(face_uv[k])]);
                   data.tri_uvs.push_back(data.uvs[std::size_t(face_uv[k + 1])]);
               }
            }

        }
        // vn / vt / o / g / s / mtllib / usemtl 今天一律跳过
    }

    // Day 21：解析完几何之后，用面法线自己算出顶点法线
    compute_vertex_normals(data);

    return data;
}

// ============================================================
// 把一个 OBJ 变成一堆三角形，加进世界
//
// center / scale 用来把模型摆到场景里、并缩放它。
// （OBJ 文件的坐标范围可能很大也可能很小，必须能调）
// ============================================================
inline HittableList obj_mesh(const std::string& path,
                             const Point3& center, double scale,
                             std::shared_ptr<Material> mat,
                             bool smooth = true) {
    HittableList world;

    ObjData d = load_obj(path);
    if (d.tri_indices.size() < 3) return world;

    for (std::size_t i = 0; i + 2 < d.tri_indices.size(); i += 3) {
        std::size_t ia = std::size_t(d.tri_indices[i + 0]);
        std::size_t ib = std::size_t(d.tri_indices[i + 1]);
        std::size_t ic = std::size_t(d.tri_indices[i + 2]);

        // 位置要跟着 center / scale 变换
        Point3 a = center + scale * d.positions[ia];
        Point3 b = center + scale * d.positions[ib];
        Point3 c = center + scale * d.positions[ic];

        // 退化三角形（面积为零）直接跳过 —— Quad 的退化保护也能兜，
        // 但提前跳过可以少建一个图元
        Vec3 e1 = b - a, e2 = c - a;
        Vec3 n  = cross(e1, e2);
        double nn = dot(n, n);
        if (nn <= 0.0 || nn < 1e-12 * dot(e1, e1) * dot(e2, e2)) continue;

        if (smooth) {
            // --------------------------------------------------
            // TODO(T4)：用【顶点法线】构造三角形（Day 15 写的 6 参数版本）
            //
            //   顶点法线存在 d.normals[ia] / d.normals[ib] / d.normals[ic]
            //
            //   ⚠️ 它们【不需要】跟着 center / scale 变换：
            //      法线是"方向"，不是"位置"。
            //      平移不影响方向；等比缩放也不影响方向。
            //      （只有【非等比】缩放才会改变方向，那时需要乘逆转置矩阵）
            //
            //   Triangle 的两个构造函数：
            //       Triangle(a, b, c, mat)                    平面着色
            //       Triangle(a, b, c, n0, n1, n2, mat)        顶点法线插值
            auto tri = std::make_shared<Triangle>(a, b, c,
                                                  d.normals[ia], d.normals[ib], d.normals[ic], mat);
            // --------------------------------------------------
            // Day 22：有 UV 就挂上去（三角化后 tri_indices 和 tri_uvs 严格一一对应）
            if (!d.tri_uvs.empty()) {
                tri->set_uv(d.tri_uvs[i + 0], d.tri_uvs[i + 1], d.tri_uvs[i + 2]);
            }
            world.add(tri);
        } else {
            auto tri = std::make_shared<Triangle>(a, b, c, mat);
            if (!d.tri_uvs.empty()) {
                tri->set_uv(d.tri_uvs[i + 0], d.tri_uvs[i + 1], d.tri_uvs[i + 2]);
            }
            world.add(tri);
        }
    }

    return world;
}
