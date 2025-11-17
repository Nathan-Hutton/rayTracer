#include "materials.h"
#include "lights.h"
#include "scene.h"
#include "renderer.h"
#include "photonmap.h"
#include <iostream>

namespace
{
    HaltonSeq<static_cast<int>(16)> haltonSeqPhi{ 2 };
    HaltonSeq<static_cast<int>(16)> haltonSeqTheta{ 3 };
}

//const bool doingDirectWithPhotonMapping{ true };

Color MtlBlinn::Shade(ShadeInfo const &shadeInfo) const
{
    Color finalColor{ emission.GetValue() };

    Vec3f normal{ shadeInfo.N().GetNormalized() };
    //return Color{ normal.x, normal.y, normal.z };

    // Calculate color for this object
    const Color diffuseColor{ diffuse.Eval(shadeInfo.UVW()) };

    if (doingDirectWithPhotonMapping)
    {
        if (IsPhotonSurface() || specular.GetValue().Sum() > 0.0f)
        {
            Color lightIntensity;
            Vec3f lightDir;
            renderer.GetPhotonMap()->EstimateIrradiance<128>(lightIntensity, lightDir, 1.0f, shadeInfo.P(), normal, 1.0f);

            if (IsPhotonSurface())
                finalColor += (1.0f / M_PI) * diffuseColor * lightIntensity;

            if (specular.GetValue().Sum() > 0.0f)
            {
                const Vec3f halfway{ (shadeInfo.V() + lightDir).GetNormalized() };
                const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
                finalColor += ((glossiness.GetValue() + 2) / (8.0f * M_PI)) * specular.GetValue() * pow(blinnTerm, glossiness.GetValue()) * lightIntensity;
            }
        }
    }
    else
    {
        for (int i{ 0 }; i < shadeInfo.NumLights(); ++i)
        {
            const Light* const light{ shadeInfo.GetLight(i) };
            Vec3f lightDir;
            Color lightIntensity{ light->Illuminate(shadeInfo, lightDir) };
            if (light->IsAmbient())
            {
                finalColor += diffuseColor * lightIntensity;
                continue;
            }

            const float geometryTerm{ std::max(0.0f, normal.Dot(lightDir)) };

            if (geometryTerm < 0.0f) continue;

            finalColor += (1.0f / M_PI) * diffuseColor * lightIntensity * geometryTerm;

            const Vec3f halfway{ (shadeInfo.V() + lightDir).GetNormalized() };
            const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
            finalColor += ((glossiness.GetValue() + 2) / (8.0f * M_PI)) * specular.GetValue() * pow(blinnTerm, glossiness.GetValue()) * lightIntensity * geometryTerm;
        }
    }

    if (!shadeInfo.CanBounce())
        return finalColor;

    if (!(doingDirectWithPhotonMapping && doingIndirectWithPhotonMapping))
    {
        if (doingIndirectWithPhotonMapping)
        {
            if (IsPhotonSurface() || specular.GetValue().Sum() > 0.0f)
            {
                Color lightIntensity;
                Vec3f lightDir;
                renderer.GetPhotonMap()->EstimateIrradiance<100>(lightIntensity, lightDir, 1.0f, shadeInfo.P(), normal, 1.0f);

                if (IsPhotonSurface())
                    finalColor += (1.0f / M_PI) * diffuseColor * lightIntensity;

                if (specular.GetValue().Sum() > 0.0f)
                {
                    const Vec3f halfway{ (shadeInfo.V() + lightDir).GetNormalized() };
                    const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
                    finalColor += ((glossiness.GetValue() + 2) / (8.0f * M_PI)) * specular.GetValue() * pow(blinnTerm, glossiness.GetValue()) * lightIntensity;
                }
            }
        }
        // Monte Carlo global illumination
        else if (diffuseColor.Sum() > 0.0f && shadeInfo.CurrentBounce() < 2)
        {
            const float phiOffset{ shadeInfo.RandomFloat() };
            const float thetaOffset{ shadeInfo.RandomFloat() };
            constexpr size_t numSamples{ 1 };
            Color colorSum{ 0.0f };
            for (size_t i{ 0 }; i < numSamples; ++i)
            {
                const float phi{ 2.0f * M_PI * fmod(haltonSeqPhi[shadeInfo.CurrentPixelSample() + i] + phiOffset, 1.0f) };
                //const float cosTheta{ fmod(haltonSeqTheta[shadeInfo.CurrentPixelSample() + i] + thetaOffset, 1.0f) };
                const float cosTheta{ sqrt(fmod(haltonSeqTheta[shadeInfo.CurrentPixelSample() + i] + thetaOffset, 1.0f)) };
                const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

                const float x{ sinTheta * cos(phi) };
                const float y{ sinTheta * sin(phi) };
                const float z{ cosTheta };

                Vec3f u, v;
                normal.GetOrthonormals(u, v);
                const Vec3f monteWorldDir{ (x * u) + (y * v) + (z * normal) };
                const Ray monteRay{ shadeInfo.P() + monteWorldDir * 0.0002f, monteWorldDir };

                float dist;
                //colorSum += shadeInfo.TraceSecondaryRay(monteRay, dist) * diffuseColor * cosTheta * 2.0f;
                colorSum += shadeInfo.TraceSecondaryRay(monteRay, dist) * diffuseColor;
            }
            finalColor += colorSum / static_cast<float>(numSamples);
        }
    }

    // Reflections
    if (reflection.GetValue().Sum() > 0.0f)
    {
        // **************
        // Glossiness ray
        // **************
        const float phiOffset{ shadeInfo.RandomFloat() };
        const float thetaOffset{ shadeInfo.RandomFloat() };
        const float phi{ 2.0f * M_PI * fmod(haltonSeqPhi[shadeInfo.CurrentPixelSample()] + phiOffset, 1.0f) };
        const float cosTheta{ pow(fmod(haltonSeqTheta[shadeInfo.CurrentPixelSample()] + thetaOffset, 1.0f), 1.0f / (glossiness.GetValue() + 1.0f)) };
        const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

        const float x{ sinTheta * cos(phi) };
        const float y{ sinTheta * sin(phi) };
        const float z{ cosTheta };

        Vec3f u, v;
        normal.GetOrthonormals(u, v);
        const Vec3f h{ (x * u) + (y * v) + (z * normal) };

        const Vec3f reflectionDir{ h * 2 * shadeInfo.V().Dot(h) - shadeInfo.V() };
        if (reflectionDir.Dot(normal) > 0.0f) 
        {
            const Ray reflectionRay{ shadeInfo.P() + reflectionDir * 0.0002f, reflectionDir };

            float dist;
            finalColor += shadeInfo.TraceSecondaryRay(reflectionRay, dist) * reflection.GetValue();
        }
    }

    // Refractions
    if (refraction.GetValue().Sum() > 0.0f)
    {
        const float phiOffset{ shadeInfo.RandomFloat() };
        const float thetaOffset{ shadeInfo.RandomFloat() };
        const Vec3f N{ shadeInfo.IsFront() ? normal : -normal };

        Vec3f u, v;
        N.GetOrthonormals(u, v);
        float dist;
        const float phi{ 2.0f * M_PI * fmod(haltonSeqPhi[shadeInfo.CurrentPixelSample()] + phiOffset, 1.0f) };
        const float cosTheta{ pow(fmod(haltonSeqTheta[shadeInfo.CurrentPixelSample()] + thetaOffset, 1.0f), 1.0f / (glossiness.GetValue() + 1.0f)) };
        const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

        const float x{ sinTheta * cos(phi) };
        const float y{ sinTheta * sin(phi) };
        const float z{ cosTheta };

        const Vec3f h{ (x * u) + (y * v) + (z * N) };

        const float viewDotH{ shadeInfo.V().Dot(h) };

        const float refractionRatio{ shadeInfo.IsFront() ? 1.0f / ior : ior };
        const float cosThetaRefractionSquared{ 1.0f - pow(refractionRatio, 2) * (1.0f - pow(viewDotH, 2)) };

        // Total internal reflection
        if (cosThetaRefractionSquared < 0.0f)
        { 
            const Vec3f reflectionDir{ h * 2 * viewDotH - shadeInfo.V() };
            if (reflectionDir.Dot(N) <= 0.0f)
                return finalColor;
            const Ray reflectionRay{ shadeInfo.P() + reflectionDir * 0.0002f, reflectionDir };

            float dist;
            Color colorFromReflection{ shadeInfo.TraceSecondaryRay(reflectionRay, dist) * refraction.GetValue() };
            finalColor += colorFromReflection;

            return finalColor;
        }

        // Fresnel
        const float fresnelRatio{ pow((1 - ior) / (1 + ior), 2) };
        const float fresnelReflectionAmount{ fresnelRatio + (1 - fresnelRatio) * pow(1 - viewDotH, 5) };

        Color colorFromReflection{ 0.0f };
        const Vec3f reflectionDir{ h * 2 * shadeInfo.V().Dot(h) - shadeInfo.V() };
        if (reflectionDir.Dot(N) > 0.0f)
        {
            const Ray reflectionRay{ shadeInfo.P() + reflectionDir * 0.0002f, reflectionDir };
            colorFromReflection = shadeInfo.TraceSecondaryRay(reflectionRay, dist) * refraction.GetValue();
        }

        finalColor += colorFromReflection * fresnelReflectionAmount;

        const Vec3f refractionDir{ -refractionRatio * shadeInfo.V() - (sqrt(cosThetaRefractionSquared) - refractionRatio * viewDotH) * h };
        const Ray refractionRay{ shadeInfo.P() + refractionDir * 0.0002f, refractionDir };
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
