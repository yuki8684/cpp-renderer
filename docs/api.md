# cpp-renderer · 函数总览（详解版）

> **用途**：写着写着忘了"这个函数叫什么、在哪、干什么"时，查这里。
> **更新**：2026-10-09（Day 8 结束 · AABB 包围盒 + 单元测试体系）
>
> **怎么用：**
> 1. 先看 §0 文件地图 —— 知道东西大致在哪
> 2. 再看 §1 调用关系图 —— 知道谁调谁
> 3. 具体函数用 `Ctrl+F` 直接搜名字 —— 每个函数都给出了：
>    **作用 / 参数 / 返回 / 为什么要这样设计 / 陷阱**
>
> **符号约定（贯穿全文）：**
> - `参数`（反引号单名）= 代码里的标识符
> - `⭐` = 核心函数，必须理解
> - `⚠️` = 容易写错的坑

---

## 0. 文件地图

| 文件 | 里面有什么 | 一句话 |
|---|---|---|
| `include/rtweekend.h` | 公共头、常量、随机数 | 所有文件的"工具箱" |
| `include/vec3.h` | `Vec3` 类 + 数学运算 | 三维向量，身兼位置/方向/颜色 |
| `include/ray.h` | `Ray` 类 | 一条光线：起点 + 方向 |
| `include/aabb.h` | `AABB` 类 | 轴对齐包围盒 + slab method 求交（BVH 的地基） |
| `include/camera.h` | `Camera` 类 | 可移动相机：把像素坐标变成光线 |
| `include/hittable.h` | `HitRecord` + `Hittable` | 命中报告单 + "能被撞的东西"接口 |
| `include/sphere.h` | `Sphere` 类 | 球体：自己会算求交 |
| `include/hittable_list.h` | `HittableList` 类 | 一堆物体的集合 |
| `include/material.h` | `Material` 抽象类 | "往哪弹、打几折"的统一接口 |
| `include/lambertian.h` | `Lambertian` 类 | 哑光（石膏、纸、墙） |
| `include/metal.h` | `Metal` 类 | 金属（镜面 + fuzz 粗糙度） |
| `include/dielectric.h` | `Dielectric` 类 | 玻璃 / 水 / 钻石 |
| `src/main.cpp` | `ray_color()` + `simple_scene()` + `random_scene()` + `main()` | 主流程 + 两个场景 |
| `tests/aabb_test.cpp` | 独立单元测试 | 23 条 AABB 求交测试（1 秒反馈，不用等渲染） |
| `tools/run.ps1` | 一键脚本 | 编译 → 渲染 → 转 PNG（分阶段报耗时） |
| `tools/test.ps1` | 一键脚本 | 编译 + 运行 `tests/` 下所有单元测试 |
| `tools/publish.ps1` | 一键脚本 | 效果图归档 → git commit → git push |
| `tools/ppm2png.ps1` | 转换脚本 | PPM(P3) → PNG（本机无 ImageMagick） |

---

## 1. 调用关系图（看懂这张图就不乱了）

```
main()
 │
 ├─ random_scene() / simple_scene()     造世界（Day 7 新增）
 │     ├─ make_shared<Sphere>(...)       造球
 │     └─ HittableList::add(...)         装进"世界"
 │
 ├─ HittableList::bounding_box(box)      算整个世界的包围盒（Day 8 新增，验证用）
 │     └─ Sphere::bounding_box(...)      每个球报自己的盒子
 │          └─ AABB::surrounding(...)    把所有盒子并起来
 │
 ├─ Camera(lookfrom, lookat, vup, vfov, focus_dist, lens_radius)
 │                                构造相机：算基向量 u/v/w + 视口左下角/横边/竖边
 │     └─ unit_vector() / cross()   算相机的右、上、后三个方向
 │
 └─ 三层循环（行 j / 列 i / 采样 sample）
      │
      ├─ Camera::get_ray(s, t)  →  得到一条光线 Ray
      │     └─ random_in_unit_disk()   光圈上的随机偏移（景深）
      │
      └─ ray_color(ray, world, depth)  ★★★ 核心函数
           │
           ├─ HittableList::hit()     问世界：撞到东西了吗？
           │    └─ Sphere::hit()      挨个问每个球（多态自动派发）
           │         ├─ dot()              向量点积
           │         ├─ r.at(t)            算出交点位置
           │         └─ set_face_normal()  决定法线朝向 + front_face
           │
           ├─ rec.mat->scatter(...)   ★ 问材质：往哪弹？打几折？
           │    ├─ Lambertian  →  rec.normal + random_unit_vector()
           │    ├─ Metal       →  reflect() + fuzz
           │    └─ Dielectric  →  refract() 或 reflect()（全反射时）
           │
           └─ ray_color(scattered, world, depth−1)  ★ 递归调用自己！
                                     出口：射向天空 或 depth 用完

tools/test.ps1
 └─ tests/aabb_test.cpp
      └─ AABB::hit(r, t_min, t_max)         23 条手工验算的用例
```

**记住三个"入口"就够了：**

1. `main()` —— 程序从这儿开始
2. `ray_color()` —— 所有"颜色"都从这儿出
3. `Sphere::hit()` —— 所有"求交"都从这儿出

> 📌 **包围盒（Day 8）目前还没接入渲染流程** —— 它只是个"零件"，
> 还没被任何渲染代码调用（只在 `main` 里算了世界包围盒打印出来）。
> **真正用它的地方是 Day 9 的 BVH。**

---

## 2. `rtweekend.h` —— 工具箱

> 约定：**其它头文件第一行都 `#include "rtweekend.h"`**，
> 所以它负责把所有常用的标准库都拉进来（`<cmath>` `<iostream>` `<memory>` `<vector>` `<random>` ……），
> 并定义全局常量和工具函数。

---

### `random_double()` → `double`

**作用**：返回 `[0, 1)` 之间的随机小数。

| 项 | 说明 |
|---|---|
| 参数 | 无 |
| 返回 | `double`，范围 `[0, 1)` —— **注意不含 1** |

```cpp
inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}
```

**为什么要用 `static`？**

`static` 局部变量**只在第一次调用时初始化**，之后每次调用复用同一个。
如果去掉 `static`，每次调用都会新建一个随机数生成器 —— 而生成器未播种时是
**确定性**的（每次从同一个序列开头开始），于是每次返回同一个数，随机性完全丧失。

**用什么场合：**
- 抗锯齿的像素内抖动
- 随机选材质（`random_scene` 里的 80/15/5）
- 拒绝采样里取候选点

---

### `random_double(min, max)` → `double`

**作用**：返回 `[min, max)` 之间的随机小数。

**这是上面那个的「重载」**（同名、参数不同），C++ 会根据你传几个参数自动选。

```cpp
return min + (max - min) * random_double();
```

**推导**：`random_double()` 给出 `[0,1)`，乘上区间宽度 `(max−min)`，再加下限 `min` → `[min, max)`

**用在：**
- `random_unit_vector()` 里取 `[-1, 1)` 的坐标
- `random_scene()` 里给金属球取 `[0.5, 1.0]` 的亮色
- `random_scene()` 里取 `[0, 0.5]` 的 fuzz

---

### `degrees_to_radians(deg)` → `double`

**作用**：角度转弧度。

```cpp
return degrees * pi / 180.0;
```

**为什么需要它**：C++ 的三角函数（`std::tan` / `std::sin` / `std::cos`）**全部吃弧度**。
用户输入的 `vfov = 20` 是度，直接 `std::tan(20.0)` 算出来的是 `tan(20 弧度)`，
结果完全错。必须先转。

**用在**：`Camera` 构造函数里把 `vfov` 转成弧度。

---

### 常量

| 名字 | 值 | 作用 |
|---|---|---|
| `infinity` | `std::numeric_limits<double>::infinity()` | 当作"还没找到任何交点"的 `t_max` 初值 |
| `pi` | `3.1415926535897932385` | 角度/弧度转换用 |

**为什么 `infinity` 这么重要**：

`ray_color` 里调用 `world.hit(r, 0.01, infinity, rec)` —— 意思是"在 `t ∈ (0.01, ∞)` 里找交点"。
用 `infinity` 而不是某个大数字，是因为**它真的比任何可能的 t 都大**，不会误杀远处的物体。

而 `HittableList::hit` 内部把这个 `infinity` 当作"当前最近命中距离"的初值，
每找到一个更近的就收紧它 —— 见 §9.2。

---

## 3. `vec3.h` —— 向量

### 3.1 `Vec3` 类

**一句话**：一个类三个 `double`，同时当**位置 / 方向 / 颜色**用。

