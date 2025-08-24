#include "objects.h"

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

bool Sphere::IntersectRay( Ray const &ray, HitInfo &hInfo, int hitSide ) const
{
    // Transform ray from camera space to object space
    return true;
}
