# C++ 笔记

> 从 [devlog.md](./devlog.md) 里抽出来的 C++ 语言 / 编程知识点，按学习时的 Day 排列，方便和开发日志对照。
>
> 只收**语言与库的通用知识**（语法、语义、标准库、编译期陷阱）。
> 调试方法与工程习惯（PowerShell、对照实验、测量方法论、单元测试组织等）仍留在开发日志里。
>
> 需要在 devlog 里读完整故事时，按 Day 编号去找即可；反过来，这里的每个小节都标了对应的 Day。
>
> 维护约定：以后的 C++ 知识点直接追加到本文件对应的 Day 小节，devlog 里只留一行链接。

## 主题索引

| 主题 | 在哪几天 |
|---|---|
| 头文件：`#pragma once`、`#include` 的本质、平台头文件污染、`std::` 限定 | Day 1 / 11 / 12 |
| 声明 vs 定义、单遍编译、前向声明（循环包含） | Day 3 / 4 / 11 |
| 作用域与变量遮蔽 | Day 3 / 6 |
| 函数：重载、递归、不能嵌套、局部 `static` | Day 3 / 10 |
| 类：构造函数（默认 / 带参 / 委托）、初始化列表、初始化顺序 | Day 1 / 2 / 4 / 6 / 9 / 14 |
| `this->`、成员 vs 局部、成员默认初始化 | Day 6 / 13 |
| `const`、引用、输出参数 | Day 1 / 2 / 15 |
| `virtual` / `override` / vtable / 模板方法 / 能力查询 | Day 2 / 4 / 8 / 13 / 14 / 15 / 16 / 17 |
| 智能指针与所有权：`shared_ptr` / `make_shared` | Day 4 / 7 |
| 值语义：RVO、按值传参、`std::move` | Day 7 / 9 / 10 |
| lambda、`std::sort`、范围 for | Day 2 / 9 |
| `static` 的几种用法（局部 / 成员 / 命名空间级） | Day 3 / 9 / 11 / 12 / 14 |
| 多线程：`std::thread` / `thread_local` / `atomic` | Day 18 |
| 浮点：NaN / infinity / `fabs` / epsilon / 容差比较 | Day 4 / 6 / 8 / 12 / 13 |
| 运算符重载（该写成员函数还是自由函数） | Day 1 / 8 / 13 |
| 标准库细节：`size_t` / `uint8_t` / `endl` / `steady_clock` / `argv` | Day 11 / 17 / 18 |
| 编译警告与惯用法：`(void)output`、省略参数名 | Day 8 / 10 / 15 |
| 下标代替指针、`vector` 扩容导致指针失效 | Day 8 / 10 |

---

## Day 1 · 头文件 / 构造函数 / 初始化列表 / 运算符重载

**`#pragma once`** —— 告诉编译器"这个头文件在一次编译里只读一遍"，防止重复定义报错。

**`Ray() {}`** —— 不带参数的构造函数（默认构造），`Ray r;` 时被调用；`{}` 是空函数体，什么都不做。

**`const Point3& origin`** —— `const` 表示"我不会改这个参数"；`&` 表示"引用"（不拷贝，直接用原对象）。

**初始化列表** `: orig(origin), dir(direction)` —— "用参数 `origin` 初始化成员 `orig`，用 `direction` 初始化成员 `dir`"。

```cpp
Ray(const Point3& origin, const Vec3& direction)
    : orig(origin), dir(direction) {}
```

等价于：

```cpp
Ray(const Point3& origin, const Vec3& direction) {
    orig = origin;      // 把参数 origin 赋给成员 orig
    dir  = direction;   // 把参数 direction 赋给成员 dir
}
```

三种写法别混：

```cpp
Ray r;               // ① 声明一个变量 r，调用默认构造
Ray r(origin, dir);  // ② 声明一个变量 r，调用带参构造
Ray(origin, dir);    // ③ 不声明变量，创建临时对象
```

### 运算符重载：推荐写法与理由

`Vec3` 里的实际写法（[include/vec3.h](../include/vec3.h)）：

| 运算符 | 签名 | 用法 |
|---|---|---|
| `+` | `operator+(u, v)` | `u + v` |
| `-` | `operator-(u, v)` | `u - v` |
| `*` | `operator*(u, v)` | `u * v`（逐元素乘） |
| `*` | `operator*(double, v)` | `t * v`（标量乘） |
| `*` | `operator*(v, double)` | `v * t` |
| `/` | `operator/(v, double)` | `v / t` |

