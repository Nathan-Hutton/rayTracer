#include "objects.h"

#include <iostream>
#include <cmath>

/*
struct HitInfo
{
	float       z;		// the distance from the ray center to the hit point
	Node const *node;	// the object node that was hit
	bool        front;	// true if the ray hits the front side, false if the ray hits the back side

	HitInfo() { Init(); }
	void Init() { z=BIGFLOAT; node=nullptr; front=true; }
};
*/

// For completeness, have code to handle all hitSide conditions
bool Sphere::IntersectRay( Ray const &normalizedLocalRay, HitInfo &hInfo, int hitSide ) const
{
    const float t{ -normalizedLocalRay.p.Dot(normalizedLocalRay.dir) };
    const Vec3f p{ normalizedLocalRay.p + normalizedLocalRay.dir * t };

    if (p.Length() > 1.0f)
        return false;

    const float x{ sqrtf(1.0f - p.LengthSquared()) };
    const float t1{ t - x };

    hInfo.z = t1;
    hInfo.front = true;
    return true;
}
