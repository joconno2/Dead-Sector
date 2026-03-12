#pragma once
#include <cmath>

struct Vec2 {
    float x = 0.f, y = 0.f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2  operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2  operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2  operator*(float s)       const { return {x * s,   y * s};   }
    Vec2  operator/(float s)       const { return {x / s,   y / s};   }
    Vec2& operator+=(const Vec2& o)      { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o)      { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)            { x *= s;   y *= s;   return *this; }

    float length()   const { return std::sqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float l = length();
        return l > 0.f ? Vec2{x / l, y / l} : Vec2{};
    }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }

    Vec2 rotated(float angle) const {
        float c = std::cos(angle), s = std::sin(angle);
        return {x * c - y * s, x * s + y * c};
    }

    // Heading vector: angle 0 = up (negative Y), clockwise positive
    static Vec2 fromAngle(float angle) {
        return {std::sin(angle), -std::cos(angle)};
    }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }
