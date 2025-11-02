#include "materials.h"
#include "lights.h"
#include "scene.h"
#include <iostream>

namespace
{
    HaltonSeq<static_cast<int>(16)> haltonSeqPhi{ 2 };
    HaltonSeq<static_cast<int>(16)> haltonSeqTheta{ 3 };
}


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
            finalColor += diffuse.Eval(shadeInfo.UVW()) * lightIntensity;
            continue;
        }

        const float geometryTerm{ std::max(0.0f, normal.Dot(lightDir)) };

        if (geometryTerm < 0.0f) continue;

        finalColor += diffuse.Eval(shadeInfo.UVW()) * lightIntensity * geometryTerm;

        const Vec3f halfway{ (shadeInfo.V() + lightDir).GetNormalized() };
        const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
        finalColor += specular.GetValue() * pow(blinnTerm, glossiness.GetValue()) * lightIntensity * geometryTerm;
    }

    if (!shadeInfo.CanBounce())
        return finalColor;

    // Reflections
    const float phiOffset{ shadeInfo.RandomFloat() };
    const float thetaOffset{ shadeInfo.RandomFloat() };
    if (reflection.GetValue().Sum() > 0.0f)
    {
        Vec3f u, v;
        normal.GetOrthonormals(u, v);
        Color accumulatedColor{ 0.0f };

        const float phi{ 2.0f * M_PI * fmod(haltonSeqPhi[0] + phiOffset, 1.0f) };
        const float cosTheta{ pow(fmod(haltonSeqTheta[0] + thetaOffset, 1.0f), 1.0f / (glossiness.GetValue() + 1.0f)) };
        const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

        const float x{ sinTheta * cos(phi) };
        const float y{ sinTheta * sin(phi) };
        const float z{ cosTheta };

        const Vec3f hLocal{ x, y, z };
        const Vec3f h{ (x * u) + (y * v) + (z * normal) };

        const Vec3f reflectionDir{ h * 2 * shadeInfo.V().Dot(h) - shadeInfo.V() };
        if (reflectionDir.Dot(normal) > 0.0f) 
        {
            const Ray reflectionRay{ shadeInfo.P() + reflectionDir * 0.0002f, reflectionDir };

            float dist;
            finalColor += shadeInfo.TraceSecondaryRay(reflectionRay, dist) * reflection.GetValue();
        }

        //const Vec3f perfectReflectionDir{ normal * 2 * shadeInfo.V().Dot(normal) - shadeInfo.V() };
        //const Ray reflectionRay{ shadeInfo.P() + perfectReflectionDir * 0.0002f, perfectReflectionDir };

        //float dist;
        //finalColor += shadeInfo.TraceSecondaryRay(reflectionRay, dist) * reflection.GetValue();
    }

    // Refractions
    if (refraction.GetValue().Sum() > 0.0f)
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
            Color colorFromReflection{ shadeInfo.TraceSecondaryRay(reflectionRay, dist) * refraction.GetValue() };
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
        Color colorFromRefraction{ shadeInfo.TraceSecondaryRay(refractionRay, dist) * refraction.GetValue() };
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