| 成员 | 说明 |
|---|---|
| `Vec3()` | 默认构造 → **`(0, 0, 0)`**（不是“无”！） |
| `Vec3(x, y, z)` | 带参构造 |
| `.x()` `.y()` `.z()` | 读三个分量 |
| `v[i]` | 下标访问，`v[0]` 就是 x。**循环处理三个轴时用** |
| `-v` | 一元取负 |
| `v += u` / `v *= t` / `v /= t` | 就地修改（返回自己，可链式） |
| `.length()` | 向量长度（**要开方，慢**） |
| `.length_squared()` | 长度平方（**不开方，快**） |

**三个设计决策，都很重要：**

#### ① 为什么位置 / 方向 / 颜色用同一个类

有人担心："万一你手滑把颜色减位置怎么办？" 对，确实防不住。
但书里的选择是：**在“少写代码”和“防手滑”之间选前者**（只要后果不严重）。
代价是：你得自己记得哪个变量是什么。

缓解办法就是下面那两个**语义别名**（编译器不会帮你检查，但**读代码的人会**）。

#### ② 为什么要有 `Point3` / `Color` 别名

```cpp
using Point3 = Vec3;   // 表示"位置"
using Color  = Vec3;   // 表示"颜色"
```

它们**完全是同一个类**（不会类型检查、不会报错、没有性能开销），
存在的唯一目的是**告诉你自己（和读者）这个变量是什么意思**。

```cpp
Point3 lookfrom;    // 看到就会想到“这是个位置”
Vec3   direction;   // 看到就会想到“这是个方向”
```

#### ③ 为什么 `length_squared()` 比 `length()` 重要

开方（`sqrt`）是很贵的指令。而很多时候你**只需要比大小**：

```
|a| < |b|  ⟺  |a|² < |b|²        （因为长度非负，平方是单调的）
```

**两边都是正数时，比平方和比长度完全等价。** 所以能用 `length_squared` 就别用 `length`。

> 本项目里 `random_unit_vector()` 的拒绝采样就是用 `len2 > 1.0` 而不是 `len > 1.0`。

**两个语义别名**（数据结构一样，含义不同，帮你看代码时理解意图）：

```cpp
using Point3 = Vec3;   // 表示"位置"
using Color  = Vec3;   // 表示"颜色"
```

### 3.2 成员运算符（写在类里的）

| 运算符 | 用法 | 含义 | 返回 |
|---|---|---|---|
| `+=` | `a += b` | a = a + b | `Vec3&`（自己） |
| `*=` | `a *= 2.0` | a = a × 2 | `Vec3&` |
| `/=` | `a /= 2.0` | a = a ÷ 2 | `Vec3&` |
| `-`（一元） | `-a` | 三个分量取负 | `Vec3`（新对象） |
| `[]` | `a[0]` | 下标访问（有 const 和非 const 两版） | `double&` 或 `double` |

**为什么 `+=` 返回 `Vec3&`**：返回**自己的引用**，这样才能链式写 `a += b += c`。
这是 C++ 运算符重载的惯例（和内置类型的 `+=` 行为一致）。

**为什么 `[]` 有两个版本**：

```cpp
double  operator[](int i) const { return e[i]; }   // 只读对象 → 只能读
double& operator[](int i)       { return e[i]; }   // 非只读对象 → 可读可写
```

所以 `const Ray& r` 里的 `r.direction()[axis]` 调的是第一个（只读，安全）；
而 `v[0] = 5.0` 调的是第二个（可写）。

### 3.3 非成员函数（自由函数）

| 函数 | 返回 | 干什么 |
|---|---|---|
| `operator+(u, v)` | Vec3 | 逐元素相加 |
| `operator-(u, v)` | Vec3 | 逐元素相减 |
| `operator*(u, v)` | Vec3 | ⚠️ **逐元素**相乘（**不是点积！**） |
| `operator*(t, v)` / `(v, t)` | Vec3 | 标量乘（两个方向都支持，`2*v` 和 `v*2` 都能写） |
| `operator/(v, t)` | Vec3 | 标量除（实现成 `(1/t) * v`，一次除法） |
| `operator<<(out, v)` | ostream | 打印成 `x y z`（调试用） |

**为什么这些是“自由函数”而不是类成员？**

为了让**两边都能交换**。类成员只能写成 `v.operator*(2.0)`，
而自由函数可以写成 `operator*(2.0, v)`，
于是 `2.0 * v` 和 `v * 2.0` 都合法。

> ⚠️ **最常见的绊倒点**：`u * v` 是**逐元素**相乘，不是点积！
>
> ```cpp
> Vec3 a(1,2,3), b(4,5,6);
> a * b                // = (4, 10, 18)   ← 逐元素
> dot(a, b)            // = 32            ← 真正的点积
> ```
>
> 为什么需要逐元素乘法：颜色调制（`attenuation * 光的颜色`）就是逐元素乘。
> `albedo = (0.9, 0.3, 0.3)` 乘上白光，得到红光 —— 这就是“红球”的物理含义。

### 3.4 几何运算（逐个详解）

| 函数 | 返回 | 一句话 |
|---|---|---|
| `dot(u, v)` | **double** | 点积：衡量两个方向"有多一致" |
| `cross(u, v)` | **Vec3** | 叉积：得到一个同时垂直于 u 和 v 的向量 |
| `unit_vector(v)` | Vec3 | 归一化：长度变 1，只留方向 |
| `random_unit_vector()` | Vec3 | 在单位**球面**上随机取一个方向 |
| `random_in_unit_disk()` | Vec3 | 在单位**圆盘**（z=0 平面）内随机取一点 |
| `reflect(v, n)` | Vec3 | 反射：镜面弹回去 |
| `refract(uv, n, eta_ratio)` | Vec3 | 折射：按 Snell 定律穿过去 |

---

#### `dot(u, v)` → `double`

**作用**：点积。返回**一个数**（不是向量）。

```
dot = u.x*v.x + u.y*v.y + u.z*v.z
```

**几何意义**（这是它真正有用的地方）：

```
dot(u, v) = |u| × |v| × cos θ          （θ 是两向量夹角）
```

如果 u、v **都是单位向量**，就退化成：

```
dot(u, v) = cos θ
```

| 值 | 含义 | 用在哪 |
|---|---|---|
| `> 0` | 夹角 < 90°，方向大致相同 | 判断法线是否朝向光线（`set_face_normal`） |
| `= 0` | 垂直 | 相机基向量互相垂直 |
| `< 0` | 夹角 > 90°，方向大致相反 | `reflect` 的推导 |

**用在哪**：球体求交（`half_b = dot(oc, D)`）、判断光线从哪侧来、`reflect`/`refract` 内部。

---

#### `cross(u, v)` → `Vec3`

**作用**：叉积。返回**一个向量**，它同时垂直于 u 和 v。

```
cross((x1,y1,z1), (x2,y2,z2)) =
    ( y1*z2 − z1*y2 ,  z1*x2 − x1*z2 ,  x1*y2 − y1*x2 )
```

**方向怎么记**：**右手定则** —— 右手四指从 u 弯向 v，大拇指指向就是结果方向。

**长度**：

```
|cross(u, v)| = |u| × |v| × sin θ
```

⚠️ **顺序不能反**：`cross(u,v) = −cross(v,u)`。

**用在哪**：相机基向量 `u = cross(vup, w)`、`v = cross(w, u)`（见 §6）。

---

#### `unit_vector(v)` → `Vec3`

**作用**：归一化。把向量变成同方向的**单位向量**（长度 1）。

```cpp
return v / v.length();
```

**为什么要归一化**：很多公式只在单位向量下才成立：
- `dot(u,v) = cos θ` 要求 u、v 都是单位向量
- `reflect(v, n)` 要求 n 是单位向量
- 叉积的长度公式同样

⚠️ **零向量会炸**：`v / 0` → `NaN`，而且不报错。就像 Day 6 里
`vup` 和视线平行时的 `cross` = `(0,0,0)` → 整张图花屏。

---

#### `random_unit_vector()` → `Vec3`

**作用**：**凭空造**一个方向完全随机的单位向量（长度一定是 1）。

**实现：拒绝采样（rejection sampling）**

```cpp
while (true) {
    Vec3 p(random_double(-1,1), random_double(-1,1), random_double(-1,1));
    double len2 = p.length_squared();
    if (len2 > 1.0 || len2 < 1e-8) continue;
    return unit_vector(p);
}
```

| 判断 | 含义 | 处理 |
|---|---|---|
| `len2 > 1.0` | 点在**球外**（立方体的八个角在球外） | 丢掉重取 |
| `len2 < 1e-8` | 点**太靠近原点**，归一化会除以接近 0 的数 | 丢掉重取 |
| 其他 | 在单位球内部 | 归一化后返回 |

**命中率约 52%**（立方体体积 = 8，球体积 ≈ 4.19 → 4.19/8 ≈ 0.524）。
平均大约循环两次就能命中一次。

