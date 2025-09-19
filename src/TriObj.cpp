#include "objects.h"

#include <iostream>
#include <cmath>
#include <algorithm>

// Möller-Trumbore
bool TriObj::IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const
{
    float closestT{ BIGFLOAT };
    Vec3f closestX{};
    float closestDet{};
    float closestU{};
    float closestV{};
    size_t closestFaceID{};
    bool hitFront{ false };

    constexpr float epsilon{ 1e-6 };
    bool hit{ false };
    for (size_t i{ 0 }; i < nf; ++i)
    {
        const TriFace& vertFace{ f[i] };

        const Vec3f& v0{ v[vertFace.v[0]] };
        const Vec3f& v1{ v[vertFace.v[1]] };
        const Vec3f& v2{ v[vertFace.v[2]] };

        const Vec3f e1{ v1 - v0 };
        const Vec3f e2{ v2 - v0 };

        const Vec3f rayCrossE2{ localRay.dir ^ e2 };
        const float det{ e1 % rayCrossE2 };

        if (det > -epsilon && det < epsilon) continue;

        if (hitSide == HIT_FRONT && det < 0.0f) continue;
        if (hitSide == HIT_BACK && det > 0.0f) continue;

        const float invDet{ 1.0f / det };
        const Vec3f s{ localRay.p - v0 };

        const float u{ invDet * (s % rayCrossE2) };
        if (u < 0.0f || u > 1.0f) continue;

        const Vec3f sCrossE1{ s ^ e1 };
        const float v{ invDet * (localRay.dir % sCrossE1) };
        if (v < 0.0f || u + v > 1.0f) continue;

        const float t{ invDet * (e2 % sCrossE1) };
        if (t <= epsilon || t > closestT) continue;

        hit = true;
        closestT = t;
        closestX = localRay.p + localRay.dir * t;
        closestDet = det;
        closestU = u;
        closestV = v;
        closestFaceID = i;
    }

    if (!hit) return false;
    
    const TriFace& normFace{ fn[closestFaceID] };

    hitInfo.z = closestT;
    hitInfo.p = closestX;
    hitInfo.front = closestDet > 0.0f;

    hitInfo.N = ((1.0f - closestU - closestV) * vn[normFace.v[0]] + closestU * vn[normFace.v[1]] + closestV * vn[normFace.v[2]]).GetNormalized();

    return true;
} 

bool TriObj::IntersectShadowRay( Ray const &localRay, float t_max ) const
{
    constexpr float epsilon{ 1e-6 };
    for (size_t i{ 0 }; i < nf; ++i)
    {
        const TriFace& vertFace{ f[i] };

        const Vec3f& v0{ v[vertFace.v[0]] };
        const Vec3f& v1{ v[vertFace.v[1]] };
        const Vec3f& v2{ v[vertFace.v[2]] };

        const Vec3f e1{ v1 - v0 };
        const Vec3f e2{ v2 - v0 };

        const Vec3f rayCrossE2{ localRay.dir ^ e2 };
        const float det{ e1 % rayCrossE2 };

        if (det > -epsilon && det < epsilon) continue;

        const float invDet{ 1.0f / det };
        const Vec3f s{ localRay.p - v0 };

        const float u{ invDet * (s % rayCrossE2) };
        if (u < 0.0f || u > 1.0f) continue;

        const Vec3f sCrossE1{ s ^ e1 };
        const float v{ invDet * (localRay.dir % sCrossE1) };
        if (v < 0.0f || u + v > 1.0f) continue;

        const float t{ invDet * (e2 % sCrossE1) };
        if (t <= epsilon || t > t_max) continue;

        return true;
    }

    return false;
}

