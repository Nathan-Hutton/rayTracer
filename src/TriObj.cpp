#include "objects.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <stack>
#include <utility>

bool IntersectRayBVHNode(Ray const &r, float t_max, const float* bounds, float* dist);

// Möller-Trumbore
bool TriObj::IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const
{
    float closestT{ BIGFLOAT };
    std::stack<std::pair<unsigned int, float>> nodeStack;
    {
        float dist;
        if (!IntersectRayBVHNode(localRay, closestT, bvh.GetNodeBounds(bvh.GetRootNodeID()), &dist)) return false;
        nodeStack.push({ bvh.GetRootNodeID(), dist});
    }

    Vec3f closestX{};
    float closestDet{};
    float closestU{};
    float closestV{};
    size_t closestFaceID{};

    constexpr float epsilon{ 1e-6 };
    bool hit{ false };
    while (!nodeStack.empty())
    {
        const std::pair<unsigned int, float> nodePair{ nodeStack.top() };
        nodeStack.pop();

        const float nodeT{ nodePair.second };
        if (closestT < nodeT)
            continue;

        const unsigned int parentID{ nodePair.first };

        if (bvh.IsLeafNode(parentID))
        {
            const unsigned int* elements{ bvh.GetNodeElements(parentID) };
            for (int i{ 0 }; i < bvh.GetNodeElementCount(parentID); ++i)
            {
                const TriFace& vertFace{ f[elements[i]] };

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
                closestFaceID = elements[i];
            }

            continue;
        }

        const unsigned int child1ID{ bvh.GetFirstChildNode(parentID) };
        float child1Dist;
        bool child1Hit{ IntersectRayBVHNode(localRay, closestT, bvh.GetNodeBounds(child1ID), &child1Dist) };
        child1Hit = child1Hit && child1Dist < closestT;

        const unsigned int child2ID{ bvh.GetSecondChildNode(parentID) };
        float child2Dist;
        bool child2Hit{ IntersectRayBVHNode(localRay, closestT, bvh.GetNodeBounds(child2ID), &child2Dist) };
        child2Hit = child2Hit && child2Dist < closestT;

        if (!child1Hit && !child2Hit) 
            continue;

        if (child1Hit && child2Hit)
        {
            if (child1Dist < child2Dist)
            {
                nodeStack.push({ child2ID, child2Dist });
                nodeStack.push({ child1ID, child1Dist });
                continue;
            }

            nodeStack.push({ child1ID, child1Dist });
            nodeStack.push({ child2ID, child2Dist });
            continue;
        }

        if (child1Hit)
        {
            nodeStack.push({ child1ID, child1Dist });
            continue;
        }

        nodeStack.push({ child2ID, child2Dist });
    }

    if (!hit) return false;
    
    const TriFace& normFace{ fn[closestFaceID] };
    const TriFace& texFace{ ft[closestFaceID] };

    hitInfo.z = closestT;
    hitInfo.p = closestX;
    hitInfo.front = closestDet > 0.0f;

    hitInfo.N = ((1.0f - closestU - closestV) * vn[normFace.v[0]] + closestU * vn[normFace.v[1]] + closestV * vn[normFace.v[2]]).GetNormalized();
    hitInfo.uvw = (1.0f - closestU - closestV) * vt[texFace.v[0]] + closestU * vt[texFace.v[1]] + closestV * vt[texFace.v[2]];

    return true;
} 

