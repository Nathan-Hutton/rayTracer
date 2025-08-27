#include "objects.h"

#include <iostream>
#include <cmath>

bool Sphere::IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const
{
    const float a{ localRay.dir.Dot(localRay.dir) };
    const float b{ 2 * localRay.dir.Dot(localRay.p) };
    const float c{ localRay.p.Dot(localRay.p) - 1.0f };

    const float discriminant{ b*b - 4*a*c };
    if (discriminant < 0.0f) return hitSide == HIT_NONE;

    const float discriminantSquareRoot{ sqrtf(discriminant) };
    const float inverse2A{ 1.0f / (2.0f * a) };
    const float t1{ (-b - discriminantSquareRoot) * inverse2A };
    //const float t2{ (-b + discriminantSquareRoot) * inverse2A };

    if (t1 < 0.0f) return false;

    hitInfo.z = t1;
    hitInfo.p = localRay.p + localRay.dir * t1;
    hitInfo.N = hitInfo.p;
    return true;
}

