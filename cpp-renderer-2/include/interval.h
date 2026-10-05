#pragma once

// ============================================================
// 区间类，用于表示光线参数的有效范围
// ============================================================
class Interval {
public:
    double min; // 区间的最小值
    double max; // 区间的最大值

    Interval(double min = 0.0, double max = 0.0) : min(min), max(max) {}

    // 检查一个值是否在区间内
    bool contains(double value) const {
        return value >= min && value <= max;
    }

    // 返回区间的长度
    double length() const {
        return max - min;
    }

    // 检查两个区间是否相交
    bool intersects(const Interval& other) const {
        return (min < other.max) && (max > other.min);
    }
};