**用在哪**：`Lambertian::scatter`（漫反射弹射方向）、`Metal::scatter`（fuzz 扰动）。

> ⚠️ **别和 `unit_vector` 搞混**：
> - `unit_vector(v)` —— 把**你给的**向量变单位长（方向不变）
> - `random_unit_vector()` —— **不给任何东西**，凭空造一个随机方向

---

#### `random_in_unit_disk()` → `Vec3`

**作用**：在**单位圆盘**（z = 0 平面、半径 1 的圆内）随机取一点。

```cpp
while (true) {
    Vec3 p(random_double(-1,1), random_double(-1,1), 0);   // ← z 恒为 0
    if (p.length_squared() < 1) return p;
}
```

**和 `random_unit_vector` 的区别：**

| | `random_unit_vector()` | `random_in_unit_disk()` |
|---|---|---|
| 在什么上取点 | 单位**球面** | 单位**圆盘** |
| 维度 | 3D（z 也随机） | 2D（z 恒为 0） |
| 返回向量的长度 | 恒为 1 | 从 0 到 1 之间任意 |
| 用在哪 | 漫反射方向、fuzz | **相机光圈上的随机偏移** |

**为什么相机要用圆盘而不是球**：光圈是一个**平面圆孔**，光线只能从孔内出发。
用球的话就会有一些光线从"镜头前后"出发，物理上不对。

> 📝 **历史遗留**：这个函数原来叫 `random_in_unit_sphere`（错的），Day 5 改了名。
> 教训：**名字必须反映实现**，否则下一个读代码的人（包括三个月后的你）会被坑。

---

#### `reflect(v, n)` → `Vec3`

**作用**：镜面反射方向。

```cpp
return v - 2 * dot(v, n) * n;
```

**推导**：把 v 拆成"切向分量"和"法向分量"，反射时**切向不变、法向翻转**：

```
切向 = v − (v·n)n
法向 = (v·n)n
反射 = 切向 − 法向 = v − (v·n)n − (v·n)n = v − 2(v·n)n
```

⚠️ **约定**：`v` 必须**指向表面内部**（即光线前进的方向），`n` 是单位法线。
这个约定下 `dot(v,n) < 0`。如果 v 指向外面，结果会完全错（往错误方向反射）。

**用在哪**：`Metal::scatter`、`Dielectric::scatter` 的全反射分支。

---

#### `refract(uv, n, eta_ratio)` → `Vec3`

**作用**：折射方向（Snell 定律）。

```cpp
double cos_theta = std::fmin(dot(-uv, n), 1.0);
Vec3 r_out_perp     = eta_ratio * (uv + cos_theta * n);
Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
return r_out_perp + r_out_parallel;
```

**参数**：

| 参数 | 含义 |
|---|---|
| `uv` | 单位入射方向，**指向表面内部** |
| `n` | 单位法线（已经翻到朝向入射光一侧） |
| `eta_ratio` | 入射介质折射率 ÷ 折射介质折射率 |

**公式拆解：**

```
Snell 定律：  sin θ₂ = eta_ratio × sin θ₁
```

把结果拆成垂直于法线和平行于法线两部分：

```
R'_⊥ = eta_ratio × (R + cos θ · n)      ← 长度 = eta_ratio × sin θ₁ = sin θ₂
R'_∥ = −√(1 − |R'_⊥|²) · n                ← 用单位向量性质反推出 cos θ₂
R'   = R'_⊥ + R'_∥
```

`std::fmin(..., 1.0)` 是为了防止浮点误差让 `cos_theta` 略大于 1，
那样 `sqrt(1 − cos²)` 会算出 `√负数` = NaN。

`std::fabs` 同样是为了防御：理论上 `1 − |R'_⊥|²` 应该 ≥ 0，但浮点误差可能让它
变成很小的负数。

⚠️ **调用前必须自己判全反射**：如果 `eta_ratio × sin θ₁ > 1`，
这个函数**不会报错**，而是靠着 `fabs` 安静地算出一个错误的方向。
所以在 `Dielectric::scatter` 里要先判 `cannot_refract`。

**用在哪**：`Dielectric::scatter`。

> ⚠️ **`reflect` / `refract` 的符号约定**：
> `v` / `uv` 是**入射方向**，必须**指向表面内部**（也就是光线前进的方向）。
> 在我们的代码里 `r_in.direction()` 天然满足。**验证：这时 `d·n < 0`。**

> ⚠️ **`unit_vector` 和 `random_unit_vector` 是两回事！**
> - `unit_vector(v)`：把**你给的那个向量**变成单位长度（方向不变）
> - `random_unit_vector()`：**凭空造**一个方向完全随机的单位向量

`random_unit_vector()` 的实现用**拒绝采样**：

```cpp
while (true) {
    Vec3 p(random_double(-1,1), random_double(-1,1), random_double(-1,1));
    double len2 = p.length_squared();
    if (len2 > 1.0 || len2 < 1e-8) continue;   // 球外 / 太靠近原点 → 重取
    return unit_vector(p);
}
```
- `len2 > 1.0`：立方体的八个角在球外，拒绝
- `len2 < 1e-8`：太接近原点，归一化会除以接近 0 的数 → NaN，拒绝
- 命中率约 52%（立方体体积 8，球体积 4.19）

---

## 4. `ray.h` —— 光线

| 成员 | 返回 | 含义 |
|---|---|---|
| `Ray()` | — | 默认构造（几乎不用） |
| `Ray(origin, direction)` | — | 带参构造 |
| `.origin()` | `const Point3&` | 起点 |
| `.direction()` | `const Vec3&` | 方向（**注意：本项目不保证它是单位向量**） |
| `.at(t)` | Point3 | **光线上的点**：P(t) = 起点 + t × 方向 |

**`at(t)` 是整个项目最重要的一个函数** —— 球体求交、计算交点、计算法线全靠它。

| t 的值 | 含义 |
|---|---|
| t = 0 | 在起点（相机位置） |
| t > 0 | 沿方向往前 |
| t < 0 | 在起点背后（通常忽略） |

---

## 4.5 `aabb.h` —— 轴对齐包围盒 ★ Day 8 新增

> **一句话**：用"便宜且保守"的盒子测试，快速排除掉一大批物体。
> 这是 **BVH 加速结构的地基**。

### 4.5.1 它解决什么问题

朴素遍历要挨个问每个物体"你被打到了吗"。480 个球就是 480 次昂贵的球体求交。

包围盒提供一个**极便宜的预筛**：

```
光线没打中盒子  ⟹  一定没打中里面的物体    ← 永远成立（因为盒子“包住”了物体）
光线打中盒子    ⟹  可能打中，也可能只是擦过
```

| 操作 | 要算什么 |
|---|---|
| 球体求交 | 3 次点积 + **1 次开方** + 若干乘加 |
| 盒子求交 | 3 个轴上各 1 减、1 乘、2 比较，**没开方** |

开方是最贵的指令 —— 所以盒子测试便宜好几倍，适合拿来做筛选。

### 4.5.2 两个成员

```cpp
Point3 minimum;   // 三个坐标都取最小的那个角
Point3 maximum;   // 三个坐标都取最大的那个角
```

“轴对齐”= 六个面分别垂直于 x / y / z 轴。
好处：**三个轴可以分开算** —— 求交退化成一维问题（见 4.5.4）。

---

### `AABB(a, b)` —— 用任意两个对角点构造

**作用**：从两个对角点造一个盒子。

```cpp
AABB(const Point3& a, const Point3& b) {
    minimum = Point3(std::fmin(a.x(), b.x()),
                     std::fmin(a.y(), b.y()),
                     std::fmin(a.z(), b.z()));
    maximum = Point3(std::fmax(a.x(), b.x()),
                     std::fmax(a.y(), b.y()),
                     std::fmax(a.z(), b.z()));
}
```

**不必关心 a、b 谁大谁小** —— 逐轴取 min/max 会自动纠正。

**为什么要 `std::fmin` / `std::fmax` 而不是三目运算符？**

C++ 标准**明确规定** `fmin` / `fmax` 是 **NaN 感知**的：
只要有一个参数是 NaN，就返回**另一个**参数。
而 `a < b ? a : b` 依赖“NaN 比较返回 false”这个副作用，可读性和可移植性都差一些。

> 📌 **能用标准函数就用标准函数** —— 语义明确、不依赖读者的推理。

---

### `AABB::surrounding(box0, box1)` → `AABB`（静态）

**作用**：把两个盒子合并成一个"刚好能同时装下两者"的新盒子。

```cpp
static AABB surrounding(const AABB& box0, const AABB& box1) {
    Point3 small(std::fmin(box0.minimum.x(), box1.minimum.x()), ...);
    Point3 big  (std::fmax(box0.maximum.x(), box1.maximum.x()), ...);
    return AABB(small, big);
}
```

