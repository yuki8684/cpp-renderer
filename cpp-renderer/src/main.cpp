#include <iostream>
#include "hittable.h"
#include "ray.h"
#include "vec3.h"

int main() {
    // 示例代码：创建一个光线并输出其信息
    Ray r(Point3(0, 0, 0), Vec3(1, 0, 0));
    std::cout << "Ray origin: " << r.origin() << ", direction: " << r.direction() << std::endl;

    // 这里可以添加更多的逻辑，例如创建物体并检测光线与物体的交点

    return 0;
}