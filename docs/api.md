# cpp-renderer · 函数总览

> **用途**：写着写着忘了"这个函数叫什么、在哪、干什么"时，查这里。
> **更新**：2026-10-02（Day 3 结束）

---

## 0. 文件地图

| 文件 | 里面有什么 | 一句话 |
|---|---|---|
| `include/rtweekend.h` | 公共头、常量、随机数 | 所有文件的"工具箱" |
| `include/vec3.h` | `Vec3` 类 + 数学运算 | 三维向量，身兼位置/方向/颜色 |
| `include/ray.h` | `Ray` 类 | 一条光线：起点 + 方向 |
| `include/camera.h` | `Camera` 类 | 把像素坐标变成光线 |
| `include/hittable.h` | `HitRecord` + `Hittable` | 命中报告单 + "能被撞的东西"接口 |
| `include/sphere.h` | `Sphere` 类 | 球体：自己会算求交 |
| `include/hittable_list.h` | `HittableList` 类 | 一堆物体的集合 |
| `src/main.cpp` | `ray_color()` + `main()` | 主流程 |
| `tools/run.ps1` | 一键脚本 | 编译 → 渲染 → 转 PNG |

---

## 1. 调用关系图（看懂这张图就不乱了）

```
main()
 │
 ├─ Camera()                    构造相机：算视口的左下角、横边、竖边
 │
 ├─ HittableList::add(...)      把球装进"世界"
 │     └─ Sphere(c, r)          造一个球
 │
 └─ 三层循环（行 j / 列 i / 采样 s）
      │
      ├─ Camera::get_ray(u, v)  →  得到一条光线 Ray
      │
      └─ ray_color(ray, world)  ★★★ 核心函数
           │
           ├─ HittableList::hit()     问世界：撞到东西了吗？
           │    └─ Sphere::hit()      挨个问每个球（多态自动派发）
           │         ├─ dot()            向量点积
           │         └─ r.at(t)          算出交点位置
           │
           ├─ random_unit_vector()    取一个随机方向
           │    ├─ random_double(min,max)
           │    └─ unit_vector()
           │
           └─ ray_color(scattered)  ★ 递归调用自己！
                                     直到光线射向天空 → 返回背景色
```

**记住三个"入口"就够了：**

1. `main()` —— 程序从这儿开始
2. `ray_color()` —— 所有"颜色"都从这儿出
3. `Sphere::hit()` —— 所有"求交"都从这儿出

---

## 2. `rtweekend.h` —— 工具箱

### `random_double()` → `double`
返回 `[0, 1)` 之间的随机小数。

```cpp
inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}
```
> `static` 变量只初始化一次，之后每次调用复用同一个生成器。
> 如果每次都新建，随机性会退化（重复播种）。

### `random_double(min, max)` → `double`
返回 `[min, max)` 之间的随机小数。**这是上面那个的"重载版"**（同名、参数不同）。

```cpp
return min + (max - min) * random_double();
```

### 常量

| 名字 | 值 | 用途 |
|---|---|---|
| `infinity` | 无穷大 | 当作"还没找到任何交点"的 t_max 初值 |
| `pi` | 3.14159... | 备用 |

### `degrees_to_radians(deg)` → `double`
角度转弧度。目前还没用到（做可调相机时才会用）。

---

## 3. `vec3.h` —— 向量

### 3.1 `Vec3` 类

| 成员 | 说明 |
|---|---|
| `Vec3()` | 默认构造，得到 (0, 0, 0) |
| `Vec3(x, y, z)` | 带参构造 |
| `.x()` `.y()` `.z()` | 读三个分量 |
| `v[i]` | 下标访问，`v[0]` 就是 x |
| `-v` | 取负 |
| `v += u` / `v *= t` / `v /= t` | 就地修改（返回自己，可链式） |
| `.length()` | 向量长度（要开方，慢） |
| `.length_squared()` | 长度平方（**不用开方，快**）→ 能比较大小就优先用这个 |

**两个语义别名**（数据结构一样，含义不同，帮你看代码时理解意图）：

```cpp
using Point3 = Vec3;   // 表示"位置"
using Color  = Vec3;   // 表示"颜色"
```

### 3.2 成员运算符（写在类里的）

