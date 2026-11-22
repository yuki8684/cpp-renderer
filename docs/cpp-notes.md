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

---

## Day 20 · 流 / 字符串 / 逗号运算符

| 知识点 | 要点 |
|---|---|
| **`std::ifstream`** | `std::ifstream in(path);` 只读打开；`if (!in)` 判断打开失败（路径错 / 文件不存在） |
| **`std::getline(in, line)`** | 每次读一整行，**读完返回 false**，所以能直接当 `while` 条件 |
| **`std::istringstream`** | 把字符串包成"可以逐词读"的流；`line[0] == '#'` 用 `[]` 取字符判注释 |
| **`>>` 是左结合的** | `ss >> x >> y >> z` 读作 `(((ss>>x)>>y)>>z)`，每个 `>>` 作用在上一个返回的流上 |
| **★ 逗号不是分隔符** | `ss >> x, y, z` 是**逗号运算符**：只读了 `x`，`y`/`z` 是**未初始化垃圾**。`-Wall` 报 `right operand of comma operator has no effect` |
| **`getline(ts, s, '/')`** | 按自定义分隔符切；分隔符本身被吃掉不放进 `s`；流会被**消耗**，可以连续调 |
| **`std::stoi(s)`** | 字符串转 int；**不做边界检查**，`"7abc"` → 7、`" 12 "` → 12；**空串会抛异常**（不是返回 0） |
| **字符串判空用 `empty()`** | `std::string` 是对象不是指针。**指针判 `nullptr`，字符串判 `empty()`**；`s != nullptr` 是编译错误 |
| **`push_back` 只收 1 个参数** | 要塞 3 个值就写 3 行。编译器会列一屏 `note:` 说"所有重载都接不了 3 个参数" |
| **`std::size_t` 做循环变量** | 和 `.size()`（无符号）比较时避免 `-Wsign-compare` |
| **`i + 2 < v.size()`** | 遍历"每 3 个一组"的安全写法，比 `i < v.size() - 2` 好（后者在空容器时无符号下溢） |
| **★ 表达式结果被丢弃** | `f(x);` / `ok == false;` / `a, b, c;` —— **没赋值、没副作用 = 白写**。自查：这行 `;` 前面那坨东西，结果去哪儿了？ |
| **`statement has no effect [-Wunused-value]`** | 这句警告几乎总是"写了比较却忘了赋值"，或"逗号运算符用错了" |
| **`defined but not used [-Wunused-function]`** | 可以当**待办清单**用：static 函数没被调用 = 对应的测试还没写 |
| **`Tee-Object -Variable out`** | PowerShell 里既显示输出又留一份副本，用来事后筛 `[FAIL]` 行 |

---

## Day 21 · 容器归零 / 表达式结果丢弃的盲区

| 知识点 | 要点 |
|---|---|
| **`v.assign(n, value)`** | 把容器改成"n 个 value"（会清空原有内容）。比 `clear()` + 循环 `push_back` 简洁。**语义是"重置"，不是"追加"** |
| **`const Point3& a = d.positions[ia];`** | 引用绑定，不拷贝（`Point3` 是 24 字节，循环里拷贝累积可观）。`const` 表示只读 |
| **★★ 类类型的"结果被丢弃"编译器不报** | `d.normals[i] / std::sqrt(L2);` 返回值被扔掉，**`-Wunused-value` 不触发**。因为 `operator/` 是函数调用，编译器无法证明它没有副作用。**只有内置类型（比如 `bool`）的结果被丢弃才会警告** |
| **多余括号 `(x = y);`** | 和 `x = y;` 完全等价。赋值表达式的值就是被赋的值，外层括号不改变任何东西。**无害但让人困惑** |
| **`return` 之后的代码** | 编译通过、无警告，但**永远不执行**。自查：缩进为 0 的语句 = 位置错了；改完看"共 N 项"涨没涨 |
| **`std::fabs` / `std::sqrt` / `std::acos`** | 都在 `<cmath>`。`std::acos` 的输入必须夹在 `[-1,1]`，`acos(1.0000000001)` 返回 **NaN** |
| **除零 → NaN 会扩散** | 一个 NaN 进包围盒会让整棵 BVH 失效。**归一化前必须判零** |
| **`i / NV` 与 `i % NV`** | 把一维下标拆成二维：`i / N` 是外层编号，`i % N` 是内层编号。**外层循环步长 = 内层总数** |
| **整数除法要显式转 double** | `2.0 * pi * ii / NU` 里 `2.0` 和 `pi` 已经是 double，`ii` 会自动提升；但 `ii / NV` 两个都是整数会截断 |

---

## Day 22 · 聚合初始化 / floor / 纹理抽象

