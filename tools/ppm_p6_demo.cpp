// ============================================================
// ppm_p6_demo.cpp -- P6 的「样本下标 <-> 字节下标」演示
//
//   g++ -std=c++17 -O2 -I include -o build/ppm_p6_demo.exe tools/ppm_p6_demo.cpp
//   .\build\ppm_p6_demo.exe
//
// 这个程序【不读文件】，也不碰 Image。
// 它只做一件事：把一堆字节按 P6 的规则拼回整数，并把每一步的下标打印出来。
//
// 假设一张 2 x 1 的 P6 图片，maxval = 1000（> 255，所以 2 字节/样本）：
//
//   像素 0 = (1000,    0, 256)
//   像素 1 = (   1,  999, 512)
//
// 一共 2*1*3 = 6 个样本，每个 2 字节 -> 12 个字节。
// ============================================================

#include <cstdio>
#include <vector>

int main() {
    // --------------------------------------------------------
    // ① 把 6 个样本按【大端】写成 12 个字节
    // --------------------------------------------------------
    const long long samples[6] = { 1000, 0, 256,     1, 999, 512 };
    const int n_samples = 6;
    const int w = 2, h = 1;

    std::vector<unsigned char> buf;
    for (int s = 0; s < n_samples; ++s) {
        buf.push_back(static_cast<unsigned char>((samples[s] >> 8) & 0xFF));  // 高字节
        buf.push_back(static_cast<unsigned char>( samples[s]       & 0xFF));  // 低字节
    }

    std::printf("字节缓冲区（%d 字节）：\n  ", static_cast<int>(buf.size()));
    for (std::size_t i = 0; i < buf.size(); ++i) {
        std::printf("%02X ", buf[i]);
        if ((i + 1) % 6 == 0) std::printf("\n  ");
    }
    std::printf("\n\n");

    // --------------------------------------------------------
    // ② 反过来：从字节拼回整数
    //
    //    ★ 核心就是这一行下标：
    //          p = (像素号 * 3 + 通道号) * 每样本字节数
    // --------------------------------------------------------
    const int bytes_per_sample = 2;

    std::printf("拼回来的过程：\n");
    std::printf("  像素  通道   样本号 p=样本号*2   字节[p]  字节[p+1]   拼出来\n");
    std::printf("  ----  ----   -----  ---------   -------  ---------   ------\n");

    for (int i = 0; i < w * h; ++i) {            // 像素号
        for (int k = 0; k < 3; ++k) {            // 0=R 1=G 2=B
            const int sample_index = i * 3 + k;              // 第几个样本
            const std::size_t p = std::size_t(sample_index) * std::size_t(bytes_per_sample);

            long long s;
            if (bytes_per_sample == 1) {
                s = buf[p];
            } else {
                s = (static_cast<long long>(buf[p]) << 8) | static_cast<long long>(buf[p + 1]);
            }

            std::printf("   %d     %s     %2d      %3zu        %02X       %02X      %6lld\n",
                        i, (k == 0 ? "R" : (k == 1 ? "G" : "B")),
                        sample_index, p, buf[p], buf[p + 1], s);
        }
    }

    // --------------------------------------------------------
    // ③ 如果 maxval <= 255，每样本只占 1 字节
    //    同样的公式，只是 bytes_per_sample = 1
    // --------------------------------------------------------
    std::printf("\n对照：maxval <= 255 时每样本 1 字节\n");
    std::printf("  样本号   p=样本号*1   得到\n");
    for (int sample_index = 0; sample_index < 4; ++sample_index) {
        const std::size_t p = std::size_t(sample_index) * 1u;
        std::printf("    %2d        %2zu        %02X\n", sample_index, p, buf[p]);
    }

    std::printf("\n一句话：p = (像素号*3 + 通道号) * bytes_per_sample\n");
    return 0;
}
