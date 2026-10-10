#pragma once

#include "vec3.h"

// ============================================================
// Material class represents the material properties of objects.
// It includes methods for handling light interaction and shading.
// ============================================================
class Material {
public:
    virtual ~Material() = default;

    // Method to compute the scattered ray and attenuation
    virtual bool scatter(const Ray& incoming, const HitRecord& rec, Vec3& attenuation, Ray& scattered) const = 0;

    // Method to get the emitted light from the material
    virtual Vec3 emitted(double u, double v, const Vec3& p) const {
        return Vec3(0, 0, 0); // Default to no emission
    }
};