**用在哪**：`HittableList::bounding_box`、以及 Day 9 的 BVH 建树
（每个父节点的盒子 = 两个子节点盒子的并集）。

---

### ⭐ `AABB::hit(r, t_min, t_max)` → `bool`

**作用**：光线 `r` 在参数区间 `(t_min, t_max)` 内是否打到这个盒子。

**算法名**：**slab method**（板法）。

#### 第一步：单轴推导

设某条轴上：光线起点 `O`，方向 `D`，盒子范围 `[min, max]`。
要找所有满足下式的 `t`：

```
min ≤ O + t·D ≤ max
```

两边减 `O`：

```
min − O ≤ t·D ≤ max − O
```

**除以 `D`**。⚠️ 除数为负时不等号要翻转，所以分两种情况：

| 情况 | t 的范围 |
|---|---|
| `D > 0` | `[ (min−O)/D , (max−O)/D ]` |
| `D < 0` | `[ (max−O)/D , (min−O)/D ]` ← 颠倒 |

**统一写法**（关键技巧）：

```
t0 = (min − O) / D
t1 = (max − O) / D
如果 D < 0：交换 t0 和 t1
→ 现在一定有 t0 ≤ t1，区间就是 [t0, t1]
```

> 💡 **一次 swap 代替一个 if-else 分支**，这叫"规范化区间"。

#### 第二步：三个轴取交集

光线要**同时**在三个板的范围内：

```
t_进入 = max(t0_x, t0_y, t0_z)    ← 必须晚于"最晚的进入时刻"
t_离开 = min(t1_x, t1_y, t1_z)    ← 必须早于"最早的离开时刻"

命中 ⟺ t_进入 ≤ t_离开
```

#### 完整代码

```cpp
bool hit(const Ray& r, double t_min, double t_max) const {
    for (int axis = 0; axis < 3; ++axis) {
        double inv_d = 1.0 / r.direction()[axis];              // ← 可能除以 0
        double t0 = (min_axis(axis) - r.origin()[axis]) * inv_d;
        double t1 = (max_axis(axis) - r.origin()[axis]) * inv_d;
        if (inv_d < 0.0) std::swap(t0, t1);                    // 规范化区间
        t_min = std::fmax(t0, t_min);                          // 交集下界取大
        t_max = std::fmin(t1, t_max);                          // 交集上界取小
        if (t_min >= t_max) return false;                      // 交集空了
    }
    return true;
}
```

#### ⚠️ 陷阱一：除以零**不用特判**

IEEE 754 浮点数里 `1.0 / 0.0 = +infinity`（**不崩溃、不抛异常**）。而：

| 情况 | `t0` | `t1` | 结果 |
|---|---|---|---|
| 起点在板内 | `−∞` | `+∞` | 该轴**不构成约束** ✓ |
| 起点在板外 | `−∞` | `−∞` | 立即判空 → **返回 false** ✓ |
| 起点恰在边界上 | `0×∞ = NaN` | `NaN` | `fmax/fmin` 把 NaN **弹开** → 退化成"无约束" ✓ |

**结论：不要写 `if (D == 0)` 特判** —— 写了反而更慢且容易写错。

#### ⚠️ 陷阱二：`inv_d < 0` 而不是 `D < 0`

当 `D = -0.0` 时，`-0.0 < 0.0` 是 **false**，但 `1/(-0.0) = −∞ < 0` 是 **true**。
两者都不交换的情况下，`t0 = +∞`、`t1 = −∞`，交集变成空集 → **错误地判为不命中**。

所以用 `inv_d < 0.0` 更稳健。

#### ⚠️ 陷阱三：厚度为 0 的盒子打不中

如果某轴 `min == max`（盒子退化成一个薄片），该轴给出单点区间 `[t, t]`，
而判据是 `t_min >= t_max` → **薄片永远打不中**。

- 球**永远不会**产生这种盒子，所以现在无所谓
- 将来加**矩形 / 三角形**这类平面几何体时必须回来处理
  （RTIOW 的做法是给盒子加极小量 padding）

#### `min_axis(axis)` / `max_axis(axis)` → `double`

**作用**：读取第 `axis` 轴（0=x, 1=y, 2=z）的两个面。

```cpp
double min_axis(int axis) const { return minimum[axis]; }
double max_axis(int axis) const { return maximum[axis]; }
```

**为什么要有这两个小函数**：让 `hit` 里的循环能用统一的 `[axis]` 下标处理三个轴，
而不用写三遍 `minimum.x()` / `minimum.y()` / `minimum.z()`。

---

## 5. `camera.h` —— 可移动相机 ★ Day 6 重写

### `Camera(lookfrom, lookat, vup, vfov, focus_dist, lens_radius)`

六个参数，对应真实相机：

| 参数 | 类型 | 含义 | 现实对应物 |
|---|---|---|---|
| `lookfrom` | Point3 | 相机**站在哪** | 三脚架位置 |
| `lookat` | Point3 | 相机**看向哪** | 镜头指向 |
| `vup` | Vec3 | 世界里的"上"，一般 `(0,1,0)` | 机身顶盖朝向 |
| `vfov` | double | **垂直**视野角（**度**） | 焦距：大=广角，小=长焦 |
| `focus_dist` | double | **对焦距离**：只有这个距离上的物体完全清晰 | 对焦环 |
| `lens_radius` | double | 光圈半径：0 = 针孔（全清晰） | 光圈 |

典型调用：

```cpp
Camera camera(Point3(13, 2, 3), Point3(0, 0, 0), Vec3(0, 1, 0),
              20,        // vfov 20°（长焦）
              13.7,      // 对焦在球上（球在 13.9 米外）
              0.1);      // 光圈半径
```

### `.get_ray(s, t)` → `Ray`

给定视口上的归一化坐标 `(s, t) ∈ [0,1]²`，返回一条光线。

> ⚠️ **参数叫 `s, t`，不叫 `u, v`** —— 因为 `u, v, w` 已经专门表示相机的三个基向量了。

```cpp
Ray get_ray(double s, double t) const {
    Vec3   offset     = lens_radius * random_in_unit_disk();   // 光圈上的随机点
    Point3 ray_origin = origin + offset;

    return Ray(ray_origin,
               lower_left_corner + s * horizontal + t * vertical - ray_origin);
}
```

### 构造函数干了什么（两步）

**第一步：算相机自己的坐标系**

| 基向量 | 公式 | 含义 |
|---|---|---|
| `w` | `unit_vector(lookfrom - lookat)` | 相机**背后**（所以 `-w` = 前方） |
| `u` | `unit_vector(cross(vup, w))` | 相机**右方**（必须归一化） |
| `v` | `cross(w, u)` | 相机**上方**（天然是单位向量，不用归一化） |

⚠️ `vup` 不能和视线方向平行，否则 `cross` = 零向量 → `unit_vector` 除零 → **NaN** → 图变黑/花屏。

**第二步：搭视口**

```
theta = degrees_to_radians(vfov)
h     = tan(theta / 2)                          ← 距离为 1 时的半高

viewport_height = 2 * h * focus_dist           ← 视口放在对焦平面上
viewport_width  = aspect_ratio * viewport_height

horizontal = viewport_width  * u               ← 横边沿相机右方向
vertical   = viewport_height * v               ← 竖边沿相机上方向

lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist * w
```

**关键**：视口尺寸 ∝ `focus_dist`，视口距离 = `focus_dist`，两者同倍放大 →
`tan(FOV/2) = (h×D)/D = h` 不变。所以 **`focus_dist` 只移动焦平面，不改变构图**（相似三角形）。

### 相机内部存的量

| 成员 | 含义 |
|---|---|
| `origin` | 相机位置（= `lookfrom`） |
| `horizontal` | 视口从左到右的边（向量） |
| `vertical` | 视口从下到上的边（向量） |
| `lower_left_corner` | 视口左下角（点） |
| `lens_radius` | 光圈半径 |

**核心公式**（`get_ray` 里那一行）：

```
视口上的点 P = lower_left_corner + s × horizontal + t × vertical
光线起点     = origin + 光圈随机偏移
光线方向     = P - 光线起点
```

> 💡 **和旧版的对比**：旧版 `origin` 恒为 `(0,0,0)`，在 `get_ray` 里被减掉，改它毫无效果；
> 旧版视口永远在 z=−1 平面，横边永远是世界 x 轴 —— 相机没有自己的坐标系。
> Day 6 之后 `origin` 真正生效，视口朝向由 `u, v` 决定，相机可以自由摆放。

### 模糊程度公式（景深）

```
弥散圆半径 = lens_radius * |focus_dist - 物体距离| / 物体距离
```

- `物体距离 = focus_dist` → 0（绝对清晰）
- `lens_radius = 0` → 0（针孔，全清晰）

