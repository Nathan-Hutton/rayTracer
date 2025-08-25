#include "objects.h"

#include <iostream>
#include <cmath>

bool Sphere::IntersectRay( Ray const &normalizedLocalRay, HitInfo &hInfo, int hitSide ) const
{
    const float t{ -normalizedLocalRay.p.Dot(normalizedLocalRay.dir) };
    const Vec3f p{ normalizedLocalRay.p + normalizedLocalRay.dir * t };

    if (p.Length() > 1.0f)
        return false;

    const float x{ sqrtf(1.0f - p.LengthSquared()) };
    const float t1{ t - x };
    if (t1 < 0.0f)
        return false;

    const Vec3f surfaceHitSpot{ normalizedLocalRay.p + normalizedLocalRay.dir * t1 };
    if (surfaceHitSpot.GetNormalized().Dot(normalizedLocalRay.dir) > 0.0f)
        return false;

    hInfo.z = t1;
    hInfo.front = true;
    return true;
}
