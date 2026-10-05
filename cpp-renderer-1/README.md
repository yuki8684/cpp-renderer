# cpp-renderer

## Overview
cpp-renderer is a simple ray tracing renderer implemented in C++. The project demonstrates fundamental concepts of computer graphics, including ray-object intersection, color manipulation, and scene rendering.

## Project Structure
The project is organized into the following directories and files:

```
cpp-renderer
├── include
│   ├── camera.h          // Camera class for rendering scenes
│   ├── color.h           // Color representation and manipulation
│   ├── hittable.h        // HitRecord structure and Hittable class
│   ├── hittable_list.h   // Management of multiple Hittable objects
│   ├── ray.h             // Ray class representing rays in 3D space
│   ├── rtweekend.h       // Utility functions and constants
│   ├── sphere.h          // Sphere class implementing Hittable
│   └── vec3.h            // 3D vector operations
├── src
│   └── main.cpp          // Entry point of the application
├── .github
│   └── workflows
│       └── build.yml     // GitHub Actions workflow for building the project
├── .gitignore            // Files and directories to ignore by Git
├── CMakeLists.txt        // CMake configuration file
└── README.md             // Project documentation
```

## Setup Instructions
To build and run the project, follow these steps:

1. **Clone the repository:**
   ```
   git clone <repository-url>
   cd cpp-renderer
   ```

2. **Install dependencies:**
   Ensure you have CMake installed on your system. You can download it from [CMake's official website](https://cmake.org/download/).

3. **Build the project:**
   ```
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **Run the application:**
   After building, you can run the application with:
   ```
   ./cpp-renderer
   ```

## Usage
The application will render a scene based on the configurations set in `main.cpp`. You can modify the scene parameters and objects in this file to experiment with different rendering results.

## Contributing
Contributions are welcome! Please feel free to submit a pull request or open an issue for any suggestions or improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.