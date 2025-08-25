#include "objects.h"

#include <iostream>
#include <cmath>

bool Sphere::IntersectRay( Ray const &normalizedLocalRay, HitInfo &hInfo, int hitSide ) const
{
    const float sphereToRayDist{ -normalizedLocalRay.p.Dot(normalizedLocalRay.dir) };
    const Vec3f projectCenterToRayDir{ normalizedLocalRay.p + normalizedLocalRay.dir * sphereToRayDist };

    if (projectCenterToRayDir.Length() > 1.0f)
        return false;

    const float x{ sqrtf(1.0f - projectCenterToRayDir.LengthSquared()) };
    const float hitPoint1{ sphereToRayDist - x };
    if (hitPoint1 < 0.0f)
        return false;

    // Maybe this is unnecessary?
    const Vec3f surfaceHitSpot{ normalizedLocalRay.p + normalizedLocalRay.dir * hitPoint1 };
    if (surfaceHitSpot.GetNormalized().Dot(normalizedLocalRay.dir) > 0.0f)
        return false;

    hInfo.z = hitPoint1;
    hInfo.front = true;
    return true;
}
