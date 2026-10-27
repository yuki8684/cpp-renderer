// ============================================================
// tools/stream_demo.cpp  (Day 20)
//
// 目的：把「用流读文本」这件事拆成 4 个最小零件，一个一个看。
//       这里的数据跟 OBJ 完全无关（假数据），你只需要看懂"手法"。
//
// 编译运行：
//   g++ -std=c++17 -O2 -o build\stream_demo.exe tools\stream_demo.cpp
//   .\build\stream_demo.exe
// ============================================================

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>   // std::setprecision，只是为了让输出好看

#ifdef _WIN32
#include <windows.h>   // SetConsoleOutputCP
#endif

// ------------------------------------------------------------
// 零件 1：istringstream + >>   —— 把一个字符串当"输入流"来读
// ------------------------------------------------------------
static void part1_extract() {
    std::cout << "========== 零件 1: istringstream + >> ==========\n";

    std::string text = "apple 3 4.5";

    // 把 text 包成一个流。之后对 ss 做 >>，就相当于"从这段文字里往外取东西"
    std::istringstream ss(text);

    std::string name;
    int    count = 0;
    double price = 0.0;

    // >> 会自动跳过空白（空格/制表符/换行），并按目标变量的类型转换
    ss >> name >> count >> price;

    std::cout << "  原文: \"" << text << "\"\n";
    std::cout << "  name  = " << name  << '\n';
    std::cout << "  count = " << count << '\n';
    std::cout << "  price = " << price << '\n';

    // --------------------------------------------------------
    // 关键点：>> 的返回值是流本身，而流"读失败/读完"时会被当成 false
    // 所以 while (ss >> 变量) 就是「一直读，读到没东西为止」
    // --------------------------------------------------------
    std::istringstream ss2("1 2 3 4 5");
    std::string tok;
    std::cout << "  while (ss2 >> tok) 得到: ";
    while (ss2 >> tok) {
        std::cout << '[' << tok << "] ";
    }
    std::cout << "\n\n";
}

// ------------------------------------------------------------
// 零件 2：getline(流, 字符串, 分隔符)  —— 按自定义分隔符切一段出来
// ------------------------------------------------------------
static void part2_split() {
    std::cout << "========== 零件 2: getline(ts, s, '/') ==========\n";

    // 四种真实存在的写法
    std::vector<std::string> samples = {
        "1",
        "1/5",
        "1//7",
        "1/5/7"
    };

    for (const std::string& tok : samples) {
        std::istringstream ts(tok);
        std::string first;
        std::getline(ts, first, '/');       // 读到 '/' 就停（'/' 本身被吃掉，不放进 first）

        // 再看第二段，用来说明"后面还有东西"
        std::string second;
        std::getline(ts, second, '/');      // 流已经到末尾 → second 保持空串

        std::cout << "  token \"" << std::setw(6) << std::left << tok
                  << "\"  ->  第1段 = \"" << first
                  << "\"   第2段 = \"" << second << "\"\n";
    }

    std::cout << "  ★ 不管哪种写法，第 1 段永远是顶点索引\n\n";
}

// ------------------------------------------------------------
// 零件 3：std::stoi —— 字符串转 int
// ------------------------------------------------------------
static void part3_stoi() {
    std::cout << "========== 零件 3: std::stoi ==========\n";

    std::string s = "42";
    int n = std::stoi(s);           // "42" -> 42
    std::cout << "  stoi(\"" << s << "\") = " << n << '\n';

    // 注意：stoi 不做任何边界/合法性检查，它只是"尽力解析开头能解析的部分"
    std::cout << "  stoi(\"7abc\") = " << std::stoi("7abc") << "   <- 后面的 abc 被无视\n";
    std::cout << "  stoi(\" 12 \") = " << std::stoi(" 12 ") << "   <- 前面空白被跳过\n";

    // 但空串会【抛异常】（不是返回 0！），所以必须先判空
    try {
        std::stoi("");
    } catch (const std::exception& e) {
        std::cout << "  stoi(\"\") 抛异常 : " << e.what() << '\n';
    }

    std::cout << "  ★ 结论：调用 stoi 前一定要 if (!s.empty())\n\n";
}

// ------------------------------------------------------------
// 零件 4：把上面三个拼起来 —— 从一行假数据里取出"索引"
// ------------------------------------------------------------
static void part4_combine() {
    std::cout << "========== 零件 4: 拼起来 ==========\n";

    std::string line = "f 3/1 1/6 2/5";     // 一条假的面记录

    std::istringstream ss(line);
    std::string tag;
    ss >> tag;                               // 先吃掉第一个词（"f"）
    std::cout << "  tag = \"" << tag << "\"   (流现在停在它后面)\n";

    std::vector<int> idx;
    std::string tok;
    while (ss >> tok) {                      // 剩下的词一个个取
        std::istringstream ts(tok);
        std::string first;
        std::getline(ts, first, '/');
        if (!first.empty()) {
            idx.push_back(std::stoi(first)); // ← 这里【没有】减 1，故意留着
        }
    }

    std::cout << "  取到的原始编号: ";
    for (int v : idx) std::cout << v << ' ';
    std::cout << "\n";
    std::cout << "  ⚠️ OBJ 文件是 1-based，所以真正该存进数组的是 v - 1\n";
    std::cout << "  减 1 之后: ";
    for (int v : idx) std::cout << (v - 1) << ' ';
    std::cout << "\n\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);   // 让 PowerShell 控制台按 UTF-8 显示中文
    part1_extract();
    part2_split();
    part3_stoi();
    part4_combine();
    std::cout << "全部零件跑完。现在回到 include/obj_loader.h 去填 T1 / T2。\n";
    return 0;
}