---

## 6. `hittable.h` —— 命中报告单 + 抽象接口

### 6.1 `HitRecord`（结构体）

| 字段 | 类型 | 含义 |
|---|---|---|
| `p` | Point3 | 交点位置 |
| `normal` | Vec3 | 表面法线（**已经翻好面**，朝向"光线来的那一侧"） |
| `t` | double | 交点对应的光线参数 |
| `front_face` | bool | 光线是从物体**外侧**打来的吗？（`true` = 从外面来） |
| `mat` | `shared_ptr<Material>` | 这个交点属于什么材质 |

**它就是一个"送货单"**：`hit()` 函数返回 true 时，必须把这些字段填满。

#### `HitRecord::set_face_normal(r, outward_normal)` → void

由 `hit()` 调用，干两件事：

1. **决定 `front_face`**：`dot(r.direction(), outward_normal) < 0` → 说明光线朝里走 → `true`
2. **把 `normal` 翻到"朝向光线来的一侧"**：`front_face` 为真取外法线，否则取反向

```cpp
void set_face_normal(const Ray& r, const Vec3& outward_normal) {
    front_face = dot(r.direction(), outward_normal) < 0;
    normal     = front_face ? outward_normal : -outward_normal;
}
```

**为什么需要它？** 因为玻璃球的光线会**从内部打出来**，那时折射率的比值要取倒数
（空气→玻璃是 1/1.5，玻璃→空气是 1.5）。见「材质系统」那节。

**为什么它不影响哑光球和金属球？** 不透明的球，光线永远从外侧打来 →
`front_face = true` → `normal = outward_normal`，和原来的行为一模一样。

### 6.2 `Hittable`（抽象基类）

```cpp
virtual bool hit(const Ray& r, double t_min, double t_max,
                 HitRecord& rec) const = 0;
```

**这是所有"能被光线撞到的东西"的统一接口。**

| 参数 | 含义 |
|---|---|
| `r` | 要测试的光线 |
| `t_min`, `t_max` | **只在开区间 (t_min, t_max) 内找交点** |
| `rec` | 输出参数：命中时往这里填结果 |

| 返回 | 含义 |
|---|---|
| `true` | 撞到了，`rec` 已填满 |
| `false` | 没撞到，**不要动 rec** |

**两个设计点：**

1. 返回 `bool` 而不是 `double` —— 命中与否是"是/否"的问题，
   细节通过引用参数 `rec` 带出来（这是 C++ 实现"多返回值"的标准手法）
2. 带 `t_min` / `t_max` —— 让调用方告诉它"我只关心这个区间内的交点"

**实现 `Hittable` 的三个类：** `Sphere`、`HittableList`（将来还会有平面、三角形、BVH……）

---

### 6.3 `Hittable::bounding_box(output)` → `bool` ★ Day 8 新增

**作用**："能把你包住的最小轴对齐盒子是哪两个对角点？"

```cpp
virtual bool bounding_box(AABB& output) const {
    (void)output;      // 消除"未使用参数"警告
    return false;
}
```

| 返回 | 含义 |
|---|---|
| `true` | 盒子已写进 `output` |
| `false` | 我**算不出**盒子（比如理论上无限大的平面） |

**为什么用 `bool + 引用参数` 而不是直接 `return AABB`**

因为要返回**两个信息**：bool（成不成功）+ 盒子。C++ 函数只能 `return` 一个值，所以
bool 走 `return`，盒子走**引用参数**。

> 📌 **同一个套路已经出现三次了：**
>
> ```cpp
> bool hit(r, t_min, t_max, HitRecord& rec)
> bool scatter(r_in, rec, Color& attenuation, Ray& scattered)
> bool bounding_box(AABB& output)
> ```
>
> **`bool` 管成败，其余结果靠引用参数"捎带"出来。**

**⚠️ 为什么它不是纯虚函数（`= 0`）**

| | 好处 | 代价 |
|---|---|---|
| 给默认实现 | 以后加新几何体不用马上实现它，能编译 | **忘了实现不报错**，但整个世界的包围盒会静默变成"没有"，BVH 直接不工作 |
| 纯虚函数 | 强迫每个子类都实现，漏了就编译失败 | 加新几何体时必须一次性写完 |

学习项目选前者（好改），大型项目常常选后者（防错）。

**`(void)output;` 是什么**：一个空操作，只是**明确告诉编译器"我知道这个参数没用"**，
免得它报 `[-Wunused-parameter]` 警告。

---

## 7. `sphere.h` —— 球体

### `Sphere(center, radius, material)`

```cpp
Sphere(const Point3& c, double r, std::shared_ptr<Material> m)
    : center(c), radius(r), mat(m) {}
```

> ⚠️ 成员声明顺序必须和初始化列表一致（`center → radius → mat`），否则报 `[-Wreorder]`。

命中时填单：

```cpp
rec.t = root;
rec.p = r.at(root);
rec.set_face_normal(r, (rec.p - center) / radius);   // ← 不是直接赋 rec.normal
rec.mat = mat;
```

### `Sphere::hit(r, t_min, t_max, rec)` → bool

把光线方程 P(t) = O + tD 代入球面方程 |P − C|² = r²，得到一元二次方程：

```
a = D·D
half_b = OC·D          （OC = O − C，即"半个 b"）
c = OC·OC − r²
判别式 disc = half_b² − a·c
```

| 结果 | 处理 |
|---|---|
| disc < 0 | 返回 false（光线掠过球外） |
| 较小的根在 (t_min, t_max) 内 | 用它 |
| 否则较大的根在区间内 | 用它（相机在球内时就是这种情况） |
| 两个根都不在区间内 | 返回 false |

命中时填：

```
rec.t      = 选中的根
rec.p      = r.at(rec.t)                 交点位置
rec.normal = (rec.p - center) / radius   球面上 |P-C| = radius，除完就是单位向量
```

> ⚠️ **常见错误**：只写"取较小的根"。
> 相机位于球内部时较小的根是负数，球会被渲染成一个黑洞。

---

### `Sphere::bounding_box(output)` → `bool` ★ Day 8 新增

**作用**：算出球的包围盒。

```cpp
bool bounding_box(AABB& output) const override {
    output = AABB(center - Vec3(radius, radius, radius),
                  center + Vec3(radius, radius, radius));
    return true;
}
```

**几何**：球在 x / y / z 三个方向"一样宽"（都是 `2r`），所以包围盒是一个**立方体**，
而且**球正好内切于它** —— 一点空间都不浪费。

```
        ┌─────────────┐  ← 包围盒（边长 2r）
        │    ___      │
        │   /   \     │  ← 球（半径 r）
        │  │     │    │
        │   \___/     │
        └─────────────┘
```

**返回值永远是 `true`**：球一定有包围盒。

---

## 8. `hittable_list.h` —— 物体列表

### `HittableList::add(object)` → void
把一个物体加进列表。参数是 `shared_ptr<Hittable>`（智能指针，自动管理内存）。

用法：
```cpp
HittableList world;
world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5));
```

### `HittableList::hit(r, t_min, t_max, rec)` → bool
**遍历所有物体，只保留"最近"的那个命中。**

```cpp
double    closest      = t_max;      // 当前已知的最近命中上限
bool      hit_anything = false;
HitRecord temp_rec;                  // 临时报告单：只有命中才提交给 rec

for (const auto& obj : objects) {
    if (obj->hit(r, t_min, closest, temp_rec)) {   // ← 注意传 closest，不是 t_max
        hit_anything = true;
        closest      = temp_rec.t;                 // 收紧上限
        rec          = temp_rec;                   // 提交结果
    }
}
return hit_anything;
```

**三个关键点：**

| 点 | 原因 |
|---|---|
| 传 `closest` 而不是 `t_max` | 让更远的物体被自动排除，既保证遮挡正确又省计算 |
| 必须遍历完，不能提前 return | 有物体在列表里的顺序和远近无关，"第一个"≠"最近" |
| 用 `temp_rec` 而不是直接传 `rec` | 防御性设计：只有真正命中才把结果"提交"给 `rec` |

---

### `HittableList::bounding_box(output)` → `bool` ★ Day 8 新增

**作用**：整堆物体的包围盒 = 所有子物体包围盒的**【并集】**。

```cpp
bool bounding_box(AABB& output) const override {
    if (objects.empty()) return false;

    AABB temp_box;
    bool first_box = true;

    for (const auto& obj : objects) {
        if (!obj->bounding_box(temp_box)) return false;   // ← 多态调用！
        output = first_box ? temp_box : AABB::surrounding(output, temp_box);
        first_box = false;
    }
    return true;
}
```

#### 为什么要两个盒子变量