| 知识点 | 要点 |
|---|---|
| **聚合体只能用 `{}` 初始化（C++17）** | `struct UV{double u,v;}` 没有构造函数 → `UV(a,b)` 报 `no matching function`，必须 `UV{a,b}`。C++20 才允许带括号的聚合初始化 |
| **怎么判断是不是聚合体** | 没有用户提供的构造函数、无私有/保护成员、无虚函数、无基类 → 是聚合体 |
| **`Vec3(1,2,3)` 能用圆括号** | 因为它有真正的构造函数，不是聚合体 |
| **`std::floor` vs `int()`** | 正数时相同；负数分家：`floor(-0.3) = -1`，`int(-0.3) = 0` |
| **坐标 → 格子索引必须用 floor** | 接缝处浮点误差会让坐标变成 `-1e-17`，`int()` 会落到错的格子，颜色突变一个像素 |
| **负数 `%` 的符号** | C++ 里跟着被除数：`-3 % 2 == -1`。所以奇偶判定写 `n % 2 == 0`，别写 `== 1` |
| **把特例变成通例** | `Lambertian(Color)` 内部包成 `SolidColor`，`scatter()` 只留一条路径 → 老代码零改动就能贴图 |
| **给虚函数加参数要同步改所有点** | `diffuse_albedo()` → `diffuse_albedo(const HitRecord&)`：基类声明、派生类覆盖、调用处三处都要改 |
| **三套索引的设计动因** | `f v/vt/vn` 是因为位置/UV/法线是三种独立的"缝合方式"，接缝处各属性的副本数不同 |
| **用对称图案验证不了方向** | 棋盘做 `1-v` 翻转后，若 `nu,nv` 都是偶数则看起来完全一样 → 测方向必须用不对称图案 |

---

## Day 23 · 函数不能嵌套 / 二维叉积

| 知识点 | 要点 |
|---|---|
| **C++ 不允许函数嵌套定义** | 把 `static double f(...)` 写在 `main()` 里面 → `error: a function-definition is not allowed here before '{' token`。函数只能定义在文件作用域（顶格） |
| **想在函数里建小函数用 lambda** | `auto f = [](double x) { return x * 2; };` —— lambda 是**语句**，允许写在函数内 |
| **看错误信息定位置** | `is not allowed here` 这类错先看**缩进**：应为顶格的函数定义被缩进了，就是位置错了 |
| **二维叉积** | `e1u*e2v - e1v*e2u` = 两条边张成的**有符号平行四边形面积**；三角形取一半 |
| **符号 = 绕向** | 正 = 逆时针，负 = 顺时针。想验绕向一致性就不能加 `fabs` |
| **测试要保护对照组** | A/B 实验里，“B 组确实是 B”也需要断言；否则对照组被悄悄改了，其余测试全绿 |
| **文件名也是契约** | 测试里硬编码的路径写错不会报错（`load_obj` 只打印提示并返回空）→ 一整组断言全红，但原因不是代码 |

---

## Day 24 · 智能指针 / explicit / 双线性 / sRGB

| 知识点 | 要点 |
|---|---|
| **`std::shared_ptr<T>`** | 带引用计数的指针，最后一个拥有者放手时自动析构。多张纹理共享一张图时不用关心谁 `delete` |
| **`shared_ptr<const T>`** | 共且只读。用类型表达“这张图不会被改”，比写注释可靠 |
| **`explicit`** | 禁止隐式类型转换。单参数构造函数默认都加；数值类型（`Vec3`）故意不加 |
| **`std::move`** | 不移动任何东西，只是把对象标成“可以被掏空”（转右值）。真正的搬运在移动构造函数里 |
| **按值传 + move** | 形参按值收（拷贝一次），再 `std::move` 给成员（省一次原子操作）。是现代 C++ 的常见写法 |
| **静态工厂 `static T::load()`** | 构造函数无法优雅地失败（只能抛异常或造半残对象）；工厂可以返回“一眼能认出来的坏值”（洋红） |
| **返回 `const T&`** | 省拷贝；但必须先确认“对象活得够久” —— 返回局部变量的引用是悬空引用（UB） |
| **二维 → 一维下标** | `index = y * width + x`。一维连续内存比 `vector<vector<T>>` 缓存友好得多 |
| **`std::vector::operator[]` 不检查边界** | 越界是 UB。调试期用 `.at()`（越界抛异常），性能测试再换回 `[]` |
| **双线性插值** | 4 个邻点，先横向插两次、再纵向插一次。1 个邻点 = 最近邻（马赛克） |
| **像素中心 `- 0.5`** | 像素是方块不是点，中心在 `k + 0.5`。不减会让插值在 `u = k/w` 处退化成最近邻 |
| **`1 - v` 翻转** | 图片第 0 行在顶部，OBJ 的 `v=0` 在底部。不翻就上下颠倒 |
| **`%` 对负数返回负数** | C++ 的截断除法。要模运算得补 `if (i < 0) i += n;` |
| **sRGB 分段解码** | 暗部直线（`s/12.92`）+ 其余幂函数（`pow((s+0.055)/1.055, 2.4)`）。暗部换直线是因为纯幂函数在 0 附近斜率无穷大 |
| **互逆函数用往返验** | `decode(encode(x)) == x` 一次验证两个函数。实测误差 3.33e-16（浮点极限） |
| **颜色图 vs 数据图** | 反照率按 sRGB 解码；**法线图/粗糙度图绝不能解码**（它们已经是线性数据） |
| **`touch` 才能触发重编** | `Move-Item` / `git checkout` 还原文件会保留旧 mtime → make 认为“没变过” → 不重编 → **实验白做** |
| **别用 `\| Out-Null` 吞构建输出** | 吞了就看不到“编译了 0 个文件” |

