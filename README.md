# cpp-renderer

从零实现的 C++17 光线追踪渲染器。

目标：完整跑通 **离线光追 → 实时渲染 → 高级特性 → 优化与工具化** 四个阶段，
把每一步的原理推导和踩坑记录写进 [`docs/devlog.md`](docs/devlog.md)。

---

## 效果展示

### Day 11 · 复杂度实测：BVH 的优势随规模放大

![Day 11 性能曲线](gallery/day11-scaling-curve.svg)

> 把场景规模从 95 个球扫到 8,095 个球，每档渲染 3 次取最小值。
> 双对数坐标下，**直线的斜率就是复杂度指数**：
>
> | | 实测斜率 | 含义 |
> |---|---|---|
> | 朴素遍历 | **0.98** | ≈ 1 → 线性 O(N) |
> | BVH 求交 | **0.17** | 远低于 1 → 对数 O(log N) |
> | BVH 建树 | 1.16 | 超线性 O(N log²N) |
>
> **N 涨 85.2 倍：朴素涨 78 倍，BVH 只涨 2.13 倍**
> （纯对数理论值 log₂ 12.98/6.57 = 1.98，**实测偏差 +7.6%**）。
> 加速比单调上升：**2.5× → 6.9× → 24.3× → 93.4×**。
>
> ⚠️ **建树线最终与求交线相交**（N=8095 时 25.5 ms vs 24.7 ms）：
> 建树是一次性成本，但它超线性增长。所以——
> **只渲染一帧**时该比「建树+求交」（仍快 46×）；
> **渲染动画多帧**时建树只付一次，该比「求交」（快 93×）。

`--grid 22` 渲染的 44×44 球阵（1,936 球）：

![Day 11 大场景](gallery/day11-big-scene.png)

### Day 10 · 扁平化 + 一个函数名的教训（13.16 s → 1.82 s）

![Day 8-10 性能历程](gallery/day10-performance-journey.svg)

> 把 `shared_ptr` 串起来的树重写成【连续数组 + 整数下标】，节点从 88 字节压到 64 字节
> （正好一条缓存线），求交改成显式栈迭代。
>
> **但扁平化只快了 6%。** 看汇编才发现真凶：
> `std::fmax` / `std::fmin` 因为特殊的 NaN 语义，被编译成**函数调用**而不是
> `maxsd` / `minsd` 指令 —— 一次盒子测试 6 次调用，合计约 **19 亿次**。
> 换成三目运算符后：**5.55 s → 1.82 s**。
>
> **验收**：单元测试 23/23；`image.png` **172,127 B（与 Day 8/9 字节级完全一致）**；
> 累计加速 **7.2×**。

### Day 9 · BVH 加速结构（画面字节级不变，只变快）

![Day 9 BVH 性能](gallery/day09-bvh-performance.svg)

> 把 481 个物体递归地分成两堆、两堆再分两堆，形成二叉树。
> 求交时先试大盒子，没中就跳过整个子树。
>
> **验收**：单元测试 23/23；`image.png` **172,127 B（与 Day 8 字节级完全一致）**；
> 渲染 **13.16 s → 5.89 s（2.23×）**。
>
> ⚠️ **有意思的地方**：球体求交次数从 481 降到 **4.71**（少 102 倍），但总耗时只快了 2.23 倍。
> 原因是**每次测试的成本涨了 5.5 倍**——朴素遍历的 480 个球在内存里是连续的（全在缓存里），
> 而 BVH 每走一个节点都要指针跳转 + 虚函数调用 + 分支预测失败（≈ 75 个 CPU 周期）。
> 详见 [devlog Day 9「原理 4」](docs/devlog.md)。

### Day 8 · AABB 包围盒（基础设施，画面不变）

![Day 8 AABB 概念图](gallery/day08-aabb-concept.svg)

> 加速结构的第一步：给每个物体算一个"外接轴对齐长方体"。
> 光线先试盒子（便宜、无开方），没中就直接排除整个物体，中了才做精确求交。
>
> **验收**：单元测试 23/23 通过；世界包围盒输出与手工验算一致；
> 渲染结果与 Day 7 **字节级完全一致**（172,127 B）—— 加速结构的第一原则是"结果不变，只是变快"。

