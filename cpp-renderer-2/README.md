# cpp-renderer

## 项目简介
`cpp-renderer` 是一个简单的光线追踪渲染器，旨在帮助用户理解光线追踪的基本原理。该项目实现了基本的光线与物体的交点检测，并支持简单的场景渲染。

## 文件结构
```
cpp-renderer
├── include
│   ├── rtweekend.h        // 常用工具函数和类型定义
│   ├── vec3.h            // 三维向量类 Vec3
│   ├── ray.h             // 光线类 Ray
│   ├── hittable.h        // 抽象基类 Hittable
│   ├── hittable_list.h   // HittableList 类
│   ├── sphere.h          // Sphere 类
│   ├── camera.h          // Camera 类
│   ├── material.h        // Material 类
│   └── interval.h        // 区间类
├── src
│   ├── main.cpp          // 程序入口点
│   └── sphere.cpp        // Sphere 类实现
├── CMakeLists.txt        // CMake 配置文件
├── .gitignore            // 版本控制忽略文件
└── README.md             // 项目文档
```

## 功能
- 实现光线与球体的交点检测。
- 支持多个可被光线撞到的物体。
- 提供简单的相机视角管理。
- 支持基本的材质属性。

## 使用方法
1. 克隆项目：
   ```
   git clone <repository-url>
   cd cpp-renderer
   ```

2. 构建项目：
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

3. 运行程序：
   ```
   ./cpp-renderer
   ```

## 贡献
欢迎任何形式的贡献！请提交问题或拉取请求。

## 许可证
该项目遵循 MIT 许可证。