---

## Day 25 · 前向声明 / 分层边界 / 常量替身

### 1. 前向声明只够“提一嘴”，不够“用它”

```cpp
class Texture;                                  // 前向声明

std::shared_ptr<Texture> t;                     // ✓ 可以
void f(Texture* p);                             // ✓ 可以
Texture& g();                                   // ✓ 可以
p->value(u, v, p);                              // ✗ 不行：complete type required
std::make_shared<Texture>();                    // ✗ 不行：要 sizeof
Texture obj;                                    // ✗ 不行：要 sizeof
```

编译器看到 `p->value(...)` 时才第一次需要知道 **`Texture` 有哪些成员**。
前向声明里什么成员都没有，于是报：

```
error: invalid use of incomplete type 'class Texture'
note:  forward declaration of 'class Texture'
```

**规则：**

| 头文件里出现的东西 | 需要什么 |
|---|---|
| `T*` / `T&` / `shared_ptr<T>` 作为**参数或返回值** | 前向声明够 |
| 调用 `T` 的**成员函数**、访问成员 | 必须 `#include` |
| 按值持有 `T`、`make_shared<T>`、`sizeof(T)` | 必须 `#include` |

### 2. “编译过了”不等于“依赖对了”

Day 25 加了 `apply_normal_map()` 后：

- `src/main.cpp` 编译**通过** —— 因为它先 include 了 `image_texture.h`，顺带拉进了 `texture.h`
- `tests/obj_test.cpp` 编译**失败** —— 它只 include 了 `obj_loader.h`

**同一个头文件、两种结果。** 这说明 `triangle.h` 缺 include，
只是被 main.cpp 的 include 顺序**碰巧掩盖**了。

> 这种 bug 的特征：**换一个 include 顺序就炸**。
> 单元测试的价值之一就是它有不同的 include 顺序，能提前引爆。

### 3. 测试替身（stub）：用最简实现替换依赖

要测“切线帧把 (1,0,0) 变成什么”，不需要真的渲染。
只需要一个**永远返回同一个值**的纹理：

```cpp
class ConstantTexture : public Texture {
public:
    explicit ConstantTexture(const Color& c) : c(c) {}
    Color value(double, double, const Point3&) const override { return c; }
private:
    Color c;
};
```

和**只实现被测方法**的材质：

```cpp
class StubMaterial : public Material {
public:
    explicit StubMaterial(std::shared_ptr<Texture> nm) : nm(nm) {}
    bool scatter(const Ray&, const HitRecord&, Color&, Ray&) const override { return false; }
    std::shared_ptr<Texture> normal_map() const override { return nm; }
private:
    std::shared_ptr<Texture> nm;
};
```

好处：

- **速度快**：不用建整个场景
- **结果可精确比对**：常量输入 → 常数输出，可以直接写 `==`（容差 1e-9）
- **隔离**：测试失败一定在切线帧，不会怪到光照/BRDF 头上

### 4. 分层边界的自证：解码只在一层做

第一版测试**全部失败**，得到 `(0.408, 0.408, 0.816)`。
原因是我用了 `ConstantTexture` 直接返回 `(0.5, 0.5, 1.0)`，
绕过了 `NormalMapTexture` 里的 `2c − 1` 解码。

| 层 | 职责 |
|---|---|
| `NormalMapTexture` | 解码：0~1 颜色 → −1~1 单位法线 |
| `Triangle::apply_normal_map` | 变换：TBN 局部 → 世界 |

**解码只在这一层做**，所以绕过它就会拿到编码值。
如果当初把 `2c − 1` 也复制进 `Triangle`（“省一次调用”），
这个 bug 就永远暴露不出来，只会表现为“渲染光照怪怪的”。

