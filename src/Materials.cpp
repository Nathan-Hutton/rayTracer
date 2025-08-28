#include "Materials.h"
#include <iostream>

Color MtlPhong::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    return Color();
}

Color MtlBlinn::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    Color finalColor{};

    const Vec3f normal{ hInfo.N.GetNormalized() };
    //return Color{ normal.x, normal.y, normal.z };

    for (const Light* const light : lights)
    {
        const Color lightIntensity{ light->Illuminate(hInfo.p, normal) };

        if (light->IsAmbient())
        {
            finalColor += diffuse * lightIntensity;
            continue;
        }

        const Vec3f lightDir{ -light->Direction(hInfo.p) };
        const float geometryTerm{ std::max(0.0f, normal.Dot(lightDir)) };
        finalColor += diffuse * lightIntensity * geometryTerm;

        //const Vec3f halfway{ (ray.dir + lightDir).GetNormalized() };
        //const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
        //finalColor += specular * blinnTerm * lightIntensity;
    }

    return finalColor;
}

Color MtlGGX::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const 
{
    return Color();
}
