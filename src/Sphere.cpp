#include "objects.h"

#include <iostream>
#include <cmath>
#include <algorithm>

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

    if (hitSide == HIT_FRONT)
    {
        if (t1 <= 0.0f)
            return false;

        hitInfo.z = t1;
        hitInfo.p = localRay.p + localRay.dir * t1;
        hitInfo.N = hitInfo.p;
        hitInfo.front = true;

        const float u{ (1.0f / (2.0f * static_cast<float>(M_PI))) * atan2(hitInfo.p.y, hitInfo.p.x) + 0.5f };
        const float v{ (1.0f / static_cast<float>(M_PI) * asin(hitInfo.p.z) + 0.5f) };
        hitInfo.uvw = Vec3f{ u, v, 1.0f };

        return true;

    }

    const float t2{ (-b + discriminantSquareRoot) * inverse2A };
    if (hitSide == HIT_BACK)
    {
        if (t1 > 0.0f || t2 < 0.0f)
            return false;

        hitInfo.z = t2;
        hitInfo.p = localRay.p + localRay.dir * t2;
        hitInfo.N = hitInfo.p;
        hitInfo.front = false;

        const float u{ (1.0f / (2.0f * static_cast<float>(M_PI))) * atan2(hitInfo.p.y, hitInfo.p.x) + 0.5f };
        const float v{ (1.0f / static_cast<float>(M_PI) * asin(hitInfo.p.z) + 0.5f) };
        hitInfo.uvw = Vec3f{ u, v, 1.0f };

        return true;
    }


    if (hitSide == HIT_FRONT_AND_BACK)
    {
        if (t1 < 0.0f && t2 < 0.0f)
            return false;

        if (t1 > 0.0f)
        {
            hitInfo.z = t1;
            hitInfo.p = localRay.p + localRay.dir * t1;
            hitInfo.N = hitInfo.p;

            const float u{ (1.0f / (2.0f * static_cast<float>(M_PI))) * atan2(hitInfo.p.y, hitInfo.p.x) + 0.5f };
            const float v{ (1.0f / static_cast<float>(M_PI) * asin(hitInfo.p.z) + 0.5f) };
            hitInfo.uvw = Vec3f{ u, v, 1.0f };

            hitInfo.front = true;
        }
        else
        {
            hitInfo.z = t2;
            hitInfo.p = localRay.p + localRay.dir * t2;
            hitInfo.N = hitInfo.p;

            const float u{ (1.0f / (2.0f * static_cast<float>(M_PI))) * atan2(hitInfo.p.y, hitInfo.p.x) + 0.5f };
            const float v{ (1.0f / static_cast<float>(M_PI) * asin(hitInfo.p.z) + 0.5f) };
            hitInfo.uvw = Vec3f{ u, v, 1.0f };

            hitInfo.front = false;
        }

        return true;
    }

    return false;
} 

bool Sphere::IntersectShadowRay( Ray const &localRay, float t_max ) const
{
    const float a{ localRay.dir.Dot(localRay.dir) };
    const float b{ 2 * localRay.dir.Dot(localRay.p) };
    const float c{ localRay.p.Dot(localRay.p) - 1.0f };

    const float discriminant{ b*b - 4*a*c };
    if (discriminant < 0.0f) return false;

    const float discriminantSquareRoot{ sqrtf(discriminant) };
    const float inverse2A{ 1.0f / (2.0f * a) };

    const float t1{ (-b - discriminantSquareRoot) * inverse2A };
    const float t2{ (-b + discriminantSquareRoot) * inverse2A };

    if (t1 >= 0.0f) return t1 < t_max;
    return t2 >= 0.0f && t2 < t_max;
}