> 分层清晰的一个副产品：**你能用测试精确指出错在哪一层。**
> 分层混乱的副产品：只能靠眼睛猜。

### 5. 退化判断用 `!(x > eps)` 而不是 `x < eps`

```cpp
if (!(std::fabs(det) > 1e-20)) return;   // ✓ NaN 会进入 if（正确拒绝）
if (std::fabs(det) < 1e-20) return;      // ✗ NaN 比较永远 false（误判为有效）
```

NaN 参与任何比较都是 `false`。取反之后，`!(false) == true`，
所以 NaN 会走“退化”分支 —— 这才是我们要的。

### 6. 一条来自画面之外的教训

变异 `cross(N,T)` → `cross(T,N)`：

```
单元测试：9 项中 2 项红（N3、N8）
渲染：63% 的像素分量变了，平均差仅 3.74，最大 92
观感：凸起变凹坑 —— 但“看起来不坏”
```

**平均差小 ≠ 没 bug。** 这个 bug 恰好是“整体规律性改变”，
大量像素各变一点点，平均值被摊薄了。

正确的判据不是“差多少”，而是**“有没有一条断言专门盯这个位置/含义”**。
N3 就是那条断言：`副切线 (0,1,0) → B`。
---

## Day 26 · 测试的射程 / 重构两步法 / 零向量

### 1. ★★ 断言只能验证「一致性」，验证不了「意图」

Day 26 的规格我一开始写错了（`cross(U,V)` 的符号），数据表按错规格填，
结果 **15 条断言全绿，但画面是镜像的**。

那条断言长这样：

```
O13 六个面手性一致（cross(U,V) == 外法线的负数）    PASS
```

它检查的是「六个面**彼此**一样」。规格本身错了，它照样绿 —— 甚至更绿，
因为错得更整齐。

| 断言能验证 | 断言验证不了 |
|---|---|
| 数据符合**你写的规格** | 规格本身对不对 |
| 六个面彼此一致 | 它们整体是否是你要的效果 |
| 关系成立 | 关系是否是你要的关系 |

**所以要分层看验收集合：**

```
编译通过   →  语法对
单元测试绿 →  数据符合规格
出图看一眼 →  规格符合意图      ← 这一层无法自动化
```

任何一层缺了，都有一种 bug 专门从那一层的缝里钻过去。

### 2. 重构两步法：先证「等价」，再改「行为」

Day 26 把生成器从「按顶点顺序发 UV」改成「点积投影」，这是一个**结构改动**。
紧接着又要改**数据值**。两件事混在一起，出问题就分不清是谁的锅。

做法是拆成两次：

```
第一步：用【旧值】跑【新代码】 → 生成的 OBJ 必须与旧文件逐字节相同
        证明：重构本身没改变任何东西
第二步：只改表里的数字 → 行为变化一定来自数字
```

逐字节比对时把注释行排除掉（注释要更新）：