| 运算符 | 用法 | 含义 |
|---|---|---|
| `+=` | `a += b` | a = a + b |
| `*=` | `a *= 2.0` | a = a × 2 |
| `/=` | `a /= 2.0` | a = a ÷ 2 |
| `-`（一元） | `-a` | 三个分量取负 |
| `[]` | `a[0]` | 下标访问 |

### 3.3 非成员函数（自由函数）

| 函数 | 返回 | 干什么 |
|---|---|---|
| `operator+(u, v)` | Vec3 | 逐元素相加 |
| `operator-(u, v)` | Vec3 | 逐元素相减 |
| `operator*(u, v)` | Vec3 | **逐元素**相乘（不是点积！） |
| `operator*(t, v)` / `(v, t)` | Vec3 | 标量乘 |
| `operator/(v, t)` | Vec3 | 标量除 |
| `operator<<(out, v)` | ostream | 打印成 `x y z` |

### 3.4 几何运算

| 函数 | 返回 | 含义 | 用在哪 |
|---|---|---|---|
| `dot(u, v)` | **double** | 点积 u·v，衡量"方向有多一致" | 球体求交、后面算光照 |
| `cross(u, v)` | **Vec3** | 叉积，得到一个同时垂直于 u 和 v 的向量 | 后面搭相机坐标系 |
| `unit_vector(v)` | Vec3 | **归一化**：长度变 1，只留方向 | 到处都在用 |
| `random_unit_vector()` | Vec3 | 在单位球面上随机取一点（长度 1，方向随机） | 漫反射弹射方向 |

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

## 5. `camera.h` —— 相机

| 成员 | 返回 | 含义 |
|---|---|---|
| `Camera()` | — | 构造时算好视口的左下角、横边、竖边 |
| `.get_ray(u, v)` | Ray | 给定归一化像素坐标 (u,v) ∈ [0,1]²，返回穿过该点的光线 |

**相机内部存了 4 个量：**

| 成员 | 含义 |
|---|---|
| `origin` | 相机位置（本项目固定在原点） |
| `horizontal` | 视口从左到右的边（向量） |
| `vertical` | 视口从下到上的边（向量） |
| `lower_left_corner` | 视口左下角（点） |

**核心公式**（`get_ray` 里那一行）：

```
视口上的点 P = lower_left_corner + u × horizontal + v × vertical
光线方向     = P - origin
```

> 💡 在当前代码里，`origin` 其实**完全不影响画面** ——
> 因为在 `get_ray` 里它被减掉了。这就是第 4 周要重构相机的原因。

---

## 6. `hittable.h` —— 命中报告单 + 抽象接口

### 6.1 `HitRecord`（结构体）

| 字段 | 类型 | 含义 |
|---|---|---|
| `p` | Point3 | 交点位置 |
| `normal` | Vec3 | 交点法线（**必须是单位向量**） |
| `t` | double | 交点对应的光线参数 |

**它就是一个"送货单"**：`hit()` 函数返回 true 时，必须把这三个字段填满。

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

## 7. `sphere.h` —— 球体

### `Sphere(center, radius)`
构造一个球。

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

## 9. `src/main.cpp`

### `ray_color(r, world)` → Color  ★ 核心函数

```cpp
Color ray_color(const Ray& r, const Hittable& world) {
    HitRecord rec;
    if (world.hit(r, 0.01, infinity, rec)) {                    // 撞到了吗？
        Vec3 target = rec.p + rec.normal + random_unit_vector();
        Ray  scattered(rec.p, target - rec.p);                  // 随手弹射
        return 0.5 * ray_color(scattered, world);               // ★ 递归！
    }
    // 没命中 → 天空渐变
    Vec3 unit_dir = unit_vector(r.direction());
    double t = 0.5 * (unit_dir.y() + 1.0);
    return (1.0 - t) * Color(1.0, 1.0, 1.0)
         +         t  * Color(0.5, 0.7, 1.0);
}
```

**契约**：输入"一条光线 + 一个世界"，输出"一个颜色"，无副作用。

**它做了两件事：**
1. 撞到东西 → **随手弹射一条新光线**，递归算它的颜色，再打 5 折
2. 没撞到 → 返回天空渐变（**这就是递归的终止条件**）

> ⚠️ **`t_min = 0.01` 不能填 0** —— 弹射光线的起点就在表面上，
> 浮点误差会让它立刻"撞到自己"，产生 shadow acne（满屏斑点）。