| 变量 | 从哪来 | 作用 |
|---|---|---|
| `output` | **函数参数**（引用） | 攒到现在的并集，最终结果 |
| `temp_box` | 本函数内声明 | 临时装"当前这个子物体"报上来的盒子 |
| `first_box` | 本函数内声明 | 标记"是不是第一轮" |

**两个盒子必须都有**，因为**并集需要两边**：

```cpp
// ❌ 错误：会把攒好的结果冲掉
obj->bounding_box(output);                  // output 被覆盖 ⚠️
output = AABB::surrounding(output, ???);    // 旧数据已经没了
```

#### 为什么需要 `first_box`

`AABB output;` 调的是默认构造 `AABB() {}` —— **什么都不做**。
但成员 `minimum` / `maximum` 会被默认构造成 `Vec3()` = **`(0,0,0)`**。

所以那个"空盒子"其实是**原点处的一个退化点，不是"无"**！

```
假设世界只有 [10, 12]³ 这一块
错误写法：surrounding([0,0]³, [10,12]³) = [0, 12]³
                                              ↑↑↑ 莫名其妙把原点圈进来了
```

**后果**：BVH 不会算错（盒子依然保守），但盒子变大 → 排除能力变弱 → **性能变差**。

> 📌 **C++ 教训：`Vec3()` 默认构造不是"无"，是"全 0"。**
> 需要表达"空"的时候，必须自己用一个 `bool` 标志。

#### 执行跟踪（三个球 A/B/C）

| 球 | 球心 | 半径 | 盒子 |
|---|---|---|---|
| A | `(0,0,0)` | 1 | `[-1,1]³` |
| B | `(5,0,0)` | 2 | `[3,7]×[-2,2]×[-2,2]` |
| C | `(-3,0,0)` | 1 | `[-4,-2]×[-1,1]×[-1,1]` |

（只看 x 轴，y / z 同理）

| 时刻 | `temp_box` | `first_box` | `output` |
|---|---|---|---|
| 进函数 | 未初始化 | `true` | 未初始化 |
| 第 1 轮 · A | `[-1,1]` | `true` | **`[-1,1]`** ← 直接赋值 |
| | | `false` | |
| 第 2 轮 · B | `[3,7]` | `false` | **`[-1,7]`** ← 求并集 |
| 第 3 轮 · C | `[-4,-2]` | `false` | **`[-4,7]`** ← 求并集 |
| 返回 | | | `[-4,7]` ✓ |

#### ⚠️ 为什么遇到任何一个子物体返回 `false` 就整体返回 `false`

因为那样算出来的盒子**不保守** —— 漏装了那个物体，而 BVH 会拿这个盒子去做
"这里肯定没东西"的判断。万一那根光线正好从那块区域过，物体就被**错误地漏掉**，
画面会出现莫名其妙的空洞或穿透。

> 📌 **保守性一旦被破坏，BVH 就是用"错得看不出来"的方式出错。**
> 宁可返回 `false`（放弃加速），也不能返回一个漏装的盒子。

#### “第一个子物体没有包围盒”也算失败

上面的代码里，第一轮就 `if (!obj->bounding_box(temp_box)) return false;`，
并在任何情况下都把 `first_box = false`。所以只要有一个子物体不行，整体就不行。

---

## 8.5 材质系统（Material / Lambertian / Metal / Dielectric）

### `Material`（抽象基类，在 `material.h`）

```cpp
class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(const Ray& r_in, const HitRecord& rec,
                         Color& attenuation, Ray& scattered) const = 0;
};
```

一句话：**"光线打在你身上，会往哪儿弹？回来的时候打几折？"**

| 参数 | 方向 | 含义 |
|---|---|---|
| `r_in` | 输入 | 射进来的光线 |
| `rec` | 输入 | 命中信息（交点、法线、front_face） |
| `attenuation` | **输出** | 反射率（打几折，是个颜色） |
| `scattered` | **输出** | 弹射出去的光线 |

返回 `bool`：有没有发生散射（`false` = 光被吸收了）。
**规律**：`const T&` 是输入，`T&` 是输出。

**为什么 `attenuation` 是 `Color` 不是 `double`？**
因为"物体的颜色"在物理上就是"它对各色光的反射率"。
`albedo = (0.9, 0.3, 0.3)` 意思是"红光反射 90%、绿蓝只反射 30%" → 看起来是**红球**。

**⚠️ 能量守恒**：每个通道必须 ≤ 1。否则每次弹射都在放大颜色，
递归下去指数爆炸（第 50 次弹射就是 ×9100）→ 画面全白，而且违背物理。

**循环包含的解法**：`material.h` 里只能写 `struct HitRecord;`（**前向声明**），
**不能** `#include "hittable.h"` —— 因为 `hittable.h` 反过来要 include `material.h`。
引用参数只需要"知道有这个名字"，不需要完整定义。

### `Lambertian(albedo)`（哑光，在 `lambertian.h`）

```cpp
Vec3 direction = rec.normal + random_unit_vector();   // 随机方向（保证在外半球）
scattered      = Ray(rec.p, direction);
attenuation    = albedo;
return true;
```

- 弹射方向**和入射方向无关**（所以 `r_in` 用不上 → 编译器会警告 unused，是正常的）
- `+ rec.normal` 保证方向落在表面**外侧**（否则光线会射进球里）

| albedo | 效果 |
|---|---|
| (1, 1, 1) | 白（自然界不存在） |
| (0.5, 0.5, 0.5) | 灰 |
| (0.9, 0.3, 0.3) | 红 |
| (0.8, 0.8, 0.0) | 芥末黄 |

### `Metal(albedo, fuzz)`（金属，在 `metal.h`）

```cpp
Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
scattered      = Ray(rec.p, reflected + fuzz * random_unit_vector());
attenuation    = albedo;
return dot(scattered.direction(), rec.normal) > 0;
```

| `fuzz` | 效果 | 现实对应 |
|---|---|---|
| 0.0 | 完美镜面 | 抛光铬、镀银镜 |
| 0.1 | 轻微模糊 | 不锈钢 |
| 0.4 | 明显磨砂 | 喷砂铝 |

- 构造函数里 `fuzz(fuzz < 1 ? fuzz : 1)` 把 fuzz **夹到 ≤ 1**（防御性编程）
- **最后那个 `dot(...) > 0` 不能省**：fuzz 可能把方向推到表面**下面**，
  那种光线物理上不存在，返回 `false` 表示吸收（`ray_color` 会返回黑色）
- 反射前要 `unit_vector` —— 为了让 `fuzz` 的**相对扰动量**一致（公式本身不要求归一化）

### `Dielectric(refraction_index)`（玻璃 / 水 / 钻石，在 `dielectric.h`）

```cpp
attenuation = Color(1.0, 1.0, 1.0);              // 玻璃不吸光

double eta_ratio = rec.front_face ? (1.0 / refraction_index) : refraction_index;

double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);
bool cannot_refract = eta_ratio * sin_theta > 1.0;          // 全反射判据

Vec3 direction = cannot_refract
               ? reflect(unit_direction, rec.normal)        // 出不去 → 反射
               : refract(unit_direction, rec.normal, eta_ratio);   // 能出去 → 折射
```

| 折射率 η | 介质 |
|---|---|
| 1.0 | 空气 |
| 1.33 | 水 |
| 1.5 | 玻璃 |
| 2.42 | 钻石 |

**核心公式**：Snell 定律 `sin θ₂ = eta_ratio × sin θ₁`。
`sin θ₂ > 1` 在数学上不存在 → 物理上"出不去" → **全反射**（光纤原理）。

**为什么需要 `front_face`**：空气→玻璃时 `eta_ratio = 1/1.5`，玻璃→空气时 `eta_ratio = 1.5`，
**互为倒数** —— 程序必须知道这次是从哪一侧打来的。

**玻璃球的临界角**：`sin θ_c = 1/1.5` → `θ_c ≈ 41.8°`。
玻璃内部入射角超过这个值的光线**全部被全反射**。

### 加一个新材质要动哪里？

**只需要新建一个头文件**，继承 `Material` 实现 `scatter()`，
然后 `main.cpp` 里 include 它、在场景里 `make_shared` 它。

**`ray_color` 和 `Hittable` 一个字都不用改。** ← 这就是抽象的价值

---

## 9. `src/main.cpp`

### `simple_scene()` → `HittableList` ★ Day 7 新增

**作用**：Day 6 的四个物体场景（玻璃球 / 红哑光球 / 银金属球 / 黄绿地面）。

**为什么保留它**：当作**回归测试的基准**。改动渲染器后切回这个场景，
画面应该与归档的 `gallery/day06-movable-camera.png` **字节级一致** —— 
一旦不一致，就知道是改坏了（而不是"看起来差不多"）。

