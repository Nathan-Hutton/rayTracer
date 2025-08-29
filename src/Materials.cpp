#include "Materials.h"
#include <iostream>

Color MtlPhong::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
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

        if (geometryTerm < 0.0f) continue;

        finalColor += diffuse * lightIntensity * geometryTerm;

        const Vec3f viewDir{ -ray.dir.GetNormalized() };
        const Vec3f reflection{ 2 * (lightDir.Dot(normal)) * normal - lightDir };
        const float phongTerm{ std::max(0.0f, reflection.Dot(viewDir)) };
        finalColor += specular * pow(phongTerm, glossiness) * lightIntensity;
    }

    return finalColor;
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

        if (geometryTerm < 0.0f) continue;

        finalColor += diffuse * lightIntensity * geometryTerm;

        const Vec3f viewDir{ -ray.dir.GetNormalized() };
        const Vec3f halfway{ (viewDir + lightDir).GetNormalized() };
        const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
        finalColor += specular * pow(blinnTerm, glossiness) * lightIntensity;
    }

    return finalColor;
}

Color MtlGGX::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const 
{
    return Color();
}
