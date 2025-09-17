#include "objects.h"

#include <iostream>
#include <cmath>
#include <algorithm>

bool Plane::IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const
{
    if (abs(localRay.dir.z) < 1e-6f) return false;

    const float t{ -localRay.p.z / localRay.dir.z };
    if (t < 0.0f) return false;

    const Vec3f pos{ localRay.p + localRay.dir * t };
    if (abs(pos.x) > 1.0f || abs(pos.y) > 1.0f) return false;

    hitInfo.z = t;
    hitInfo.p = pos;
    hitInfo.N = Vec3f{ 0.0f, 0.0f, 1.0f };
    hitInfo.front = localRay.dir.z < 0.0f;
    return true;
} 