**推荐写法与理由**

| 运算符 | 推荐写法 | 理由 |
|---|---|---|
| `+=` `-=` `*=` `/=` | **成员函数**，返回 `T&` | 要改左操作数，`this` 天然指向它；返回引用才能链式 `a += b += c` |
| `+` `-` `*` `/`（两个同类对象） | **自由函数**，返回 `T` | 对称、不修改操作数；两侧都能吃隐式转换 |
| `+` `-` `*` `/`（左操作数是其他类型） | **必须自由函数** | `2 * v` 的左操作数不是 `Vec3`，成员函数根本收不到 |
| `<<` `>>` | **必须自由函数** | 左操作数是 `std::ostream` / `std::istream`，不属于我们的类 |
| `==` `!=`（`<` `<=` `>` `>=` 同理） | **自由函数**（C++20 可用 `<=>`） | 对称性：两侧都允许隐式转换，`a == b` / `b == a` 都成立 |
| `[]` `()` `->` `=` | **必须成员函数** | C++ 语法强制（C++23 起 `[]` / `()` 可加 `static`，但也仍是成员） |
| `++` `--`（前置） | **成员函数**，返回 `T&` | 改的是自己，返回引用才支持 `++(++x)` |
| `++` `--`（后置） | **成员函数**，带 `int` 哑参数，返回**旧值的副本** | `int` 参数是它和前置版的唯一区分；返回副本才符合"先取值、后自增" |
| 一元 `-` `!` `~` | 成员或自由都行，返回新值 | 只读操作，跟同族运算符保持一致即可 |

**两条判据，覆盖绝大多数情况**

1. **改不改左操作数？** 改 → **成员函数 + 返回 `T&`**（`+=`、`*=`、`++`）。
   成员函数自带 `this`，本来就是左操作数，不必再多传一个 `T&` 参数。
2. **左右是否对称？** 对称 → **自由函数**（`+`、`*`、`==`）。
   因为**成员函数的隐式 `*this` 参数不参与隐式类型转换**：`v * 2` 能编译（`2` 转 `double`），
   但 `2 * v` 用成员函数永远匹配不上 —— 这正是 `operator*(double, const Vec3&)` 必须是自由函数的原因。

**补充：几条容易漏的细节**

- `operator=` 返回 `T&`（支持 `a = b = c`），并自己处理自赋值；拷贝 / 移动赋值**必须是成员**。
- 算术运算符返回 `T`，**不要**返回 `const T&` —— 引用绑到临时对象上就是悬空的经典来源。
- `operator<<` 返回 `std::ostream&` 而不是 `void`，否则 `std::cout << a << b` 当场断链：
  ```cpp
  inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
      return out << v.x() << ' ' << v.y() << ' ' << v.z();
  }
  ```
- 自由函数要读私有成员时，在类里写 `friend` 声明 —— 它**仍然是自由函数**，对称性不受影响。
  这正是 "hidden friend"（把 `friend` 定义在类内的自由函数）成为 `==` 和 `<<` 主流写法的原因：
  能访问私有成员，又只靠 ADL 查找、不污染命名空间。
  （本项目 `Vec3` 全程走 `x()` / `y()` / `z()`，连 `friend` 都用不上。）
- `operator[]` 成对提供两个版本 —— 可写 `double& operator[](int)` 和只读 `double operator[](int) const`。
  漏掉 `const` 版，`const Vec3` 就没法下标，错误会连锁到一个完全无关的报错上。
- `operator bool` 这类转换运算符一律写 `explicit`，否则会偷偷混进算术 / 比较的隐式转换。
- **不要重载** `&&` `||` `,`：会丢掉短路求值和求值顺序，收益为零。
- **不能重载**：`::` `.` `.*` `?:` `sizeof` `typeid`（`#` `##` 是预处理记号，不属于运算符重载）。
- 比较运算符要成组、语义一致：有 `==` 就该有 `!=`；当排序键时必须满足严格弱序。
- C++20 起 `auto operator<=>(const T&) const = default;` 一次拿到 `< <= > >=`，并**隐式**带出 `==` / `!=`；
  但**浮点类型不要 `= default`**（`NaN != NaN`），本项目的相等要按容差比（见 Day 13）。