### `main()`

按顺序做这些事：

| 步骤 | 代码 |
|---|---|
| 1. 设置控制台编码（Windows） | `SetConsoleOutputCP(CP_UTF8)` |
| 2. 设分辨率、采样数 | `image_width` / `image_height` / `samples_per_pixel` |
| 3. 打开输出文件 | `std::ofstream out("output/image.ppm")` |
| 4. 写 PPM 头 | `out << "P3\n" << W << ' ' << H << "\n255\n"` |
| 5. 建世界、加球 | `HittableList world; world.add(...)` |
| 6. 建相机 | `Camera camera;` |
| 7. **三层循环渲染** | 行 j → 列 i → 采样 s，累积后取平均 |
| 8. 关闭文件、报完成 | `return 0` |

**三层循环的层级：**

| 层 | 变量 | 职责 |
|---|---|---|
| 第 1 层 | `j` | 逐行（从下往上遍历，因为 PPM 从上往下写） |
| 第 2 层 | `i` | 逐列 |
| 第 3 层 | `s` | 采样（**计数器必须叫 s，不能叫 i/j**，否则会遮蔽外层） |

---

## 10. 容易混淆的几组名字

| 容易混 | 区别 |
|---|---|
| `unit_vector(v)` vs `random_unit_vector()` | 前者"把你的向量变单位长"，后者"凭空造一个随机单位向量" |
| `dot(u,v)` vs `cross(u,v)` | `dot` 返回 **double**（一个数）；`cross` 返回 **Vec3**（一个向量） |
| `length()` vs `length_squared()` | 后者不开方，**只比较大小就用后者** |
| `Hittable::hit` vs `Sphere::hit` vs `HittableList::hit` | 同一个接口的三种实现：抽象、球、列表 |
| `Ray::at(t)` vs `Camera::get_ray(u,v)` | `at` 是"光线上的点"；`get_ray` 是"造一条光线" |
| `HitRecord::t` vs 循环里的 `t` | `rec.t` 是"光线参数"；循环里那个 `t` 是"渐变插值系数"，只是重名 |
| `Point3` / `Vec3` / `Color` | **同一个类**，只是语义别名，提示"这个变量是什么意思" |

---

## 11. 一帧的完整数据流（串起来看）

```
①  main 里：像素 (i, j) + 随机抖动
         ↓
②  Camera::get_ray(u, v)  →  Ray
         ↓
③  ray_color(ray, world)
         ↓
④  HittableList::hit(ray, 0.01, ∞, rec)
         ↓ 遍历
⑤  Sphere::hit(...)  →  解二次方程  →  填 rec
         ↓
⑥  命中？ 否 → 返回天空渐变色（结束）
         ↓ 是
⑦  随手弹射：target = rec.p + rec.normal + random_unit_vector()
             scattered = Ray(rec.p, target - rec.p)
         ↓
⑧  回到 ③（递归），拿到颜色后 × 0.5
         ↓
⑨  回到 main，累加到 pixel_color
         ↓
⑩  采样 s 循环 8 次后 ÷ 8 → 得到最终颜色
         ↓
⑪  写进 PPM 文件
```

---

## 12. 接下来会新增什么（预告）

Day 4 会引入 **`Material` 抽象基类**，它和 `Hittable` 是**同一套多态手法**：

```
Material（抽象）
 ├─ Lambertian   哑光（现在的 0.5 硬编码会变成 albedo 参数）
 ├─ Metal        金属（反射 + 模糊）
 └─ Dielectric   玻璃（折射 + 全反射）
```

`HitRecord` 会增加一个字段：`shared_ptr<Material> mat;`
`Sphere` 的构造函数会增加一个参数：材质。

到时候 `ray_color` 里的 `0.5 * ray_color(...)` 会变成：

```cpp
Color attenuation;   // 反射率（而不是写死的 0.5）
Ray   scattered;     // 弹射光线（由材质自己决定方向）
if (rec.mat->scatter(r, rec, attenuation, scattered))
    return attenuation * ray_color(scattered, world);
return Color(0, 0, 0);
```

> **"弹射方向怎么算"和"打几折"这两件事，从 `ray_color` 搬到材质里去。**
> 这就是"材质"这个概念的本质。
