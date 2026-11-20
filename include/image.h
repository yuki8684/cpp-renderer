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

    // ============================================================
    // TODO(你填 D5)：二进制格式（P6）
    //
    //   P3 和 P6 的【头部完全一样】，只差像素数据怎么存：
    //       P3: "205 205 200 75 80 90 ..."        十进制文本
    //       P6: 0xCD 0xCD 0xC8 0x4B 0x50 0x5A ...  原始字节
    //   同一张 256×256 图：P3 = 686,802 B，P6 = 196,623 B（小 3.5 倍）。
    //
    //   ① 吃掉 maxval 之后的【恰好一个】空白字符
    //
    //        in.get();
    //
    //      ★ 为什么"恰好一个"：
    //        一个都不吃 → 那个空白字节变成第一个 R 值 → 整张图右移一位
    //        吃两个      → 第一个真实字节被吃掉 → 整张图左移一位
    //        两种都是"整体错位"，症状是画面出现斜向颜色污迹，极难反查。
    //
    //   ② 每个样本占几个字节
    //
    //        const int bytes_per_sample = (maxval > 255) ? 2 : 1;
    //
    //   ③ 一次性读进来
    //
    //        std::vector<unsigned char> buf(w * h * 3 * bytes_per_sample);
    //        in.read(reinterpret_cast<char*>(buf.data()),
    //                static_cast<std::streamsize>(buf.size()));
    //        if (in.gcount() != static_cast<std::streamsize>(buf.size())) {
    //            // 文件被截断了 → 报错并返回 Image{}
    //        }
    //
    //      ★ gcount() = 【实际读到的字节数】。read() 本身不抛异常，
    //        不检查的话剩下的像素会静默留成黑色。
    //
    //   ④ 把字节拼回原始整数（★ 2 字节时是【大端】：高字节在前）
    //
    //        1 字节：  s = buf[p];
    //        2 字节：  s = (buf[p] << 8) | buf[p + 1];
    //
    //      ⚠️ 不能 memcpy 到 uint16_t —— x86 是小端，会读反。
    //         256 (0x01 0x00) 会被读成 1，1 (0x00 0x01) 会被读成 256。
    //         测试文件 models/p6_max65535.ppm 里专门放了这两个值。
    //
    //   ⑤ 换算和 P3 分支【完全一样】
    //
    //        encoded = double(s) / double(maxval)
    //        c = srgb ? srgb_to_linear(encoded) : encoded
    //
    //      既然一样，把这段抽成 lambda 最省事（见 P3 分支里那几行）。
    // ============================================================
    in.get();   // 吃掉 maxval 后的【恰好一个】空白字符
    const int bytes_per_sample = (maxval > 255) ? 2 : 1;
    std::vector<unsigned char> buf(std::size_t(w) * std::size_t(h) * 3u * std::size_t(bytes_per_sample));
    in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    if (in.gcount() != static_cast<std::streamsize>(buf.size())) {
        std::cerr << "[PPM] 数据不足（文件被截断）: " << path << '\n';
        return Image{};
    }
    for (long long i = 0; i < w * h; ++i) {
        double c[3] = {0.0, 0.0, 0.0};
        for (int k = 0; k < 3; ++k) {
            // ★ 这两个下标就是全部：
            //     样本号 = 像素号*3 + 通道号
            //     字节下标 p = 样本号 * bytes_per_sample
            const std::size_t sample_index = std::size_t(i) * 3u + std::size_t(k);
            const std::size_t p = sample_index * std::size_t(bytes_per_sample);

            long long s = 0;
            if (bytes_per_sample == 1) {
                s = static_cast<long long>(buf[p]);
            } else {
                // 大端：高字节在前。不能 memcpy 到 uint16_t（x86 是小端）
                s = (static_cast<long long>(buf[p]) << 8) |
                     static_cast<long long>(buf[p + 1]);
            }
            if (s < 0) { s = 0; }            // 夹边界：坏文件别把 decode 送到域外
            if (s > maxval) { s = maxval; }
            const double encoded = double(s) / double(maxval);   // → 0~1
            c[k] = srgb ? srgb_to_linear(encoded) : encoded;
        }
        img.pixels[std::size_t(i)] = Color(c[0], c[1], c[2]);
    }
    return img;
}