> 📌 这叫**回归测试（regression test）**：留一份"已知正确的旧结果"，
> 改代码后能快速判断"是我改坏了，还是本来就长这样"。

---

### `random_scene()` → `HittableList` ★ Day 7 新增

**作用**：程序化生成一个 480 个物体的场景。

**结构**：

| 层 | 内容 | 数量 |
|---|---|---|
| 1 | 地面：半径 1000 的巨球，球心 `(0,−1000,0)` → 球顶落在 `y=0` | 1 |
| 2 | 三颗"主角"球（都在 `z=0` 这条线上） | 3 |
| 3 | 22×22 格随机小球阵 | ~476 |

**关键数字的来历：**

| 数字 | 怎么来的 |
|---|---|
| 地面半径 **1000** | `h ≈ d²/(2R)`：球阵跨度 d≈15，R=1000 → 边缘比平面低 0.11，勉强可接受 |
| 抖动 **0.9** | 让球在格子内随机游走，但**不越过格界**（0.9 < 1.0）→ 位置随机且不重叠 |
| 避让半径 **0.9** | 大球（y=1, r=1）在 y=0.2 处的水平截面半径 = `√(1−0.8²)` = **0.6**；加小球半径 0.2 → 0.8；留余量取 0.9 |
| 概率 **80/15/5** | 现实里漫反射表面最多，金属次之，玻璃最少（而且折射计算最贵） |

**哑光颜色为什么用两个随机颜色相乘：**

```cpp
Color albedo = Color(random_double(), random_double(), random_double())
             * Color(random_double(), random_double(), random_double());
```

单个随机颜色三个通道**互不相关** → 出来是荧光色（纯红、亮黄…），一眼假。
相乘后：期望从 0.5 降到 **0.25**（整体变暗），通道间差异被压缩 → 
灰、褐、暗红、土黄、橄榄绿 —— **像真材料**。

> 📌 这是个通用技巧：**"乘两次" = 让颜色偏向暗部、降低饱和度。**

**返回值**：按值返回 `HittableList`。
看起来像"拷贝一整个列表"，其实不会 ——
C++17 的**拷贝省略 / RVO** 会直接在调用方的内存里构造它。
就算真拷贝，里面存的是 `shared_ptr`，成本只是**引用计数 +1**。

---

### `ray_color(r, world, depth)` → Color  ★ 核心函数

```cpp
Color ray_color(const Ray& r, const Hittable& world, int depth) {
    // ★ 保险丝：弹射次数用完就放弃（否则玻璃的全反射会让递归无限深入 → 栈溢出）
    if (depth <= 0) return Color(0, 0, 0);

    HitRecord rec;
    if (world.hit(r, 0.01, infinity, rec)) {          // 撞到了吗？
        Ray   scattered;
        Color attenuation;

        if (rec.mat->scatter(r, rec, attenuation, scattered))     // ★ 问材质
            return attenuation * ray_color(scattered, world, depth - 1);   // ★ 递归

        return Color(0, 0, 0);                        // 材质把光吸收了
    }

    // 没命中 → 天空渐变
    Vec3 unit_dir = unit_vector(r.direction());
    double t = 0.5 * (unit_dir.y() + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0)
         +         t  * Color(0.5, 0.7, 1.0);
}
```

**契约**：输入"一条光线 + 一个世界 + 剩余弹射次数"，输出"一个颜色"，无副作用。

**它做了两件事：**
1. 撞到东西 → **问材质**往哪弹、打几折 → 递归算弹射光的颜色 → 相乘
2. 没撞到 → 返回天空渐变（递归的**正常**出口）

**两个必须记住的点：**

> ⚠️ **`t_min = 0.01` 不能填 0** —— 弹射光线的起点就在表面上，
> 浮点误差会让它立刻"撞到自己"，产生 shadow acne（满屏斑点）。

> ⚠️ **`depth` 保险丝不能省** —— 颜色的衰减**不会**终止递归！
> `return attenuation * ray_color(...)` 是先算完递归再乘衰减，层数一层不少。
> 没有 `depth`，玻璃的全反射会让递归无限深入 → 栈溢出崩溃（`0xC00000FD`）。

`main` 里：`const int max_depth = 50;` → `ray_color(r, world, max_depth)`

### `main()`

按顺序做这些事：

| 步骤 | 代码 |
|---|---|
| 1. 设置控制台编码（Windows） | `SetConsoleOutputCP(CP_UTF8)` |
| 2. 设分辨率、采样数 | `image_width` / `image_height` / `samples_per_pixel` |
| 3. 打开输出文件 | `std::ofstream out("output/image.ppm")` |
| 4. 写 PPM 头 | `out << "P3\n" << W << ' ' << H << "\n255\n"` |
| 5. 建世界 | `HittableList world = random_scene();`（或 `simple_scene()`） |
| 5.5 打印世界包围盒 | `world.bounding_box(world_box)` → 输出两个角点（验证用） |
| 6. 建相机 | `Camera camera(lookfrom, lookat, vup, vfov, focus_dist, lens_radius);` |
| 7. **三层循环渲染** | 行 j → 列 i → 采样 sample，累积后取平均 |
| 8. 关闭文件、报完成 | `return 0` |

**三层循环的层级：**

| 层 | 变量 | 职责 |
|---|---|---|
| 第 1 层 | `j` | 逐行（从下往上遍历，因为 PPM 从上往下写） |
| 第 2 层 | `i` | 逐列 |
| 第 3 层 | `sample` | 采样计数（**叫 `sample` 不叫 `s`** —— 里面还要用 `s` 当像素横坐标） |

**第 3 层里面**（每个采样）：

```cpp
double s = (i + random_double()) / image_width;    // 像素横坐标 + 抖动
double t = (j + random_double()) / image_height;   // 像素纵坐标 + 抖动
Ray r = camera.get_ray(s, t);
pixel_color += ray_color(r, world, max_depth);
```

---

## 10. 容易混淆的几组名字

| 容易混 | 区别 |
|---|---|
| `unit_vector(v)` vs `random_unit_vector()` | 前者"把你的向量变单位长"，后者"凭空造一个随机单位向量" |
| `dot(u,v)` vs `cross(u,v)` | `dot` 返回 **double**（一个数）；`cross` 返回 **Vec3**（一个向量） |
| `length()` vs `length_squared()` | 后者不开方，**只比较大小就用后者** |
| `Hittable::hit` vs `Sphere::hit` vs `HittableList::hit` | 同一个接口的三种实现：抽象、球、列表 |
| `Material::scatter` vs `Sphere::hit` | 两个**不同**的接口：`hit` 回答"撞到了吗"，`scatter` 回答"往哪弹" |
| `reflect(v,n)` vs `refract(uv,n,eta)` | 反射（滑回去）；折射（穿过去）。**两者都要求入射方向指向表面内部** |
| `rec.t` vs `rec.mat` vs `rec.front_face` | 光线参数；材质；从哪一侧打来的 |
| `Ray::at(t)` vs `Camera::get_ray(s,t)` | `at` 是"光线上的点"；`get_ray` 是"造一条光线" |
| `rec.t` vs `get_ray` 的 `s,t` | `rec.t` 是"光线参数"（沿光线走了多远）；`s,t` 是"视口上的归一化坐标"（0~1）。**只是重名，毫无关系** |
| 相机基向量 `u,v,w` vs `get_ray(s,t)` | `u,v,w` 是相机的右/上/后三个方向；`s,t` 是视口坐标。**刻意用不同名字区分**（见 Day 6 变量遮蔽） |
| `vup` vs `v` | `vup` 是你**传进来**的"世界上方向"；`v` 是相机**算出来**的上方向（一般不等于 vup） |
| `albedo` vs `attenuation` | 前者是材质**固有**的反射率（成员变量）；后者是**这一次**散射填报的反射率（输出参数）。目前两者相等，将来支持纹理时就会分开 |
| `depth` vs `max_depth` | `max_depth` 是 `main` 里的初值（50）；`depth` 是递归过程中**每层减 1** 的计数 |
| `Point3` / `Vec3` / `Color` | **同一个类**，只是语义别名，提示"这个变量是什么意思" |
| `minimum` / `maximum` vs `minimum[0]` | 前者是 `Point3`（一个点）；后者是 `double`（那个点的 x） |
| `AABB::hit` vs `Hittable::hit` | 盒子求交**只要区间**；`Hittable::hit` 还要填 `HitRecord`（交点、法线、材质） |
| `output` vs `temp_box` | 都是 `AABB`：`output` 是"攒到现在的并集"，`temp_box` 是"新来的一个"（见 §9.3） |

---

## 11. 一帧的完整数据流（串起来看）