**`inline`** —— 给编译器的建议："这个函数很短，直接展开到调用处，别真的函数调用。"

struct默认public，class默认private
---

## Day 2 · 范围 for / 多态 / 初始化

- `for (const auto& obj : objects)` —— 范围 for；`&` 避免每轮拷贝 `shared_ptr`（原子引用计数）
- `obj->hit(...)` vs `obj.hit(...)`：`->` 访问的是**指针指向的那个对象**的成员
- **多态**：`obj` 的静态类型是基类 `Hittable`，实际指向 `Sphere`，调用自动派发到 `Sphere::hit`
  → 这就是 `HittableList` 不需要知道"装的是球还是别的"的原因
- 局部变量必须显式初始化，否则是内存垃圾
- struct 整体赋值 `rec = temp_rec` 会逐字段拷贝
- **成员初始化列表** vs 函数体内赋值：const / 引用 / 无默认构造的成员只能用初始化列表
- **输出参数**（`HitRecord& rec`）实现"一个 bool 装不下的多返回值"

---

## Day 3 · 作用域 / 重载 / 递归 / 局部 static

### 1. 作用域（Scope）
变量从**声明那一刻**起，到它所在的**那一层 `}`** 为止才存在。

    int main() {
        int a = 1;          // a 在整个 main 里活着
        {
            int b = 2;      // b 只在这个 {} 内活着
            a = 5;          // ✅ 内层可以改外层
        }
        b = 3;              // ❌ 错：b 已经死了
    }

三条推论：
- 内层看得见外层，外层看不见内层
- 离开 `{}` 变量就销毁（所以 pixel_color 必须放在采样循环**外面**，否则每次采样都清零）
- **内层同名变量会"遮蔽"外层** —— 编译器不报错，只是行为不对，最阴险

### 2. 函数不能嵌套

    int main() {
        int f() { return 1; }    // ❌ C++ 不允许
        // 报错：a function-definition is not allowed here
    }

    int f() { return 1; }        // ✅ 定义在文件作用域（main 外面）

### 3. 函数重载（Overloading）
两个同名函数，靠**参数个数 / 类型**区分：

    inline double random_double()                          // [0, 1)
    inline double random_double(double min, double max)    // [min, max)

编译器根据你传几个参数自动选。这不是"重复定义"，是合法的。

### 4. 递归（Recursion）
函数调用自己。必须有两样东西：终止条件 + 每次向终止条件靠近。

### 5. 局部 static 变量

    inline double random_double() {
        static std::uniform_real_distribution<double> distribution(0.0, 1.0);
        static std::mt19937 generator;
        return distribution(generator);
    }

函数内的 `static` 变量**只初始化一次**，之后每次调用复用同一个对象。
这里很关键：如果每次都新建 generator，它会被重新播种，随机性会**退化**。

### 6. 声明顺序（C++ 是单遍扫描的语言）
函数用到的类型和函数，必须在它**之前**已经"见过"。
所以 `random_unit_vector()` 返回 `Vec3`，就不能写在 `rtweekend.h`（那里没有 Vec3），
必须写在 `vec3.h` 末尾，同时 `vec3.h` 顶部 `#include "rtweekend.h"`。

---

## Day 4 · override / shared_ptr / 前向声明 / 初始化顺序

### 1. `override` 关键字 —— 强烈建议每次覆盖虚函数都写

```cpp
bool scatter(...) const override { ... }
```

`override` 让**编译器帮你检查**："你确定这是覆盖基类的虚函数吗？"
签名写错一个字（比如忘 `const`），编译器会**报错**而不是悄悄创建一个新函数。

**不写 `override` 的话，这种错误编译器不会告诉你** —— 症状是"运行时材质完全没生效"，极难查。

### 2. `shared_ptr` 与 `make_shared`

```cpp
std::make_shared<Lambertian>(Color(0.9, 0.3, 0.3))
//                  ↑ 类型        ↑ 传给构造函数的参数
```

`make_shared<T>(args...)`：
1. 在堆上创建 `T` 对象（用 `args` 调构造函数）
2. 返回管理它的 `shared_ptr`

比 `shared_ptr<T>(new T(args))` 更好：**内存分配只有一次**（控制块和对象放一起），更快也更安全。

### 3. 在 `HitRecord` 里存 `shared_ptr<Material>` 而不是 `Material*`

