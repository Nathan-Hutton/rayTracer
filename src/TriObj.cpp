#include "objects.h"

#include <iostream>
#include <cmath>
#include <algorithm>

bool TriObj::IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const
{
    float closestT{ BIGFLOAT };
    Vec3f closestX{};
    size_t closestFaceID{};
    float doubleArea0{};
    float doubleArea1{};
    float doubleArea2{};
    float doubleAreaTriangle{};

    bool hit{ false };
    bool hitFront{ false };
    for (size_t i{ 0 }; i < nf; ++i)
    {
        const TriFace& vertFace{ f[i] };

        const Vec3f& v0{ v[vertFace.v[0]] };
        const Vec3f& v1{ v[vertFace.v[1]] };
        const Vec3f& v2{ v[vertFace.v[2]] };

        const Vec3f e1{ v1 - v0 };
        const Vec3f e2{ v2 - v0 };
        const Vec3f normal{ e1 ^ e2 };

        const float normalDotRayDir{ normal % localRay.dir };
        const bool localHitFront{ normalDotRayDir <= 0.0f };
        if (hitSide == HIT_FRONT && !localHitFront) continue;
        if (hitSide == HIT_BACK && localHitFront) continue;

        const float h{ -v0 % normal };

        const float t{ -(localRay.p % normal + h) / normalDotRayDir };
        if (t < 0.0f || t > closestT) continue;
        const Vec3f x{ localRay.p + localRay.dir * t };

        const float normalXLen{ abs(normal.x) };
        const float normalYLen{ abs(normal.y) };
        const float normalZLen{ abs(normal.z) };

        Vec2f v0_2d;
        Vec2f v1_2d;
        Vec2f v2_2d;
        Vec2f x_2d;

        if (normalXLen > normalYLen && normalXLen > normalZLen)
        {
            v0_2d = Vec2f{ v0.y, v0.z };
            v1_2d = Vec2f{ v1.y, v1.z };
            v2_2d = Vec2f{ v2.y, v2.z };
            x_2d = Vec2f{ x.y, x.z };
        }
        else if (normalYLen > normalXLen && normalYLen > normalZLen)
        {
            v0_2d = Vec2f{ v0.x, v0.z };
            v1_2d = Vec2f{ v1.x, v1.z };
            v2_2d = Vec2f{ v2.x, v2.z };
            x_2d = Vec2f{ x.x, x.z };
        }
        else
        {
            v0_2d = Vec2f{ v0.x, v0.y };
            v1_2d = Vec2f{ v1.x, v1.y };
            v2_2d = Vec2f{ v2.x, v2.y };
            x_2d = Vec2f{ x.x, x.y };
        }

        const float tempDoubleArea0{ (v2_2d - v1_2d) ^ (x_2d - v1_2d) };
        const float tempDoubleArea1{ (v0_2d - v2_2d) ^ (x_2d - v2_2d) };
        const float tempDoubleArea2{ (v1_2d - v0_2d) ^ (x_2d - v0_2d) };
        
        // Check if the signs are inconsistent. If so, the point is outside. This is because 2D projection can ruin winding order
        if (!((tempDoubleArea0 >= 0 && tempDoubleArea1 >= 0 && tempDoubleArea2 >= 0) || (tempDoubleArea0 <= 0 && tempDoubleArea1 <= 0 && tempDoubleArea2 <= 0)))
            continue;

        doubleAreaTriangle = (v1_2d - v0_2d) ^ (v2_2d - v0_2d);
        doubleArea0 = tempDoubleArea0;
        doubleArea1 = tempDoubleArea1;
        doubleArea2 = tempDoubleArea2;
        closestT = t;
        closestX = x;
        closestFaceID = i;
        hitFront = localHitFront;
        hit = true;
    }

    if (!hit) return false;

    const float b0{ doubleArea0 / doubleAreaTriangle };
    const float b1{ doubleArea1 / doubleAreaTriangle };
    const float b2{ 1.0f - (b0 + b1) };
    
    const TriFace& normFace{ fn[closestFaceID] };

    hitInfo.z = closestT;
    hitInfo.p = closestX;
    hitInfo.N = (b0 * vn[normFace.v[0]] + b1 * vn[normFace.v[1]] + b2 * vn[normFace.v[2]]).GetNormalized();
    hitInfo.front = hitFront;

    return true;
} 

bool TriObj::IntersectShadowRay( Ray const &localRay, float t_max ) const
{
    for (size_t i{ 0 }; i < nf; ++i)
    {
        const TriFace& vertFace{ f[i] };

        const Vec3f& v0{ v[vertFace.v[0]] };
        const Vec3f& v1{ v[vertFace.v[1]] };
        const Vec3f& v2{ v[vertFace.v[2]] };

        const Vec3f e1{ v1 - v0 };
        const Vec3f e2{ v2 - v0 };
        const Vec3f normal{ e1 ^ e2 };

        const float h{ -v0 % normal };

        const float t{ -(localRay.p % normal + h) / (normal % localRay.dir) };
        if (t < 0.0f || t > t_max) continue;
        const Vec3f x{ localRay.p + localRay.dir * t };

        const float normalXLen{ abs(normal.x) };
        const float normalYLen{ abs(normal.y) };
        const float normalZLen{ abs(normal.z) };

        Vec2f v0_2d;
        Vec2f v1_2d;
        Vec2f v2_2d;
        Vec2f x_2d;

        if (normalXLen > normalYLen && normalXLen > normalZLen)
        {
            v0_2d = Vec2f{ v0.y, v0.z };
            v1_2d = Vec2f{ v1.y, v1.z };
            v2_2d = Vec2f{ v2.y, v2.z };
            x_2d = Vec2f{ x.y, x.z };
        }
        else if (normalYLen > normalXLen && normalYLen > normalZLen)
        {
            v0_2d = Vec2f{ v0.x, v0.z };
            v1_2d = Vec2f{ v1.x, v1.z };
            v2_2d = Vec2f{ v2.x, v2.z };
            x_2d = Vec2f{ x.x, x.z };
        }
        else
        {
            v0_2d = Vec2f{ v0.x, v0.y };
            v1_2d = Vec2f{ v1.x, v1.y };
            v2_2d = Vec2f{ v2.x, v2.y };
            x_2d = Vec2f{ x.x, x.y };
        }

        const float doubleArea0 = (v2_2d - v1_2d) ^ (x_2d - v1_2d);
        const float doubleArea1 = (v0_2d - v2_2d) ^ (x_2d - v2_2d);
        const float doubleArea2 = (v1_2d - v0_2d) ^ (x_2d - v0_2d);

        // Check if the signs are inconsistent. If so, the point is outside. This is because 2D projection can ruin winding order
        if (!((doubleArea0 >= 0 && doubleArea1 >= 0 && doubleArea2 >= 0) || (doubleArea0 <= 0 && doubleArea1 <= 0 && doubleArea2 <= 0)))
            continue;

        return true;
    }

    return false;
}