```powershell
$a = (Get-Content old.obj | Where-Object { $_ -notlike '#*' }) -join "`n"
$b = (Get-Content new.obj | Where-Object { $_ -notlike '#*' }) -join "`n"
$a -eq $b
```

> 这跟 Day 13 的「两步验收」（先关 BVH 后开 BVH 字节相同）是同一个手法：
> **一次只动一个变量。**

### 3. 零向量和非单位向量当「方向轴」

`tu = (dot(P, U) + 1) / 2` 这种投影写法，对 `U` 有两个隐含要求：

| U 的问题 | 症状 |
|---|---|
| `U = (0,0,0)` | `dot` 恒为 0 → `tu` 恒为 0.5 → 所有 UV 的 u 相同 → 三角形共线 → `det = 0` → **整个面的 UV 消失** |
| `|U| = √2`（`(0,1,1)`） | UV 被拉伸 2 倍，每个面的 UV 面积不再是 1 |

**第一种特别隐蔽**：面还在、法线还在，只是 UV 全塌了。
面积和不变量（`O14`：六面面积和 = 6.0）能抓到 —— 实测得到 5.0。

> 通用规则：**任何被当成"基向量"的东西，都要断言 `|v| == 1` 且与其他基正交。**
> 这和 Day 21 顶点法线要用 `|n| == 1` 抓"结果被丢弃"是同一个套路。

### 4. 一个符号错误的高效自检

填完一组基向量，手算 `cross(U, V)` 和目标法线比：

- 等于 `+n` 而你要 `−n`（或反之） → **把 U 反号**
- 长度不是 1 → 向量写错了
- 某个分量恒为 0 但应该是 ±1 → 选错了轴

**一行叉积，同时检查正交性、手性、单位长度。**

### 5. PowerShell：数组的数组

```powershell
$a = @(
    @( 1, 0, 0,   0, 1, 0),
    @( -1, 0, 0,  0, 1, 0)
)
$a.Count       # 2
$a[0][3]       # 0
```

**注意**：从函数**返回**数组的数组时会被展平，必须用前置逗号 `,@(...)`：

```powershell
function Triples {
    $out = @()
    $out += ,@(1,2,3)     # ✓ 保持三元组
    # $out += @(1,2,3)    # ✗ 变成 3 个独立元素
}
```

同一个坑 Day 20 在 `gen_obj.ps1` 的 `$icoNorm` 那里踩过一次。

---

## Day 27 · `[[maybe_unused]]` / 遮蔽 / 累加键

### 1. `[[maybe_unused]]`：给「暂时还没用到」的东西开口子

留空骨架时常见两个警告：

```
warning: unused parameter 'alpha' [-Wunused-parameter]
warning: unused variable 'a'     [-Wunused-variable]
```

以前的做法是加一行 `(void)alpha;` —— 但它**在代码写完之后就变成死代码**，得记得删。

C++17 的写法：

```cpp
void f([[maybe_unused]] double alpha) { /* 还没实现 */ }
[[maybe_unused]] const Point3& a = ...;
```

实现之后它自动失效，不用回头清。**留填空题时用它最省事。**

### 2. 局部变量遮蔽了 `std::tan`

```cpp
double w_a = 1.0 - alpha - beta;
Vec3 tan = w_a * tan0 + alpha * tan1 + beta * tan2;   // ⚠️
```

`tan` 是 `<cmath>` 里的函数名。局部变量叫 `tan` **合法**，但在这个作用域内 `tan(x)` 就调不动了（`std::tan(x)` 还是可以的）。

**规则：别用标准库函数名当变量名。** 同理要避开的：`abs` `div` `left` `right` `distance` `count` `min` `max` `data` `size` `time` `exp` `log` `pow` `gamma` `beta`。

> 改一个字母的事：`Vec3 t`、`Vec3 tan_v`。

### 3. 累加时要选对「键」

Day 21 的顶点法线按 `position` 索引累加，没问题 —— 因为法线只跟位置有关。

Day 27 的顶点切线按 `position` 索引累加，**在 UV 不连续的模型上就错了**：

```
立方体：8 个位置，24 个 UV
每个角被 3 个面共享，3 个面给它 3 个不同的 UV
→ 3 个面的切线被加到一起 → 无意义的平均
```

**正确的键是 `(position, uv)` 配对。** OBJ 的 `v` / `vt` 双索引就是为这件事准备的。

> **通用教训：累加器按什么分组，取决于「什么变了这个量就该变」。**
> 法线只依赖位置 → 按位置分组；
> 切线依赖位置 **和** UV → 按 (位置, UV) 分组。

### 4. 空实现自查法：一条断言是不是假绿

写测试时最怕「数据是垃圾，断言却绿」。最快的自查：

```
把被测函数体改成 return;（或直接 return 默认值）
重新跑测试
```

**还绿的每一条都是假绿。**

Day 27 实测抓到三条：

| 断言 | 为什么空实现还绿 |
|---|---|
| `全部是单位向量` | 循环里 `continue` 跳过了零向量，最大值一直是 0 |
| `全部垂直于法线` | `dot(零向量, n)` 恒等于 0 |
| `夹角 ≤ 0.5°` | 除零产生 NaN，`std::fmax` **忽略 NaN**，最大值停在 0 |

修法：把「有没有垃圾数据」本身也写进判据。

```cpp
check_bool("S4 全部垂直于顶点法线（且无零向量）",
           !any_zero && max_perp_err <= 1e-9);