1. **共享**：一个材质可以给多个球用（100 个球同一块大理石）
2. **生命周期**：`shared_ptr` 用引用计数自动管理，最后一个持有者销毁时才释放
3. **安全**：裸指针容易忘记释放（泄漏）或悬空（use-after-free）

### 4. 前向声明解决**循环包含**

- `material.h` 需要 `HitRecord`（作为函数参数）
- `hittable.h` 的 `HitRecord` 需要 `Material`（作为成员）
- **互相 include 会死循环**

解法：`material.h` 只用**前向声明**：

```cpp
struct HitRecord;   // 只告诉编译器"有这么个类型"，不给出内容
```

**什么时候够用？**
- 用作**引用 / 指针** → 够用 ✅
- 需要访问**它的成员**（`rec.normal`）→ 不够，需要完整定义 ❌

所以 `material.h` 前向声明即可；`lambertian.h` 要读 `rec.normal`，就必须 `#include "hittable.h"`。

### 5. 三元运算符 `条件 ? 值A : 值B`

```cpp
fuzz(fuzz < 1 ? fuzz : 1)
double eta_ratio = rec.front_face ? (1.0 / refraction_index) : refraction_index;
```

**用途**：初始化列表里没法写 `if`，三元运算符是唯一选择。

### 6. `std::fmin` / `std::fabs` —— 浮点专用版本

| 函数 | 作用 |
|---|---|
| `std::fmin(a, b)` | 返回较小的（浮点安全，处理 NaN、−0） |
| `std::fabs(x)` | 浮点绝对值 |

### 7. 成员的**声明顺序 = 初始化顺序**

```cpp
private:
    Point3 center;
    double radius;                    // ← 先声明
    std::shared_ptr<Material> mat;    // ← 后声明
```

**实际初始化顺序由"声明顺序"决定，而不是初始化列表里写的顺序。**
两边不一致 → 编译器报 `[-Wreorder]` 警告。

当前代码里无害（成员互不依赖），但如果是 `: b(a + 1), a(2)` 这种**有依赖**的写法就会真出错。
**养成习惯：成员声明顺序 = 初始化列表顺序。**

### 8. `struct` 也可以有方法

`HitRecord` 不再只是"一堆字段"，现在有了行为（`set_face_normal`）。
**struct 和 class 在 C++ 里只差默认访问权限**（struct 默认 public，class 默认 private）。

---

## Day 5（当天没有 C++ 语言知识点）

当天的技能点全是工程侧内容（PowerShell 的 `$?` / `$LASTEXITCODE`、命名约定、对照实验），
仍留在 [devlog.md](./devlog.md) Day 5「工程 技能点」。

---

## Day 6 · 构造函数 / this-> / 遮蔽 / 浮点除零

1. **构造函数带参数**：`Camera(Point3 a, Point3 b, ...)`
   对象创建时就把配置全交进去。比"先创建、再一个个 set"更安全 —— **不可能漏设**，而且成员变量不用给默认值（避免"忘了初始化"的隐藏 bug）。

2. **`this->member = param;`**
   当参数名和成员名**故意同名**时，`this->` 明确指向**成员**：

   ```cpp
   Camera(... , double lens_radius) {
       this->lens_radius = lens_radius;   // 左 = 成员，右 = 参数
   }
   ```

   不写 `this->` 的话，`lens_radius = lens_radius` 会变成"参数赋值给自己"，**成员永远是垃圾值**（而且不会报错！）。
   （另一种做法是给成员加前缀 `m_` 或用初始化列表，各有取舍。）

3. **变量遮蔽（shadowing）**
   内层同名变量会"遮住"外层，**同类型时编译器一声不吭**：

   ```cpp
   int s = 0;
   {
       int s = 99;      // 遮蔽：合法，不报错
       // 作用域内的 s 全指向内层这个
   }
   ```

   外层变量毫发无损，但作用域内每一处引用都换了目标 —— 循环计数器会"失控"，程序能跑、结果全错。
   习惯：把警告开到最高（`-Wall -Wextra`，遮蔽警告是 `-Wshadow`）。
   （完整事故复盘见 [devlog.md](./devlog.md) Day 6「今天最重要的一课 · 变量遮蔽」。）

