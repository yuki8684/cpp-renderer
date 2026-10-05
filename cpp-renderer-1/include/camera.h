#pragma once

#include "vec3.h"

class Camera {
public:
    Camera() {
        // Initialize camera properties here
    }

    Ray get_ray(double u, double v) const {
        // Generate a ray from the camera through the pixel at (u, v)
        return Ray(); // Placeholder for actual ray generation logic
    }

private:
    // Camera properties such as position, direction, field of view, etc.
    Point3 origin;
    Vec3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
};