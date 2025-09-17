#include "Materials.h"
#include "lights.h"
#include "scene.h"
#include <iostream>

Color MtlBlinn::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights, int bounceCount) const
{
    Color finalColor{};

    Vec3f normal{ hInfo.N.GetNormalized() };
    const Vec3f viewDir{ -ray.dir.GetNormalized() };
    //return Color{ normal.x, normal.y, normal.z };

    // Calculate color for this object
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

        const Vec3f halfway{ (viewDir + lightDir).GetNormalized() };
        const float blinnTerm{ std::max(0.0f, normal.Dot(halfway)) };
        finalColor += specular * pow(blinnTerm, glossiness) * lightIntensity;// * geometryTerm;
    }

    if (bounceCount == 0)
        return finalColor;

    // Reflections
    if (reflection.Sum() > 0.0f)
    {
        const Vec3f perfectReflectionDir{ normal * 2 * viewDir.Dot(normal) - viewDir };
        const Ray reflectionRay{ hInfo.p + perfectReflectionDir * 0.0002f, perfectReflectionDir };
        HitInfo reflectionHitInfo{};
        if (shootRay(lightsGlobalVars::rootNode, reflectionRay, reflectionHitInfo))
            finalColor += reflectionHitInfo.node->GetMaterial()->Shade(reflectionRay, reflectionHitInfo, lights, bounceCount - 1) * reflection;
    }

    // Refractions
    if (refraction.Sum() > 0.0f)
    {
        const Vec3f N{ hInfo.front ? normal : -normal };
        const float viewDotN{ viewDir.Dot(N) };

        const float eta{ hInfo.front ? 1.0f / ior : ior };
        const float cosThetaRefractionSquared{ 1.0f - pow(eta, 2) * (1.0f - pow(viewDotN, 2)) };

        const Vec3f perfectReflectionDir{ N * 2 * viewDotN - viewDir };
        const Ray reflectionRay{ hInfo.p + perfectReflectionDir * 0.0002f, perfectReflectionDir };

        // Total internal reflection
        if (cosThetaRefractionSquared < 0.0f)
        { 
            HitInfo reflectionHitInfo{};
            if (shootRay(lightsGlobalVars::rootNode, reflectionRay, reflectionHitInfo, HIT_FRONT_AND_BACK))
            {
                Color colorFromReflection{ reflectionHitInfo.node->GetMaterial()->Shade(reflectionRay, reflectionHitInfo, lights, bounceCount - 1) * refraction };
                const float absorptionRed{ exp(-absorption[0] * reflectionHitInfo.z) };
                const float absorptionGreen{ exp(-absorption[1] * reflectionHitInfo.z) };
                const float absorptionBlue{ exp(-absorption[2] * reflectionHitInfo.z) };
                colorFromReflection[0] *= absorptionRed;
                colorFromReflection[1] *= absorptionGreen;
                colorFromReflection[2] *= absorptionBlue;
                finalColor += colorFromReflection;
            }

            return finalColor;
        }

        // Fresnel
        const float fresnelRatio{ pow((1 - ior) / (1 + ior), 2) };
        const float fresnelReflectionAmount{ fresnelRatio + (1 - fresnelRatio) * pow(1 - viewDotN, 5) };
        HitInfo reflectionHitInfo{};
        if (shootRay(lightsGlobalVars::rootNode, reflectionRay, reflectionHitInfo, HIT_FRONT_AND_BACK))
        {
            const Color colorFromReflection{ reflectionHitInfo.node->GetMaterial()->Shade(reflectionRay, reflectionHitInfo, lights, bounceCount - 1) * refraction };
            finalColor += colorFromReflection * fresnelReflectionAmount;
        }

        const Vec3f refractionDir{ -eta * viewDir - (sqrt(cosThetaRefractionSquared) - eta * viewDotN) * N };
        Ray refractionRay{ hInfo.p + refractionDir * 0.0002f, refractionDir };
        HitInfo refractionHitInfo{};
        if (shootRay(lightsGlobalVars::rootNode, refractionRay, refractionHitInfo, HIT_FRONT_AND_BACK))
        {
            Color colorFromRefraction{ refractionHitInfo.node->GetMaterial()->Shade(refractionRay, refractionHitInfo, lights, bounceCount - 1) * refraction };
            if (!refractionHitInfo.front)
            {
                const float absorptionRed{ exp(-absorption[0] * refractionHitInfo.z) };
                const float absorptionGreen{ exp(-absorption[1] * refractionHitInfo.z) };
                const float absorptionBlue{ exp(-absorption[2] * refractionHitInfo.z) };
                colorFromRefraction[0] *= absorptionRed;
                colorFromRefraction[1] *= absorptionGreen;
                colorFromRefraction[2] *= absorptionBlue;
            }
            finalColor += colorFromRefraction * (1.0f - fresnelReflectionAmount);
        }
    }

    return finalColor;
}

//Color MtlPhong::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights, int bounceCount) const
//{
//    Color finalColor{};
//
//    const Vec3f normal{ hInfo.N.GetNormalized() };
//    //return Color{ normal.x, normal.y, normal.z };
//
//    for (const Light* const light : lights)
//    {
//        const Color lightIntensity{ light->Illuminate(hInfo.p, normal) };
//
//        if (light->IsAmbient())
//        {
//            finalColor += diffuse * lightIntensity;
//            continue;
//        }
//
//        const Vec3f lightDir{ -light->Direction(hInfo.p) };
//        const float geometryTerm{ std::max(0.0f, normal.Dot(lightDir)) };
//
//        if (geometryTerm < 0.0f) continue;
//
//        finalColor += diffuse * lightIntensity * geometryTerm;
//
//        const Vec3f viewDir{ -ray.dir.GetNormalized() };
//        const Vec3f reflection{ 2 * (lightDir.Dot(normal)) * normal - lightDir };
//        const float phongTerm{ std::max(0.0f, reflection.Dot(viewDir)) };
//        finalColor += specular * pow(phongTerm, glossiness) * lightIntensity;
//    }
//
//    return finalColor;
//}

Color MtlMicrofacet::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights, int bounceCount) const
{
    return Color{};
}