4. **浮点除零 → NaN**：`0.0/0.0` **不报错、不崩溃、不抛异常**，安静地产出 NaN，然后污染整张图。
   → 这是渲染器最阴险的敌人之一。**遇到"莫名其妙的黑图/花屏"，先怀疑 NaN。**

---

## Day 7 · 值返回容器 / make_shared

1. **函数按值返回容器**
   `HittableList random_scene() { ... return world; }` 看起来像"拷贝一整个列表"，其实不会：
   - C++17 的**拷贝省略 / RVO**：编译器直接在调用方的内存里构造 `world`，**零拷贝**
   - 就算真拷贝，里面存的是 `shared_ptr`，成本只是**引用计数 +1**，不是复制 480 个球

   > 口诀：**容器的"值语义"可以放心用**，别为了"省拷贝"到处写指针。

2. **`std::make_shared<T>(...)` 而不是 `new T(...)`**
   - `make_shared` 把"对象"和"引用计数控制块"**一次分配**，更快更省内存
   - 更安全：`new` 出来的裸指针如果中途抛异常 → **内存泄漏**
   - `shared_ptr` 自动销毁，**永远不需要写 `delete`**

   > 规则：**永远用 `make_shared`，永远别自己写 `new`。**

---

## Day 8 · 三目 / fmin / IEEE 754 / 多态

1. **运算符重载的"另一种用法"** —— 用一个三目表达式替代 if-else 产生值
2. **`std::fmin` / `std::fmax` 的 NaN 语义**（标准保证的，不是巧合）
3. **IEEE 754 浮点除零**：`±infinity` / `NaN`，不崩溃。**别乱加特判**
4. **`Vec3()` 默认构造 = `(0,0,0)`，不是"无"** → 表达"空"要用 `bool` 标志
5. **`(void)output;`** —— 消除"未使用参数"警告的一种惯用写法
6. **多态的进一步应用**：同一个虚接口，不同子类给不同实现（`hit` / `bounding_box`）

---

## Day 9 · lambda / sort / 委托构造 / static

1. **lambda 与捕获列表**：`[axis](...){...}` ——
   把外部变量"复制进"一个匿名函数，才能塞给 `std::sort` 当比较器
2. **`std::sort(起点, 终点, 比较函数)`**
   —— 两个迭代器而不是"数组 + 长度"
3. **成员函数 vs 静态函数**：`对象.函数()` vs `类名::函数(参数)`
   （`longest_axis()` 是成员，`surrounding()` 是静态）
4. **构造函数委托**：`BVHNode(HittableList list) : BVHNode(list.objects, 0, ...) {}`
   —— 一个构造函数调用另一个完成工作
5. **`static` 成员函数**：`make_inv` 不依赖任何对象，所以可以是静态的
6. **按值传参当"拷贝一份"用**：`BVHNode(HittableList list)` 故意拷贝，
   这样建树时排序不会弄乱调用方的列表

---

## Day 10 · std::move / 显式栈 / vector 失效

1. **`std::move`**：`objects(std::move(list.objects))`
   把参数里的 vector **"搬"** 成成员，不复制一份
2. **整数字下标代替指针/智能指针**：
   更省内存、没有引用计数开销、不怕 `vector` 扩容（见 Q3）
3. **显式栈**代替递归：省函数调用、可控、不怕深递归
4. **`(void)output;`、`struct Node` 嵌套定义**、`nodes[index]` 重复寻址
5. **`vector` 扩容 → 指针/引用/迭代器全部失效**（今天最重要的 C++ 陷阱）
6. **看汇编**：`g++ -O3 -S -masm=intel` —— 不用读懂全部，搜指令名就行
7. **`std::fmax` / `std::fmin` 会生成函数调用**（NaN 语义导致 GCC 不敢用指令）

---

## Day 11 · 声明与定义 / 头文件 / 计时 / argv

| 知识点 | 要点 |
|---|---|
| **声明 vs 定义** | 用之前只需"声明"；`'x' was not declared` = 编译器没见过这个名字 |
| **单遍编译** | C++ 从上往下读，名字必须**先声明后使用**（和 Python 相反） |
| **前向声明** | `HittableList random_scene(int);` —— 一行预告，解决顺序问题 |
| **头文件的本质** | `#include` = 预处理器把被包含文件**原样粘贴**到当前位置 |
| **指针 vs 引用** | 引用表达"必须有"，指针表达"可能有"（可 `nullptr`） |
| **`steady_clock`** | 单调递增，适合计时；`system_clock` 会被校时跳变 |
| **函数内 `static`** | 隐藏的全局状态；破坏可重复性；要留"重置"的口子 |
| **单位一致性** | 变量名写 `_ms` 就真的要是毫秒 —— 编译器不管，数据会管 |
| **`argc` / `argv`** | `argv[0]` 是程序名；解析循环从 1 开始；要检查 `i+1 < argc` |

