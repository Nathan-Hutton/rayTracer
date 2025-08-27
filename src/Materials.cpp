#include "Materials.h"
#include <iostream>

Color MtlPhong::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    return Color();
}

Color MtlBlinn::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    Color finalColor{};

    const Node* node{ hInfo.node };
    const Vec3f worldSpaceHitPoint{ node->TransformFrom(hInfo.p) };
    const Vec3f worldSpaceNormal{ node->NormalTransformFrom(hInfo.N) };

    for (const Light* const light : lights)
    {
        if (light->IsAmbient())
        {
            finalColor += diffuse * light->Illuminate(worldSpaceHitPoint, worldSpaceNormal);
            continue;
        }
    }

    return finalColor;
}

Color MtlGGX::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const 
{
    return Color();
}