bool TriObj::IntersectShadowRay( Ray const &localRay, float t_max ) const
{
    std::stack<unsigned int> nodeStack;
    {
        float dist;
        if (!IntersectRayBVHNode(localRay, t_max, bvh.GetNodeBounds(bvh.GetRootNodeID()), &dist)) return false;
        nodeStack.push(bvh.GetRootNodeID());
    }

    constexpr float epsilon{ 1e-6 };
    while (!nodeStack.empty())
    {
        const unsigned int parentID{ nodeStack.top() };
        nodeStack.pop();

        if (bvh.IsLeafNode(parentID))
        {
            const unsigned int* elements{ bvh.GetNodeElements(parentID) };
            for (int i{ 0 }; i < bvh.GetNodeElementCount(parentID); ++i)
            {
                const TriFace& vertFace{ f[elements[i]] };

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

            continue;
        }

        const unsigned int child1ID{ bvh.GetFirstChildNode(parentID) };
        float child1Dist;
        bool child1Hit{ IntersectRayBVHNode(localRay, t_max, bvh.GetNodeBounds(child1ID), &child1Dist) };
        child1Hit = child1Hit;

        const unsigned int child2ID{ bvh.GetSecondChildNode(parentID) };
        float child2Dist;
        bool child2Hit{ IntersectRayBVHNode(localRay, t_max, bvh.GetNodeBounds(child2ID), &child2Dist) };
        child2Hit = child2Hit;

        if (!child1Hit && !child2Hit) 
            continue;

        if (child1Hit && child2Hit)
        {
            if (child1Dist < child2Dist)
            {
                nodeStack.push(child2ID);
                nodeStack.push(child1ID);
                continue;
            }

            nodeStack.push(child1ID);
            nodeStack.push(child2ID);
            continue;
        }

        if (child1Hit)
        {
            nodeStack.push(child1ID);
            continue;
        }

        nodeStack.push(child2ID);
    }

    return false;
}

bool Box::IntersectRay(Ray const &r, float t_max) const
{
    const Vec3f invDir{ 1.0f / r.dir.x, 1.0f / r.dir.y, 1.0f / r.dir.z };
    float tNear{ 0.0f };
    float tFar{ t_max };

    // x
    float t1{ (pmin.x - r.p.x) * invDir.x };
    float t2{ (pmax.x - r.p.x) * invDir.x };
    float slabTMin{ std::min(t1, t2) };
    float slabTMax{ std::max(t2, t1) };

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    // y
    t1 = (pmin.y - r.p.y) * invDir.y;
    t2 = (pmax.y - r.p.y) * invDir.y;

    slabTMin = std::min(t1, t2);
    slabTMax = std::max(t1, t2);

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    // z
    t1 = (pmin.z - r.p.z) * invDir.z;
    t2 = (pmax.z - r.p.z) * invDir.z;

    slabTMin = std::min(t1, t2);
    slabTMax = std::max(t1, t2);

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    return tNear < tFar;
}

bool IntersectRayBVHNode(Ray const &r, float t_max, const float* bounds, float* dist)
{
    const Vec3f invDir{ 1.0f / r.dir.x, 1.0f / r.dir.y, 1.0f / r.dir.z };
    float tNear{ 0.0f };
    float tFar{ t_max };

    const Vec3f pmin = Vec3f{ bounds[0], bounds[1], bounds[2] };
    const Vec3f pmax = Vec3f{ bounds[3], bounds[4], bounds[5] };

    // x
    float t1{ (pmin.x - r.p.x) * invDir.x };
    float t2{ (pmax.x - r.p.x) * invDir.x };
    float slabTMin{ std::min(t1, t2) };
    float slabTMax{ std::max(t2, t1) };

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    // y
    t1 = (pmin.y - r.p.y) * invDir.y;
    t2 = (pmax.y - r.p.y) * invDir.y;

    slabTMin = std::min(t1, t2);
    slabTMax = std::max(t1, t2);

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    // z
    t1 = (pmin.z - r.p.z) * invDir.z;
    t2 = (pmax.z - r.p.z) * invDir.z;

    slabTMin = std::min(t1, t2);
    slabTMax = std::max(t1, t2);

    tNear = std::max(tNear, slabTMin);
    tFar = std::min(tFar, slabTMax);
    if (tNear > tFar) return false;

    if (tNear < tFar)
    {
        *dist = tNear;
        return true;
    }

    return false;
}
