# cpp-renderer · 函数总览

> **用途**：写着写着忘了"这个函数叫什么、在哪、干什么"时，查这里。
> **更新**：2026-10-07（Day 6 结束 · 可移动相机）

---

## 0. 文件地图

| 文件 | 里面有什么 | 一句话 |
|---|---|---|
| `include/rtweekend.h` | 公共头、常量、随机数 | 所有文件的"工具箱" |
| `include/vec3.h` | `Vec3` 类 + 数学运算 | 三维向量，身兼位置/方向/颜色 |
| `include/ray.h` | `Ray` 类 | 一条光线：起点 + 方向 |
| `include/camera.h` | `Camera` 类 | 可移动相机：把像素坐标变成光线 |
| `include/hittable.h` | `HitRecord` + `Hittable` | 命中报告单 + "能被撞的东西"接口 |
| `include/sphere.h` | `Sphere` 类 | 球体：自己会算求交 |
| `include/hittable_list.h` | `HittableList` 类 | 一堆物体的集合 |
| `include/material.h` | `Material` 抽象类 | "往哪弹、打几折"的统一接口 |
| `include/lambertian.h` | `Lambertian` 类 | 哑光（石膏、纸、墙） |
| `include/metal.h` | `Metal` 类 | 金属（镜面 + fuzz 粗糙度） |
| `include/dielectric.h` | `Dielectric` 类 | 玻璃 / 水 / 钻石 |
| `src/main.cpp` | `ray_color()` + `main()` | 主流程 |
| `tools/run.ps1` | 一键脚本 | 编译 → 渲染 → 转 PNG |

---

## 1. 调用关系图（看懂这张图就不乱了）

```
main()
 │
 ├─ Camera(lookfrom, lookat, vup, vfov, focus_dist, lens_radius)
 │                                构造相机：算基向量 u/v/w + 视口左下角/横边/竖边
 │     └─ unit_vector() / cross()   算相机的右、上、后三个方向
 │
 ├─ HittableList::add(...)      把球装进"世界"
 │     └─ Sphere(c, r)          造一个球
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
| `dot(u, v)` | **double** | 点积 u·v，衡量"方向有多一致" | 球体求交、折射、法线朝向判断 |
| `cross(u, v)` | **Vec3** | 叉积，得到一个同时垂直于 u 和 v 的向量 | 后面搭相机坐标系 |
| `unit_vector(v)` | Vec3 | **归一化**：长度变 1，只留方向 | 到处都在用 |
| `random_unit_vector()` | Vec3 | 在单位球面上随机取一点（长度 1，方向随机） | 漫反射弹射方向、Metal 的 fuzz |
| `reflect(v, n)` | Vec3 | **反射**：v − 2(v·n)n | Metal、Dielectric 全反射 |
| `refract(uv, n, eta_ratio)` | Vec3 | **折射**：按 Snell 定律计算 | Dielectric 透射 |

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
`front_face = true` → `normal = outward_normal`，和原来的行为一模一样
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
| 5. 建世界、加球 | `HittableList world; world.add(...)` |
| 6. 建相机 | `Camera camera;` |
| 7. **三层循环渲染** | 行 j → 列 i → 采样 s，累积后取平均 |
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

## 12. 材质系统（**Day 4 已实现** ✅）

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

## 13. 接下来的路线（按计划）

| 阶段 | 内容 | 完成情况 |
|---|---|---|
| **第 4 周** | 景深 + **可移动相机** + 随机场景生成 → 一张有景深的多材质场景图 | 🔄 差"随机场景" |
| **第 2 月 · 周 1** | AABB 包围盒 + **BVH 加速结构** | ⏳ |
| 第 2 月 · 周 2 | 纹理映射、程序化纹理（棋盘格、噪声） | ⏳ |
| 第 2 月 · 周 3 | `std::thread` 多线程分块渲染 | ⏳ |
| 第 2 月 · 周 4 | 性能对比、优化笔记 | ⏳ |

**⚠️ BVH 是必须的**：现在只有 4 个物体就 3.0 秒。
第 4 周要"随机生成"几百个球的场景 —— 不学 BVH 根本渲染不完。

**相机已经改完** ✅（Day 6）：
`lookfrom / lookat / vup / vfov / focus_dist / lens_radius` 六参数齐全，
可以摆在任意位置、看向任意方向、自由调焦和调光圈。
