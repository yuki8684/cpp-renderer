# cpp-renderer

从零实现的 C++ 光线追踪渲染器（离线光追 → 实时渲染 → 高级特性 → 优化与工具化）。

## 目录结构

```
cpp-renderer/
├── CMakeLists.txt
├── .gitignore
├── include/
│   ├── vec3.h        # 三维向量：位置 / 方向 / 颜色
│   ├── ray.h         # 光线 P(t) = O + tD
│   └── camera.h      # 像素 (u,v) -> 光线
├── src/
│   └── main.cpp      # 主循环，输出 PPM
├── docs/
│   └── devlog.md     # 技术日志
└── output/           # 运行时生成的图片
```

## 构建与运行

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Windows（MSVC / Visual Studio 生成器）：

```powershell
.\build\Release\raytracer.exe
```

其他平台：

```bash
./build/raytracer
```

输出文件：`output/image.ppm`

## 查看 PPM

- VS Code 安装 PPM Viewer 插件
- 或转换：`magick output/image.ppm output/image.png`

## 当前进度

- [x] 阶段一 · 第 1 周：CMake/Git 骨架、vec3 / ray / camera、PPM 输出
- [ ] 第 2 周：球体求交 + 法线着色
