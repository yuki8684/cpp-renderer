#pragma once

#include "rtweekend.h"
#include "color.h"
#include "vec3.h"

#include <fstream>
#include <string>
#include <vector>

// ============================================================
// 图片读取（Day 24）：PPM（P3 文本 / P6 二进制）
//
// 为什么先做 PPM 而不是 PNG/JPG？
//   ① 不需要第三方库（zlib / libjpeg），一行 C++ 就能读完
//   ② 格式本身只有 4 个字段：magic / 宽 / 高 / 最大值
//   ③ P3 是纯文本，打开就能用眼睛核对解析对不对
//
// ★ 图片里存的是【编码值】（sRGB），不是线性光强。
//   所以读进来后必须逐通道解码，否则贴图会偏亮 ——
//   这就是 Day 24 的 A/B 实验：srgb = false 时画面明显发灰发亮。
// ============================================================

struct Image {
    int width  = 0;
    int height = 0;

    // 线性光强，行优先：第 0 行 = 图片【最上面】那一行
    // （和 PPM 文件顺序一致；翻转 v 的事交给 ImageTexture 做）
    std::vector<Color> pixels;

    bool valid() const {
        return width > 0 && height > 0 &&
               pixels.size() == std::size_t(width) * std::size_t(height);
    }
};

// TODO(你填 D3)：读一个"词"，同时跳过 # 开头的注释行
//
//   为什么不能直接 `in >> tok`？
//     PPM 允许在【任意位置】插注释，例如：
//         P3
//         # 这是注释
//         3 2
//     直接读会把 "#" 当成一个 token。
//
//   提示：
//     while (in >> tok) {
//         if (!tok.empty() && tok[0] == '#') {
//             std::string rest; std::getline(in, rest);  // 吃掉整行
//             continue;
//         }
//         return true;
//     }
//     return false;   // 流结束
inline bool ppm_next_token(std::istream& in, std::string& tok) {
    while (in >> tok) {
        if (!tok.empty() && tok[0] == '#') {
            // 这一 token 是注释的开头 → 吃掉整行，继续找下一个 token
            std::string rest;
            std::getline(in, rest);
            continue;
        }
        return true;
    }
    return false;   // 流结束
}

// srgb = true  → 按 sRGB 图片解码成线性光强（反照率贴图就该这样）
// srgb = false → 当成已经是线性的数据（法线图 / 粗糙度图就该这样）
inline Image load_ppm(const std::string& path, bool srgb = true) {
    Image img;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "[PPM] 打不开文件: " << path << '\n';
        return img;
    }

    std::string magic, tok;
    if (!ppm_next_token(in, magic) || (magic != "P3" && magic != "P6")) {
        std::cerr << "[PPM] 不是 P3/P6 文件: " << path << '\n';
        return img;
    }

    long long w = 0, h = 0, maxval = 0;
    if (!ppm_next_token(in, tok)) { std::cerr << "[PPM] 缺少宽度: " << path << '\n'; return img; }
    w = std::stoll(tok);
    if (!ppm_next_token(in, tok)) { std::cerr << "[PPM] 缺少高度: " << path << '\n'; return img; }
    h = std::stoll(tok);
    if (!ppm_next_token(in, tok)) { std::cerr << "[PPM] 缺少最大值: " << path << '\n'; return img; }
    maxval = std::stoll(tok);

    if (w <= 0 || h <= 0 || maxval <= 0 || maxval > 65535 ||
        w * h > 100000000LL) {
        std::cerr << "[PPM] 尺寸不合法: " << path
                  << "  (" << w << "x" << h << ", max=" << maxval << ")\n";
        return img;
    }

    img.width  = int(w);
    img.height = int(h);
    img.pixels.resize(std::size_t(w) * std::size_t(h));


    // --------------------------------------------------------
    // ★ 两种分支只差"数字从哪来"，转换公式完全一样：
    //     线性值 = decode( 原始值 / maxval )
    //   decode(encoded) = srgb ? srgb_to_linear(encoded) : encoded
    // --------------------------------------------------------

    if (magic == "P3") {
        // 文本格式：width*height 个像素，每个像素 3 个 0~maxval 的整数
        for (long long i = 0; i < w * h; ++i) {
            double c[3] = {0.0, 0.0, 0.0};
            for (int k = 0; k < 3; ++k) {
                if (!ppm_next_token(in, tok)) {
                    std::cerr << "[PPM] 数据不足（第 " << i << " 个像素）: " << path << '\n';
                    return Image{};
                }
                long long s = std::stoll(tok);
                if (s < 0) { s = 0; }            // 夹边界：坏文件别把 decode 送到域外
                if (s > maxval) { s = maxval; }
                const double encoded = double(s) / double(maxval);   // → 0~1
                c[k] = srgb ? srgb_to_linear(encoded) : encoded;
            }
            img.pixels[std::size_t(i)] = Color(c[0], c[1], c[2]);
        }
        return img;
    }

    // TODO(你填 D5)：二进制格式（P6）
    //
    //   ★ 这一步是全篇最容易错的地方：
    //     `>>` 读完 maxval 之后，流停在【最后一个数字的后面】，
    //     而二进制数据的第一个字节紧跟着【恰好一个】空白字符。
    //     所以必须显式 in.get() 吃掉它，否则整张图会错位一个字节。
    //
    //   ① in.get()
    //   ② bytes_per_sample = (maxval > 255) ? 2 : 1
    //   ③ in.read(...) 一次性读 width*height*3*bytes_per_sample 字节
    //   ④ 用 in.gcount() 检查"读够了没有"（文件可能被截断）
    //   ⑤ 每个样本换算成 0~1 再 decode
    return Image{};
}
