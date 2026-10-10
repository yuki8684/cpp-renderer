#include <iostream>
#include <memory>
#include "hittable_list.h"
#include "camera.h"
#include "renderer.h"
#include "visualizer.h"

int main() {
    // Set up the scene
    HittableList world;
    // Add objects to the world (e.g., spheres, planes, etc.)
    // Example: world.add(std::make_shared<Sphere>(...));

    // Set up the camera
    Camera camera;

    // Initialize the renderer
    Renderer renderer;

    // Initialize the visualizer
    Visualizer visualizer;

    // Render the scene
    visualizer.render(renderer.render(world, camera));

    std::cout << "Rendering complete!" << std::endl;
    return 0;
}