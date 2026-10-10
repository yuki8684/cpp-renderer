#pragma once

#include "hittable.h"
#include "camera.h"
#include "color.h"
#include <vector>

class Visualizer {
public:
    Visualizer(int width, int height);
    void render(const std::vector<std::shared_ptr<Hittable>>& objects, const Camera& camera);
    void display() const;

private:
    int width;
    int height;
    std::vector<Color> framebuffer;

    void clearFramebuffer();
    void setPixel(int x, int y, const Color& color);
};