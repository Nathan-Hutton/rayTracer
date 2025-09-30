#include "materials.h"
#include "lights.h"
#include "scene.h"
#include <iostream>


Color MtlBlinn::Shade(ShadeInfo const &shadeInfo) const
{
    Color finalColor{};

    Vec3f normal{ shadeInfo.N().GetNormalized() };
    //return Color{ normal.x, normal.y, normal.z };

    // Calculate color for this object
    for (int i{ 0 }; i < shadeInfo.NumLights(); ++i)
    {
        const Light* const light{ shadeInfo.GetLight(i) };
        Vec3f lightDir;
        const Color lightIntensity{ light->Illuminate(shadeInfo, lightDir) };

        if (light->IsAmbient())
        {
            finalColor += diffuse * lightIntensity;
            continue;
        }

        const float geometryTerm{ std::max(0.0f, normal.Dot(lightDir)) };

        if (geometryTerm < 0.0f) continue;

        finalColor += diffuse * lightIntensity * geometryTerm;

        const Vec3f halfway{ (shadeInfo.V() + lightDir).GetNormalized() };
        const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
        finalColor += specular * pow(blinnTerm, glossiness) * lightIntensity;// * geometryTerm;
    }

    if (!shadeInfo.CanBounce())
        return finalColor;

    // Reflections
    if (reflection.Sum() > 0.0f)
    {
        const Vec3f perfectReflectionDir{ normal * 2 * shadeInfo.V().Dot(normal) - shadeInfo.V() };
        const Ray reflectionRay{ shadeInfo.P() + perfectReflectionDir * 0.0002f, perfectReflectionDir };

        float dist;
        finalColor += shadeInfo.TraceSecondaryRay(reflectionRay, dist) * reflection;
    }

    // Refractions
    if (refraction.Sum() > 0.0f)
    {
        const Vec3f N{ shadeInfo.IsFront() ? normal : -normal };
        const float viewDotN{ shadeInfo.V().Dot(N) };

        const float refractionRatio{ shadeInfo.IsFront() ? 1.0f / ior : ior };
        const float cosThetaRefractionSquared{ 1.0f - pow(refractionRatio, 2) * (1.0f - pow(viewDotN, 2)) };

        // Total internal reflection
        if (cosThetaRefractionSquared < 0.0f)
        { 
            const Vec3f perfectReflectionDir{ N * 2 * viewDotN - shadeInfo.V() };
            const Ray reflectionRay{ shadeInfo.P() + perfectReflectionDir * 0.0002f, perfectReflectionDir };

            float dist;
            Color colorFromReflection{ shadeInfo.TraceSecondaryRay(reflectionRay, dist) * refraction };
            const float absorptionRed{ exp(-absorption[0] * dist) };
            const float absorptionGreen{ exp(-absorption[1] * dist) };
            const float absorptionBlue{ exp(-absorption[2] * dist) };
            colorFromReflection[0] *= absorptionRed;
            colorFromReflection[1] *= absorptionGreen;
            colorFromReflection[2] *= absorptionBlue;
            finalColor += colorFromReflection;

            return finalColor;
        }

        // Fresnel
        const float fresnelRatio{ pow((1 - ior) / (1 + ior), 2) };
        const float fresnelReflectionAmount{ fresnelRatio + (1 - fresnelRatio) * pow(1 - viewDotN, 5) };
        const Vec3f perfectReflectionDir{ N * 2 * viewDotN - shadeInfo.V() };
        const Ray reflectionRay{ shadeInfo.P() + perfectReflectionDir * 0.0002f, perfectReflectionDir };
        float dist;
        const Color colorFromReflection{ shadeInfo.TraceSecondaryRay(reflectionRay, dist) };
        finalColor += colorFromReflection * fresnelReflectionAmount;

        const Vec3f refractionDir{ -refractionRatio * shadeInfo.V() - (sqrt(cosThetaRefractionSquared) - refractionRatio * viewDotN) * N };
        Ray refractionRay{ shadeInfo.P() + refractionDir * 0.0002f, refractionDir };
        Color colorFromRefraction{ shadeInfo.TraceSecondaryRay(refractionRay, dist) * refraction };
        const float absorptionRed{ exp(-absorption[0] * dist) };
        const float absorptionGreen{ exp(-absorption[1] * dist) };
        const float absorptionBlue{ exp(-absorption[2] * dist) };
        colorFromRefraction[0] *= absorptionRed;
        colorFromRefraction[1] *= absorptionGreen;
        colorFromRefraction[2] *= absorptionBlue;
        finalColor += colorFromRefraction * (1.0f - fresnelReflectionAmount);
    }

    return finalColor;
}
