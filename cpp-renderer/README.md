# C++ Renderer

## 项目简介
该项目是一个简单的 C++ 渲染器，旨在实现基本的光线追踪功能。项目包含多个模块，用于表示可以被光线撞到的物体、光线本身、三维向量以及一些常用的工具函数。

## 文件结构
```
cpp-renderer
├── include
│   ├── hittable.h      // 定义抽象基类 Hittable 和 HitRecord 结构体
│   ├── ray.h           // 光线相关的定义和实现
│   ├── rtweekend.h     // 常用工具函数和类型定义
│   └── vec3.h          // 三维向量类 Vec3 的定义
├── src
│   └── main.cpp        // 程序入口点，包含主函数和主要逻辑
├── tests               // 测试代码目录
├── CMakeLists.txt      // CMake 配置文件
├── .gitignore          // Git 忽略文件
└── README.md           // 项目文档和说明
```

## 使用说明
1. **构建项目**: 使用 CMake 构建项目。在项目根目录下运行以下命令：
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

2. **运行程序**: 构建完成后，运行生成的可执行文件。

## 贡献
欢迎任何形式的贡献！请提交问题或拉取请求以帮助改进项目。

## 许可证
该项目遵循 MIT 许可证。请查看 LICENSE 文件以获取更多信息。