### Day 7 · 程序化随机场景（480 个球）

![Day 7 随机场景](gallery/day07-random-scene.png)

> 400×225 · 32 spp · 纯渲染 13.0 秒
>
> 金属球反射出整个世界、玻璃球折射出内部透出的小球阵列、
> 476 个小球用 80% 哑光 / 15% 金属 / 5% 玻璃的概率随机生成。

### Day 6 · 可移动相机（景深）

![Day 6 可移动相机](gallery/day06-movable-camera.png)

> 相机参数：`lookfrom(13,2,3)` · `lookat(0,0,0)` · `vfov 20°` · `focus_dist 13.7` · `lens_radius 0.1`

<details>
<summary>🔍 Day 6 对照实验：故意对错焦（focus_dist = 5）</summary>

![Day 6 对焦实验](gallery/day06-focus-experiment.png)

球在 13.9 米外、焦平面在 5 米处 → 球全部糊掉，但**构图与上图完全一致**。
这验证了「视口尺寸与距离同倍放大 → FOV 不变，只有焦平面移动」。

</details>

---

## 进度

| 天 | 内容 | 状态 |
|:--:|---|:--:|
| 1 | CMake 骨架、`vec3` / `ray` / `camera`、PPM 输出、渐变背景 | ✅ |
| 2 | 球体求交、法线着色、`Hittable` 抽象、多物体场景 | ✅ |
| 3 | 抗锯齿（三层循环 + 抖动采样）、Lambertian 漫反射（递归弹射） | ✅ |
| 4 | 材质系统：`Lambertian` / `Metal` / `Dielectric`、递归深度限制 | ✅ |
| 5 | 景深：薄透镜模型、`random_in_unit_disk`、弥散圆公式 | ✅ |
| 6 | 可移动相机：基向量 `u/v/w`、`vfov`、`focus_dist` | ✅ |
| 7 | 程序化随机场景（480 个球） + 性能测量方法论 | ✅ |
| 8 | **AABB 包围盒**（slab method + 单元测试体系） | ✅ |
| 9 | **BVH 递归建树**（结果字节级不变，2.23× 加速） | ✅ |
| 10 | **扁平化 BVH + 干掉 `std::fmax` 函数调用**（累计 **7.2×**） | ✅ |
| 11 | **复杂度实测**：双对数性能曲线（斜率 0.98 vs 0.17）+ 可重置 RNG | ✅ |
| — | 后续：Schlick 菲涅尔、gamma 校正、多线程、实时渲染、降噪 | 📋 |

---

## 性能数据（实测）

### 复杂度随规模的变化（Day 11）

`120×68 · 4 spp · 每档 3 次取最小 · 不含写文件`

| N | 建树 ms | 朴素 ms | BVH 求交 ms | 加速比 |
|---:|---:|---:|---:|---:|
| 95 | 0.089 | 29.4 | 11.6 | 2.5× |
| 480 | 0.668 | 141.0 | 20.4 | 6.9× |
| 1,931 | 4.016 | 559.6 | 23.0 | 24.3× |
| 8,095 | 25.509 | 2,308.0 | 24.7 | **93.4×** |

复现：`\build\raytracer.exe --bench` → `output/bench.csv` → `\tools\plot_bench.ps1`

### 单场景优化历程

| 场景 | 物体数 | 平均弹射层数 | 求交次数 | 纯渲染耗时 |
|---|---|---|---|---|
| `simple_scene` | 4 | 1.63 | 1,874 万 | **0.28 s** |
| `random_scene` | 480 | 2.75 | ≡37.96 亿（朴素） | **13.03 s** → **1.82 s**（BVH） |

求交吞吐量 ≈ **2.9 亿次 / 秒**（单线程）
> ⚠️ 讨论渲染性能请**直接计时 `raytracer.exe`**。
> `tools/run.ps1` 报的"总耗时"包含 CMake 编译（≈ 2.6 s），会严重干扰判断 —— 详见
> [devlog Day 7「原理 3」](docs/devlog.md)。

---

## 目录结构