---

## Day 12 · static 成员 / sqrt 定义域 / 色彩空间

| 知识点 | 要点 |
|---|---|
| **`static` 成员函数** | 属于类、不属于对象；没有 `this`，不能访问非静态成员 |
| **为什么用 static** | 纯计算、不需要对象状态时，写得越"无依赖"越好测、越好推理 |
| **`std::sqrt` 的定义域** | 负数 → 返回 `NaN`（不是报错！）；`NaN` 会**传染**整个表达式 |
| **`std::pow(x, 5)`** | 比 `x*x*x*x*x` 慢；小整数次幂可以用连乘代替（**但先测再优化**） |
| **职责分离** | 色彩空间转换不写在渲染循环里，单独一个 `color.h` |
| **魔法数字要有边界条件** | Schlick 的"5 次方"是拟合值；知道它在 0 和 1 两头是对的 |

**补记（Day 12 收尾追加的两条）**

- **平台头文件污染全局命名空间**：`<windows.h>` 会带进 `typedef unsigned char byte;` —— 别用 `byte` 当变量名。
- **标准名字要带 `std::`**：`fmin` 在 MinGW 能编译（实现把名字也放进了全局），但**标准不保证**。

---

## Day 13 · 构造函数缓存 / epsilon / 成员 vs 局部

| 知识点 | 要点 |
|---|---|
| **构造函数里做缓存** | 平面参数算一次，求交时零成本（空间换时间） |
| **成员默认初始化** | `Vec3 w;` 默认构造为 (0,0,0)；`double D = 0.0;` 显式给值 |
| **`virtual` 成员函数** | `is_interior` 写成 virtual，是为了明天三角形能覆盖它 |
| **`Vec3` 的可写 `operator[]`** | `minimum[i] -= x` 能编译，说明返回的是 `double&` |
| **`std::fabs` vs `std::abs`** | 浮点用 `fabs`；`abs` 在 `<cstdlib>` 里是给整数的（重载容易踩坑） |
| **`1e-8` 这种 epsilon** | 浮点不能直接和 0 比较；"够小"就当作 0 |
| **成员 vs 局部** | `double D = dot(normal, Q);` 在构造函数里是**新建局部变量**，成员 `D` 仍是 0。写了类型就是新变量，不写 `this->` 才是赋值给成员 |

---

## Day 13 · 补充：static_cast / 浮点比较

| 知识点 | 要点 |
|---|---|
| **`static_cast<const Hittable&>`** | 两个不同派生类要装进同一个引用，需要显式转换到共同基类 |
| **三元运算符返回左值** | `use_bvh ? (const Hittable&)world : (const Hittable&)world_list` 两条分支类型必须一致，否则编译失败 |
| **`std::fabs` 需要 `<cmath>`** | 依赖别的头文件"顺带"引入是不稳定的，自己用的头文件自己 include |
| **浮点相等用容差比** | `fabs(a - b) < 1e-12`，永远不要 `a == b` |
| **`0.0001 / 2.0` 是精确的** | 除以 2 只是指数减 1，尾数不动，所以 `delta/2` 和字面量 `0.00005` 位模式相同 |

---

## Day 14 · override / 基类构造 / vtable

| 知识点 | 要点 |
|---|---|
| **`override` 关键字** | 写在派生类函数后面，编译器会检查"你确实覆盖了基类虚函数"。签名写错（少个 `const`、参数类型不同）会立刻报错，而不是悄悄变成一个新函数、永远不被调用 |
| **初始化列表调用基类构造** | `Triangle(...) : Quad(a, b-a, c-a, mat) { }` —— 派生类构造前必须先构造基类 |
| **基类构造先于派生类构造体** | 所以 `Quad` 里 `set_bounding_box()` 用的是 `b-a`、`c-a` 时，`Triangle` 的构造体还没开始执行 |
| **虚函数表（vtable）** | `Quad::hit()` 里调 `is_interior(...)`，实际执行 `Triangle::is_interior`。这就是多态 |
| **位置 − 位置 = 向量** | `b - a` 返回 `Vec3`，正好当边向量；`Q + 向量` 得到位置 |
| **`static` 命名空间级变量** | `static bool g_normal_shading` 只在本编译单元可见，避免和其他文件的同名变量冲突 |
| **`AABB(Q, Q)` 是合法调用** | 零尺寸盒子不会崩，`pad_to_minimums()` 会把它撑成 0.0001 的小方块 |

