# cpp-renderer

## Overview
The `cpp-renderer` project is a ray tracing renderer implemented in C++. It utilizes a bounding volume hierarchy (BVH) for efficient ray-object intersection tests, allowing for realistic rendering of 3D scenes.

## Project Structure
```
cpp-renderer
├── include
│   ├── aabb.h          // Defines the AABB class for axis-aligned bounding boxes
│   ├── bvh.h           // Implements the BVHNode class for bounding volume hierarchy
│   ├── camera.h        // Handles camera position, orientation, and projection
│   ├── color.h         // Provides color representation and manipulation utilities
│   ├── hittable.h      // Defines the Hittable interface for intersection tests
│   ├── hittable_list.h // Manages a collection of Hittable objects
│   ├── material.h      // Represents material properties and light interaction
│   ├── ray.h           // Represents a ray in 3D space
│   ├── sphere.h        // Defines the Sphere class for sphere intersection tests
│   ├── vec3.h          // Represents a 3D vector with various operations
│   └── visualizer.h     // Responsible for rendering the scene visually
├── src
│   ├── main.cpp        // Entry point of the application
│   ├── renderer.cpp    // Implements the rendering logic
│   └── visualizer.cpp   // Implements the visualizer for graphical representation
├── CMakeLists.txt      // CMake configuration file for building the project
└── README.md           // Documentation for the project
```

## Setup Instructions
1. **Clone the repository**:
   ```
   git clone <repository-url>
   cd cpp-renderer
   ```

2. **Build the project**:
   - Ensure you have CMake installed.
   - Create a build directory and navigate into it:
     ```
     mkdir build
     cd build
     ```
   - Run CMake to configure the project:
     ```
     cmake ..
     ```
   - Build the project:
     ```
     make
     ```

## Usage
- Run the application:
  ```
  ./cpp-renderer
  ```
- The application will initialize the renderer and visualizer, set up the scene, and start the rendering process.

## Components
- **AABB**: Represents axis-aligned bounding boxes and provides methods for intersection tests.
- **BVHNode**: Implements a bounding volume hierarchy for efficient ray-object intersection.
- **Camera**: Handles the camera's position and generates rays for rendering.
- **Color**: Provides utilities for color representation and manipulation.
- **Hittable**: Interface for objects that can be hit by rays.
- **HittableList**: Manages a collection of hittable objects for intersection tests.
- **Material**: Represents the material properties of objects in the scene.
- **Ray**: Represents a ray in 3D space.
- **Sphere**: Defines a sphere and includes methods for intersection tests.
- **Vec3**: Represents a 3D vector with various operations.
- **Visualizer**: Responsible for rendering the scene visually.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.