```
cpp-renderer/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/          # 头文件（全部基于接口/组合设计）
│   ├── rtweekend.h        工具箱：公共头、常量、随机数
│   ├── vec3.h             三维向量（身兼位置/方向/颜色）
│   ├── ray.h              光线 P(t) = O + tD
│   ├── aabb.h             轴对齐包围盒 + slab method 求交
│   ├── bvh.h              BVH 树（连续数组 + 整数下标，无虚函数遍历）
│   ├── camera.h           可移动相机：像素坐标 -> 光线
│   ├── hittable.h         HitRecord + Hittable 抽象接口（hit / bounding_box）
│   ├── sphere.h           球体求交 + 包围盒
│   ├── hittable_list.h    物体列表（组合模式，将来换 BVH 只需改这里）
│   ├── material.h         Material 抽象接口
│   ├── lambertian.h       哑光
│   ├── metal.h            金属（镜面 + fuzz）
│   └── dielectric.h       玻璃 / 水 / 钻石（折射 + 全反射）
├── src/
│   └── main.cpp            ray_color + 场景构建 + render() + --bench 基准测试
├── tests/
│   └── aabb_test.cpp       AABB 单元测试（独立可执行，1 秒反馈）
├── tools/
│   ├── run.ps1            一键：编译 -> 渲染 -> 转 PNG（分阶段报耗时）
│   ├── test.ps1           一键：编译 + 运行所有单元测试
│   ├── plot_bench.ps1     读 bench.csv -> 生成双对数性能曲线 SVG
│   ├── publish.ps1        一键：效果图归档 -> commit -> push
│   └── ppm2png.ps1        PPM(P3) -> PNG 转换
├── docs/
│   ├── devlog.md          技术日志（逐日原理推导 + 踩坑）
│   └── api.md             函数总览（速查手册）
├── gallery/          # 各阶段效果图（入库存档）
└── output/           # 运行时产物（不入库）
```

---

## 构建与运行

```bash
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/raytracer.exe                 # Windows (MinGW)
./build/raytracer.exe --grid 22       # 改球阵规模（half=22 → 44×44，约 1900 球）
./build/raytracer.exe --bench         # 扫规模跑基准 -> output/bench.csv
```

一键脚本（推荐）：

```powershell
.\tools\test.ps1                             # 单元测试（1 秒反馈）
.\tools\run.ps1                              # 编译 + 渲染 + 转 PNG
.\tools\plot_bench.ps1                       # bench.csv -> 性能曲线图
.\tools\publish.ps1 -Message "Day 11" -Image day11.png   # 发布到 GitHub
```

输出：`output/image.ppm` → `output/image.png`

---

## 设计要点

- **组合模式**：`HittableList` 本身也是一个 `Hittable`，所以"一堆物体"和"一个物体"可以同样对待。
  后来把线性遍历换成 `BVH` 时，**`main.cpp` 一行都没改** —— 设计伏笔直接兜现。
- **不变量优先**：`BVH::bounding_box()` 直接返回建树时算好的盒子（缓存）。
  代价是树必须**不可变**——否则盒子会对不上内容，导致 BVH “错得看不出来”。
- **可重复性优先**：`reset_rng()` 让随机序列可重置。
  **跨进程**渲染能字节一致（默认种子 5489）；**同进程内**连续渲染必须显式重置。
  这是基准测试能“只换算法、不换场景”的前提。
- **接口分离**：`Hittable` 回答"撞到了吗"，`Material` 回答"往哪弹、打几折"。
  加新材质（比如 `Isotropic`）时，`ray_color` 和求交代码完全不用动。
- **输出参数**：`scatter(r_in, rec, attenuation, scattered)` 用引用传出两个结果，
  配合 `bool` 返回"是否散射"。
- **可空输出用指针**：`render(..., std::ostream* out)` —— `nullptr` 表示“只算不写”。
  引用做不到这件事，因为**引用不能为空**。
- **递归深度限制**：玻璃的全反射会让递归无限深入（曾造成栈溢出 `0xC00000FD`），
  用 `max_depth` 作保险丝。

---

## 技术日志

完整推导（球体求交、Snell 定律、薄透镜成像、弥散圆、相机基向量…）见
[`docs/devlog.md`](docs/devlog.md)。函数速查见 [`docs/api.md`](docs/api.md)。