---

## Day 15 · const 签名 / 虚函数钩子

| 知识点 | 要点 |
|---|---|
| **`const` 是签名的一部分** | `f(double)` 和 `f(double) const` 是两个函数，可以共存 |
| **const 成员函数只能调 const 成员函数** | 否则"间接修改自己"，契约破裂 |
| **`override` 抓签名不匹配** | 少了 `const`、参数类型不同 → 立刻报错，而不是悄悄变成新函数 |
| **省略参数名** | `void f(double, double) const` —— 保留签名但不引入变量，天然没有 unused 警告 |
| **返回值 vs 原地修改** | `unit_vector(n);` 单独一行是空操作；`const Vec3&` 参数改不了原值 |
| **虚函数钩子（模板方法）** | 基类写死流程，只把变化的部分做成 `virtual` |
| **`Quad::set_hit_normal(...)`** | 显式限定作用域，跳过虚函数分派，直接调基类实现 |

---

## Day 16 · 基类非纯虚函数 / 参数必须用

| 知识点 | 要点 |
|---|---|
| **给基类加非纯虚函数** | `virtual Color emitted() const { return Color(0,0,0); }` —— 新增能力但不强制派生类实现，已有的三个材质**零改动** |
| **收进来的参数必须用** | 第一版 `DiffuseLight::emitted()` 返回硬编码 `Color(15,15,15)` 而不是成员 `emit` —— 构造函数收到的值被丢掉了。在 Cornell Box 上看不出来（恰好也是 15），做彩色灯时才炸 |
| **值返回小对象** | `Color` 只有 3 个 double（24 字节），值返回比返回引用更安全 |

---

## Day 17 · 能力查询 / 夹边界 / size_t

| 知识点 | 要点 |
|---|---|
| **给基类加能力查询** | `virtual bool is_diffuse() const { return false; }` —— 比 `dynamic_cast` 快，而且把"我能不能被采样"交还给材质自己回答 |
| **递归函数加状态参数** | `bool skip_emitted` 随递归链传递，比全局变量安全（不会串味） |
| **`int(random_double() * N)` 要夹边界** | `random_double()` 返回 `[0,1)`，浮点可能给出正好 1.0 → `if (idx >= N) idx = N-1;` |
| **`std::size_t(idx)`** | 容器下标用无符号，避免 `-Wsign-conversion` |
| **`-wi` 不修改 `wi`** | `operator-` 返回新向量（同 Day 15 的 `unit_vector` 教训） |
| **`Color / double`** | `Vec3` 有逐元素除法，`cos_surf / pdfw` 可以直接接在颜色后面 |

---

## Day 18 · thread_local / std::thread / atomic

| 知识点 | 要点 |
|---|---|
| **`thread_local`** | 每个线程一份独立副本，首次使用时构造。替换 `static` 就让 `rng()` 变成线程安全的 |
| **`std::thread`** | `std::thread t(函数, 参数...)`；**必须 `join()`**，否则 `main` 返回时 `std::terminate` |
| **`std::vector<std::thread>`** | 批量建线程，最后统一 join |
| **lambda 捕获** | `[&, tid]` —— 引用捕获共享数据，按值捕获该线程专属的编号 |
| **`std::atomic<int>`** | 多线程计数器的正确写法；`++x` 是原子的 |
| **`std::uint8_t`** | 明确 8 位无符号，比 `unsigned char` 语义清楚 |
| **`std::uint64_t` 避免溢出** | `height * tid / nthreads` 里先转成 64 位，防止 4K 图时中间结果溢出 |
| **主线程也干活** | 开 `nthreads-1` 个额外线程，主线程算第 0 块，省一次线程创建 |
| **不用 `std::endl`** | 它每次都 flush，热循环里极慢；用 `'\n'` |