```
①  main 里：像素 (i, j) + 随机抖动
         ↓
②  Camera::get_ray(s, t)  →  Ray（起点 = 相机 + 光圈随机偏移）
         ↓
③  ray_color(ray, world, depth)     ← depth ≤ 0 则直接返回黑色（保险丝）
         ↓
④  HittableList::hit(ray, 0.01, ∞, rec)
         ↓ 遍历
⑤  Sphere::hit(...)  →  解二次方程  →  填 rec
             （含 set_face_normal 决定法线朝向、rec.mat = 材质）
         ↓
⑥  命中？ 否 → 返回天空渐变色（结束）
         ↓ 是
⑦  问材质：rec.mat->scatter(r, rec, attenuation, scattered)
             ├ Lambertian  → 随机方向
             ├ Metal       → 反射 + fuzz
             └ Dielectric  → 折射 / 全反射
         ↓
⑧  回到 ③（递归，depth − 1），拿到颜色后 × attenuation
         ↓
⑨  回到 main，累加到 pixel_color
         ↓
⑩  采样 s 循环 8 次后 ÷ 8 → 得到最终颜色
         ↓
⑪  写进 PPM 文件
```

---

## 12. 材质系统设计回顾（为什么抽象出 Material）

Day 4 引入了 **`Material` 抽象基类**，它和 `Hittable` 是**同一套多态手法**：

```
Material（抽象）
 ├─ Lambertian   哑光（现在的 0.5 硬编码会变成 albedo 参数）
 ├─ Metal        金属（反射 + 模糊）
 └─ Dielectric   玻璃（折射 + 全反射）
```

`HitRecord` 会增加一个字段：`shared_ptr<Material> mat;`
`Sphere` 的构造函数会增加一个参数：材质。

`ray_color` 里的 `0.5 * ray_color(...)` 已经变成了：

```cpp
Ray   scattered;
Color attenuation;
if (rec.mat->scatter(r, rec, attenuation, scattered))
    return attenuation * ray_color(scattered, world, depth - 1);
return Color(0, 0, 0);
```

> **"弹射方向怎么算"和"打几折"这两件事，从 `ray_color` 搬到了材质里。**
> 这就是"材质"这个概念的本质。
>
> **回报**：加 Metal 和 Dielectric 时，`ray_color` 和 `Hittable` **一个字都没改**。

---

## 13. 单元测试体系（★ Day 8 新增）

```
tests/aabb_test.cpp     测试代码（有自己的 main）
tools/test.ps1          一键：编译 tests/*_test.cpp → 运行
```

**跑法：**

```powershell
.\tools\test.ps1
```

**为什么值得单独建一套：**

| 理由 | 说明 |
|---|---|
| 反馈快 | 编译 1 个文件 + 运行 = **1 秒**；而渲染要 13 秒 |
| 隔离 | 测的是算法本身，和渲染流程无关，不该混在 `main.cpp` 里 |
| 可提前验证 | 其他代码还没写完、整个项目编不过时，就能先把 `aabb.h` 验完 |
| 可回归 | 以后改 `aabb.h`（比如加 padding 支持薄片），立刻知道有没有改坏 |

**测试文件的写法（极简）：**

```cpp
static int g_pass = 0, g_fail = 0;

static void check(const char* name, bool got, bool want) {
    if (got == want) { ++g_pass; std::cout << "  [PASS] " << name << '\n'; }
    else { ++g_fail; std::cout << "  [FAIL] " << name << '\n'; }
}

int main() {
    AABB box(Point3(-1,-1,-1), Point3(1,1,1));
    check("正前方命中", box.hit(Ray(Point3(0,0,-5), Vec3(0,0,1)), 0.001, infinity), true);
    // ...
    return g_fail == 0 ? 0 : 1;   // ← 返回非 0 告诉脚本"测试挂了"
}
```

> ⚠️ **测试条目必须是"手工算过答案"的**，否则测试本身不可信。
> 目前 23 条全部人手验算：基本情形 5、斜射 2、除零专项 5、t 区间 4、
> 退化盒子 1、并集 4、构造纠序 2。

---

## 14. C++ 语法速查（本项目用到的）

### `const T&` vs `T&` —— 一眼分辨输入输出

| 写法 | 含义 |
|---|---|
| `const T& x` | **输入**：只读，不会改你传进来的东西 |
| `T& x` | **输出**：函数会往里面写东西 |
| `T x` | 拷贝一份（基本类型常用，如 `double`） |

### `override` 关键字

写在子类函数后面，表示"我要覆盖基类的虚函数"。
**写错了（比如参数类型不对）编译器会报错** —— 不加的话编译器会以为你写了个新函数，
然后多态静默失效。

### `virtual` 和 `= 0`

```cpp
virtual bool hit(...) const = 0;    // 纯虚：必须子类实现，基类不能实例化
virtual bool bounding_box(...) const { ... }   // 虚函数带默认实现：子类可写可不写
```

### `shared_ptr` / `make_shared`

```cpp
std::make_shared<Sphere>(center, radius, material)   // 建一个带引用计数的对象
```

- 指向它的 `shared_ptr` 数量归零时，对象**自动销毁** —— 永远不用写 `delete`
- 优先用 `make_shared` 而不是 `new`：一次分配、更快、更安全（不会中途泄漏）

### lambda（匿名函数）

```cpp
auto check = [&](const char* name, bool got, bool want) { ... };
//           ↑ ↑ 捕获列表：& 表示"按引用访问外面的变量"
//             ↑ 参数列表
```

用在：需要临时传一个"小函数"给别的函数时（比如 `std::sort` 的比较函数）。

### 三目运算符 `?:`

```cpp
output = first_box ? temp_box : AABB::surrounding(output, temp_box);
//                  ↑ 条件   ↑ 真   ↑ 假
```

"用一行 if-else 产生一个值"的简写。条件为真取左边，为假取右边。

### `static` 局部变量

只在**第一次**调用时初始化，之后保留上次的值。
用在 `random_double` 里：保证随机数生成器不会被反复重建。

### `-Wall -Wextra`

GCC 的警告开关。测试脚本里已经开了。
**把警告当错误看**是个好习惯 —— 很多真 bug 在变成错误之前，先是一行警告。

---

## 15. 已知待补 / 已知局限

| 项 | 说明 |
|---|---|
| ⚠️ **`Dielectric` 缺菲涅尔反射** | RTIOW 完整版里是 `cannot_refract \|\| reflectance(cos_theta, ri) > random_double()`。<br>缺了它，玻璃表面没有反光（真实玻璃垂直看约反射 4%，掠射接近 100%） |
| `Lambertian` 的 `r_in` 未使用 | 编译器警告 `[-Wunused-parameter]`，**这是正常的**（哑光弹射方向确实与入射无关） |
| 厚度为 0 的 `AABB` 打不中 | 见 §4.5 陷阱三。加矩形/三角形时必须处理 |
| 无 gamma 校正 | 目前颜色是线性空间直写，画面比真实偏暗（RTIOW 第 9.5 节有讲） |
| 单线程 | 4 核可以快 3~4 倍 |

---

## 16. 接下来的路线（按计划）

| 阶段 | 内容 | 完成情况 |
|---|---|---|
| 第 1~4 周 | 骨架 → 求交 → 抗锯齿 → 材质 → 景深 → 可移动相机 → 随机场景 | ✅ |
| **第 2 月 · 周 1** | **AABB 包围盒 ✅ → BVH 递归建树 → 接入 → 性能对比** | 🔄 **进行中** |
| 第 2 月 · 周 2 | 纹理映射、程序化纹理（棋盘格、噪声）、gamma 校正 | ⏳ |
| 第 2 月 · 周 3 | `std::thread` 多线程分块渲染 | ⏳ |
| 第 2 月 · 周 4 | 性能对比、优化笔记 | ⏳ |

### 当前性能实测（Day 8）

| 场景 | 物体数 | 平均弹射层数 | 求交次数 | 纯渲染耗时 |
|---|---|---|---|---|
| `simple_scene` | 4 | 1.63 | 1,874 万 | **0.28 s** |
| `random_scene` | 480 | 2.75 | **37.96 亿** | **13.03 s** |

求交吞吐量 ≈ **2.9 亿次 / 秒**（单线程）

**BVH 目标**：把每层弹射需要测试的物体数从 `O(N)=480` 降到 `O(log N)≈9`
→ `random_scene` 从 **13 s 降到约 0.3 s**。

### 包围盒暴露出的重要事实

`random_scene` 的"内容"其实只有 **22 × 2 × 22**，世界包围盒却是 **2000 × 2002 × 2000** ——
**99.9% 是空的**（被地面巨球撑大）。

> 这就是 **BVH 存在的第二个理由**（第一个是"减少测试次数"）：
> 一个大盒子没用，**必须递归地切小**。
> 地面这种"撑破天"的物体会被分到自己的叶子节点里，
> 而"球阵"那一堆会被框进一个紧凑得多的小盒子里。