```

### 5. `std::fmax` / `std::fmin` 会忽略 NaN

```cpp
std::fmax(0.0, std::nan(""))   // → 0.0    不是 NaN！
std::fmax(std::nan(""), 5.0)   // → 5.0
```

这是 C 标准规定的：`fmax` 返回「另一个参数」当其中一个是 NaN。

**好处**：NaN 不会污染「求最大值」的结果。
**坏处**：**NaN 会被静默吞掉** —— 上面第 4 节的三条假绿就有一条是它造成的。

> 想让 NaN 冒出来：用 `a > b ? a : b`（NaN 参与比较恒为 false，会选 b），或者显式 `std::isnan` 检查。

---

## Day 28 · 二进制 I/O / 端序 / 编译警告

### 1. `>>` 只适合文本，二进制必须用 `read()`

```cpp
in >> x;                 // ✗ 对二进制：会跳过空白、试数字解析、受 locale 影响
in.read(ptr, n);         // ✓ 原样搬 n 个字节
```

`read()` 的签名：

```cpp
std::istream& read(char* s, std::streamsize n);
```

注意参数是 `char*` —— 所以 `unsigned char` 缓冲区要 `reinterpret_cast`：

```cpp
std::vector<unsigned char> buf(n);
in.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
```

### 2. `read()` 不抛异常，用 `gcount()` 检查

```cpp
if (in.gcount() != static_cast<std::streamsize>(buf.size())) {
    // 文件被截断了
}
```

`gcount()` 返回**上一次非格式化输入操作实际读到的字节数**。
不检查的后果：剩下的缓冲区保持初值（`vector<unsigned char>` 是 0），
表现为"加载成功、但下半张图全黑"。

> **对比**：`std::vector::at()` 越界抛异常、`[]` 不检查。
> C++ 的 I/O 也是这个风格 —— **默认不检查，检查是你的事**。

### 3. 端序（endianness）

```
大端（big-endian）：高字节在前      256 = 0x01 0x00
小端（little-endian）：低字节在前   256 = 0x00 0x01
```

- **x86 / x86-64 / ARM（默认）：小端**
- **网络协议（TCP/IP）、PPM/PNG 等多字节整数：大端**

所以：

```cpp
uint16_t v;
std::memcpy(&v, buf + p, 2);        // ✗ 在 x86 上读出反的值
s = (buf[p] << 8) | buf[p + 1];     // ✓ 显式拼，跨平台正确
```

> **`0x01 0x00` vs `0x00 0x01` 是最省事的端序测试用例** ——
> 两个值（256 和 1）互为字节交换，一眼就能看出有没有读反。

### 4. `unsigned char` 才能安全左移

```cpp
char  c = 0xE6;              // 有符号 → -26
c << 8;                      // ✗ 负数左移是 UB
unsigned char u = 0xE6;      // 230
u << 8;                      // ✓ 整数提升成 int，230*256 = 58880
```

**处理二进制数据一律用 `unsigned char`。**

### 5. `compile=0` 不代表代码对

```cpp
Image f() {
    // 忘了 return
}                            // → warning: control reaches end of non-void function
                             // → 退出码依然是 0
```

C++ 标准说这是 **UB（未定义行为）**，不要求编译器报错。GCC/Clang 给
`-Wreturn-type` 警告，MSVC 给 `C4715` 警告 —— 都**不是错误**。

> **习惯：把 `warning:` 当 `error:` 处理。**
> 项目长期保持 0 warning，就是为了让"新冒出来的那一条"永远醒目。

### 6. 不要用 PowerShell 改写带中文的源码

```powershell
$s = Get-Content src\main.cpp -Raw                 # ✗ 无 BOM 的 UTF-8 被当 GBK 读
Set-Content src\main.cpp -Value $s -Encoding utf8  # ✗ 写出带 BOM 的 UTF-8
```

结果：中文全变乱码 + 文件多了 BOM，编译报
`extended character ... is not valid in an identifier`。

**正确做法**：

```powershell
$s = [System.IO.File]::ReadAllText($p, [System.Text.Encoding]::UTF8)
[System.IO.File]::WriteAllText($p, $s, (New-Object System.Text.UTF8Encoding($false)))
```

或者干脆用编辑器 / 编辑工具改，别用 PowerShell 碰源码。

---

## Day 29 · 嵌套函数 / 虚函数默认实现 / 恒等重构

### 1. C++ 函数只能定义在「文件作用域」或「类作用域」

这个坑 Day 23 已经踩过一次（`uv_area` 写在 `main()` 里），今天原样重演：

```cpp
bool scatter(...) const override {
    ...
    double scattering_pdf(...) const override {   // ✗ 函数里定义函数
        ...
    }
}
```

```
error: a function-definition is not allowed here before '{' token
```

合法位置只有两处：

```cpp
// ① 文件作用域（顶格）
double helper(double x) { return x * 2; }

class Lambertian : public Material {
public:
    // ② 类作用域（4 空格，和别的成员平级）
    double scattering_pdf(...) const override { ... }

