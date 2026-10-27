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
};

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

            std::string tok;
            while (ss >> tok) {
                std::istringstream ts(tok);
                std::string first;
                std::getline(ts, first, '/');       // 只取 '/' 之前那一段

                // TODO(T2-1)：把 first 转成 int，再 -1 变成 0-based，push 进 face
                //   字符串转 int 用 std::stoi(first)
                //   ⚠️ 先判断 first 非空，某些文件会写 f 1//0 这种
                if(!first.empty()) {
                    std::stoi(first);
                    face.push_back(std::stoi(first) - 1);
                }

            }

            // TODO(T2-2)：扇形三角化，把每三个索引追加到 data.tri_indices
            //   如果 face.size() < 3 就跳过（不是面）
            if(face.size() < 3) {
               continue;
            }

            for(std::size_t k = 1; k  + 1 < face.size(); k++) {
               data.tri_indices.push_back(face[0]);
               data.tri_indices.push_back(face[k]);
               data.tri_indices.push_back(face[k + 1]);
            }

        }
        // vn / vt / o / g / s / mtllib / usemtl 今天一律跳过
    }

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
                             std::shared_ptr<Material> mat) {
    HittableList world;

    ObjData d = load_obj(path);
    if (d.tri_indices.size() < 3) return world;

    for (std::size_t i = 0; i + 2 < d.tri_indices.size(); i += 3) {
        Point3 a = center + scale * d.positions[std::size_t(d.tri_indices[i + 0])];
        Point3 b = center + scale * d.positions[std::size_t(d.tri_indices[i + 1])];
        Point3 c = center + scale * d.positions[std::size_t(d.tri_indices[i + 2])];

        // 退化三角形（面积为零）直接跳过 —— Quad 的退化保护也能兜，
        // 但提前跳过可以少建一个图元
        Vec3 e1 = b - a, e2 = c - a;
        Vec3 n  = cross(e1, e2);
        double nn = dot(n, n);
        if (nn <= 0.0 || nn < 1e-12 * dot(e1, e1) * dot(e2, e2)) continue;

        world.add(std::make_shared<Triangle>(a, b, c, mat));
    }

    return world;
}