    bool scatter(...) const override {
        auto f = [](double x) { return x * 2; };   // 想在函数里建小函数 → lambda
    }
};
```

> **自查口诀**：在函数体内部看到「8 空格的返回类型 + `override` + `{`」，
> 位置一定错了。**成员函数之间是平级的。**

顺便：`.h` 里的内联成员函数，编译器通常不会把错误定位到「嵌套」这件事上，
只报 `a function-definition is not allowed here`，容易误以为是括号不配对。
**看到这句话先往上找最近的 `{` 属于谁。**

### 2. 虚函数可以有默认实现 —— 用来表达「这一类没有这个概念」

```cpp
class Material {
public:
    virtual bool scatter(...) const = 0;                    // 纯虚：必须实现
    virtual double scattering_pdf(...) const { return 0.0; } // 默认：大多数不用管
};
```

设计意图很重要：这里 `0` **不是**「还没实现」，而是一个有物理含义的返回值 ——

```
δ 分布（镜面 / 玻璃）只有一个散射方向
→ 该方向的概率密度是无穷大，其余方向是 0
→ 不存在有限的 pdf
```

于是积分器约定：`pdf == 0` 时 `cosθ/pdf = 1`。

```cpp
if (pdf > 0.0) throughput = attenuation * (cos_theta / pdf);
else           throughput = attenuation;      // δ 分布
```

> **经验**：给虚函数写默认实现时，问自己「这个默认值是不是一个**合法的语义**」。
> 是 → 好设计；只是想偷懒 → 改成纯虚。
> 这里 `0.0` 是合法的（表示无有限密度），所以合适。

### 3. 把「常数被约掉」的结果还原成公式

Day 19 的代码：

```cpp
attenuation = albedo;      // 余弦采样
attenuation = operator*(2, albedo) * cos_t;   // 均匀半球（另一个分支）
```

前者看起来像「反射率」，其实是：

```
(albedo/π) · cosθ / (cosθ/π)  =  albedo
```

cosθ 和 π 是被**采样策略配平**掉的。今天拆开成：

```cpp
attenuation = albedo / pi;                 // 只是 BRDF，与采样无关
scattering_pdf(...) { return cos_t / pi; } // pdf 单独一个函数
// 积分器统一做 attenuation * cosθ / pdf
```

**判断一个量该不该拆开的标准：它是否依赖别的东西。**
`attenuation` 依赖采样方式，说明采样方式的信息混进来了。

### 4. 恒等重构的验收标准：输出逐字节相同

把 `attenuation` 从「含 pdf 补偿」改成「纯 BRDF」，同时让积分器补上补偿。
数学上恒等，实际呢？

**必须用字节级对比，不能用「看起来一样」。**

```powershell
--cornell --spp 64                 → C61F1512...
--cube-tris --spp 64 --no-nee      → 32258571...
--cube-tris --spp 64 --uniform-hs  → 5C0D9F0C...
```

三个场景覆盖三条路径，MD5 全部不变。

**如果结果差了几个像素而不是整体偏差，最可能的原因：**

```
消耗随机数的顺序变了
  → 每条光线的随机序列错位
  → 画面结构正确但每个像素的噪声不同
```

**如果整体亮度变了**，那是公式错，不是随机数错。**两种症状的判别价值很高。**

### 5. 数值断言能反推出错误位置

学员把分支 A 写成了：

```cpp
attenuation = (2.0 / (2.0 * pi)) * cos_t * albedo;   // ✗
```

`2.0/(2.0*pi)` 就是 `1/π`，所以这一行等于 `(albedo/π)·cosθ` —— 
即 **f·cosθ 忘了除以 pdf**（pdf 已经在 `1/π` 里被"用"过一次了，但方式错了）。

U8 实测：

```
期望 0.190986  (= albedo.r/π)
实际 0.150058
比值 0.150058 / 0.190986 = 0.7857
```

**比值的判读表：**

| 比值 | 说明 |
|---|---|
| **1.0** | 对 |
| **0.3183 (= 1/π)** | 漏了常数因子 `1/π` |
| **3.1416 (= π)** | 多乘了一个 `π` |
| **随运行变化、且等于某个 cosθ** | 多/少乘了一个**随方向变化的**因子（cosθ）|

实测比值 0.7857 落在最后一行（θ≈38.2°），立刻定位到「多乘了一个 cosθ」。
**一个数字就能区分「常数错」和「方向相关项错」，比肉眼看图快得多。**

> 这也是为什么测试里要用 `albedo(0.6, 0.4, 0.2)` 这种**可手算**的值，
> 而不是随机数 —— 期望值要能当场算出来。
>
> ⏰ 注意：涉及随机采样的断言只有在**固定随机序列**时才可复现，
> 否则同一个 bug 每次跑出来的数字都不一样，判读表就用不上了。

### 6. 求积公式：半球上的 `dω = sinθ dθ dφ`

数值验证 `∫ pdf dω = 1` 时，必须有 `sinθ`：

```cpp
// 中点法则，θ ∈ [0, π/2]，φ ∈ [0, 2π]
sum += pdf(θ, φ) * std::sin(theta) * dtheta * dphi;
```

`sinθ` 来自球坐标的雅可比行列式 —— **立体角不是 θ-φ 平面上的均匀面积**，
越靠近极点（θ→0）同样的 dθ 对应的立体角越小。

丢了 `sinθ` 会积出 `2`（对 `cosθ/π` 而言），
`∫pdf dω` 变成 2 —— 一个概率密度积分出 2，说明没归一化，测试立刻抓到。

---

## Day 30 · `const` 不是只读 / 默认参数 / `std::filesystem` / PS 脚本编码

### 1. 内置类型的 `const` + 立即初始化 = **编译期常量**

```cpp
const int w = 400;      // 编译期常量
w = 800;                // ✗ error: assignment of read-only variable

int w = 400;            // ✓ 变成普通变量，运行时可以覆盖
w = 800;
```

`const` 在这里有两层含义，容易混：

| 说法 | 含义 |
|---|---|
| "只读" | 初始化之后不能再赋值 |
| "编译期常量" | 值在编译时就确定，可以用于 `case` 标签、数组大小 |

对 `int` / `double` 这类内置类型，**用字面量初始化的 `const` 两者兼具**。
所以"想让它可配置"必须先把 `const` 去掉。

> 对比：类类型（比如 `const std::string s = "x";`）一般只是"只读"，不是编译期常量。

### 2. 默认参数：给老调用点留后路

```cpp
Camera(Point3 lookfrom, Point3 lookat, Vec3 vup,
       double vfov, double focus_dist, double lens_radius,
       double aspect_ratio = 16.0 / 9.0);      // ← 新增，带默认值
```

这样**8 个已有的 `Camera(...)` 调用点一个字都不用改**，需要时再传第 7 个参数。

**规则：**
- 默认参数只能从**最右边**开始连续给
- 默认值写在**声明**里，不要同时又写在定义里（否则重定义错误）
- 虚函数的默认参数**按静态类型取**（和虚函数本身的动态绑定相反）—— 这是个坑，本项目没用到但要知道

### 3. `std::filesystem::create_directories` 不抛异常的版本

```cpp
std::error_code ec;
std::filesystem::create_directories(dir, ec);   // ✓ 不抛，用 ec 查
if (ec) { /* 失败 */ }

std::filesystem::create_directories(dir);       // 会抛 filesystem_error
```

**为什么用 `ec` 版本：** 目录已存在不算错（返回 false），但**权限不足会抛异常**。
在 `main()` 里让异常飞出去等于直接崩，而 `ofstream` 失败本来是有干净处理路径的。
**能被正常处理的失败，就不要用异常。**

> 顺带：`ofstream` **不会**自动建目录。
> 写 `output/frames/frame_0000.ppm` 之前，`output/frames` 必须已经存在。

### 4. `snprintf` 生成编号文件名

```cpp
char name[64];
std::snprintf(name, sizeof(name), "frame_%04d.ppm", f);   // frame_0007.ppm
```

`%04d` = 至少 4 位、不足补 0。**视频帧文件名必须是定宽的**，
否则 `frame_9.ppm` 和 `frame_10.ppm` 排序会乱，ffmpeg 的 `frame_%04d` 也对不上。

> C++20 有 `std::format`，但 MinGW 14 的实现还不完整，这里用 C 的 `snprintf` 更稳。

### 5. `std::chrono::steady_clock` 测单帧耗时

```cpp
const auto t0 = std::chrono::steady_clock::now();
// ... 干活 ...
const auto t1 = std::chrono::steady_clock::now();
const double sec = std::chrono::duration<double>(t1 - t0).count();
```

**为什么用 `steady_clock` 而不是 `system_clock`：**
`system_clock` 可以被用户改时间、被 NTP 校准 —— 会得到负数或跳变。
`steady_clock` 保证**单调不减**，专门用来测时间间隔。

`duration<double>` 的 `count()` 单位是**秒**（因为是 `duration<double>` 默认 ratio 为 1）。

### 6. 又一个 PowerShell 5.1 编码坑（第三次，形态不同）

写了个带中文提示的 `.ps1`，运行直接解析失败：

```
字符串缺少终止符: '。
ParserError: TerminatorExpectedAtEndOfString
```

**根因链：**

```
.ps1 存成【无 BOM 的 UTF-8】
  → PowerShell 5.1 按 GBK 解码
  → GBK 是双字节：汉字 = 首字节 + 尾字节
  → 某个汉字的尾字节恰好是 0x27（'）
  → 那个 ' 被当成字符串结束符 → 引号被"吃"掉 → 语法错
```

前两次（Day 19、Day 28）是**写文件时**编码错；
这一次是**读脚本时**解码错。**同一台机器，同一个编码问题，第三种表现。**

**对策（本项目已定为规矩）：`tools/*.ps1` 里的输出文字一律用 ASCII。**
脚本内部逻辑、注释可以用中文，但**别让中文字符进入字符串字面量**。

> 更普遍的一条：**PowerShell 5.1 会在 GBK 与 UTF-8 之间猜**。
> 任何"中文 + 引号 / 中文 + 变量名"的组合都可能坏。
> 能用 ASCII 就用 ASCII，不能用就用 `[System.IO.File]::WriteAllText` 显式指定编码。
