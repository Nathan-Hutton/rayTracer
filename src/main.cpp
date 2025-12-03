#include "scene.h"
#include "lights.h"
#include "renderer.h"
#include "materials.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"
#include "rng.h"
#include "photonmap.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

bool shootRay(const Node* const node, const Ray& ray, HitInfo& bestHitInfo, int hitSide)
{
    const Object* const obj{ node->GetNodeObj() };
    const Ray localRay{ node->ToNodeCoords(ray) };

    bool hitObject{ false };
    if (obj != nullptr)
    {
        HitInfo hitInfo{};
        if (obj->IntersectRay(localRay, hitInfo, hitSide))
        {
            bestHitInfo = hitInfo;
            bestHitInfo.node = node;
            node->FromNodeCoords(bestHitInfo);
            hitObject = true;
        }
    }

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        HitInfo childHitInfo{};
        const Node* const childNode{ node->GetChild(i) };
        if (shootRay(childNode, localRay, childHitInfo, hitSide))
        {
            if (childHitInfo.z < bestHitInfo.z)
            {
                bestHitInfo = childHitInfo;
                node->FromNodeCoords(bestHitInfo);
            }
            hitObject = true;
        }
    }

    return hitObject;
}

bool shootRayAtLights(const Ray& ray, HitInfo& bestHitInfo)
{
    for (int i{ 0 }; i < static_cast<int>(renderer.GetScene().lights.size()); ++i)
    {
        const Light* const light{ renderer.GetScene().lights[i] };

        HitInfo hitInfo{};
        if (!light->IntersectRay(ray, hitInfo))
            continue;

        if (hitInfo.z < bestHitInfo.z)
        {
            bestHitInfo = hitInfo;
            return true;
        }
    }

    return false;
}

bool shootShadowRay(const Node* const node, const Ray& ray, float t_max)
{
    const Object* const obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };

    if (obj != nullptr)
        if (obj->IntersectShadowRay(localRay, t_max))
            return true;

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        const Node* const childNode{ node->GetChild(i) };
        if (shootShadowRay(childNode, localRay, t_max)) 
            return true;
    }

    return false;
}

namespace tileThreads
{
    constexpr int tileSize{ 16 };
    int numTilesX{};
    int numTilesY{};
    int totalNumTiles{};
    std::atomic<int> tileCounter{};

    float imagePlaneHalfWidth{};
    float imagePlaneHalfHeight{};
    float pixelSize{};
    Matrix3f cameraToWorld{};

    Color24* pixels{ nullptr };
    float* depthValues{ nullptr };

    constexpr size_t samplesPerPixel{ 16 };
    RNG rng{};
    HaltonSeq<static_cast<int>(samplesPerPixel)> aaHaltonSeqX{ 2 };
    HaltonSeq<static_cast<int>(samplesPerPixel)> aaHaltonSeqY{ 3 };
    HaltonSeq<static_cast<int>(samplesPerPixel)> diskHaltonSeqTheta{ 5 };
    HaltonSeq<static_cast<int>(samplesPerPixel)> diskHaltonSeqRadius{ 7 };
}

bool Renderer::TraceRay(Ray const &ray, HitInfo &hInfo, int hitSide) const
{
    bool output{ shootRay(&scene.rootNode, ray, hInfo, HIT_FRONT_AND_BACK) };
    output |= shootRayAtLights(ray, hInfo);
    return output;
}

bool Renderer::TraceShadowRay(Ray const &ray, float t_max, int hitSide) const
{
    return shootShadowRay(&scene.rootNode, ray, t_max);
}

# ifdef LEGACY_SHADING_API
float ShadeInfo::TraceShadowRay(Ray const &ray, float t_max) const
{
    const Ray biasRay{ ray.p + ray.dir * 0.0002f, ray.dir };
    return renderer.TraceShadowRay(biasRay, t_max) ? 0.0f : 1.0f;
}

Color ShadeInfo::TraceSecondaryRay( Ray const &ray, float &dist, bool reflection ) const
{
    if (!CanBounce())
    {
        dist = BIGFLOAT;
        return Color(0,0,0);
    }

    HitInfo hInfo{};
    if (!renderer.TraceRay(ray, hInfo))
    {
        dist = BIGFLOAT;
        return EvalEnvironment(ray.dir);
    }

    dist = hInfo.z;
    ShadeInfo newShadeInfo{ *this };
    newShadeInfo.SetHit(ray, hInfo);
    ++newShadeInfo.bounce;
    if (hInfo.light)
        return hInfo.node->GetMaterial()->Emission();

    return hInfo.node->GetMaterial()->Shade(newShadeInfo);
}
#endif

Color tracePath(Ray ray)
{
    Color throughput{ 1.0f };
    Color result{ 0.0f };
    constexpr size_t maxBounces{ 5 };
    const Light* light{ renderer.GetScene().lights[0] };

    float lastBounceProb{ 1.0f };

    for (size_t bounce{ 0 }; bounce < maxBounces; ++bounce)
    {
        HitInfo hInfo{};
        if (!renderer.TraceRay(ray, hInfo, HIT_FRONT_AND_BACK))
        {
            const Color c{ renderer.GetScene().background.Eval(ray.dir) };
            result += c * throughput;
            return result;
        }

        const Vec3f normal{ hInfo.N.GetNormalized() };

        if (hInfo.light)
        {
            if (bounce == 0)
            {
                result += light->Intensity() * throughput;
            }
            //else
            //{
            //    //const float distSquared{ powf((hInfo.p - ray.p).Length(), 2.0f) };
            //    const float distSquared{ hInfo.z * hInfo.z };
            //    const float cosThetaLight{ std::max(1e-4f, hInfo.N.Dot(-ray.dir)) };
            //    const float lightAreaPDF{ 1.0f / (4.0f * M_PI * light->GetSize() * light->GetSize()) };
            //    const float lightSolidPDF{ lightAreaPDF * (distSquared / cosThetaLight) };

            //    const float lastBouncePDFSquared{ lastBounceProb * lastBounceProb };
            //    const float lightSolidPDFSquared{ lightSolidPDF * lightSolidPDF };
            //    const float weight{ lastBouncePDFSquared / (lastBouncePDFSquared + lightSolidPDFSquared) };

            //    result += light->Intensity() * throughput * weight;
            //}
            return result;
        }

        const MtlBasePhongBlinn* material{ static_cast<const MtlBasePhongBlinn*>(hInfo.node->GetMaterial()) };
        SamplerInfo sInfo{ tileThreads::rng };
        sInfo.SetHit(ray, hInfo);

        // Next event estimation
        DirSampler::Info nextEventInfo;
        Vec3f nextEventShadowDir;
        if (light->GenerateSample(sInfo, nextEventShadowDir, nextEventInfo))
        {
            const float sign{ hInfo.front ? 1.0f : -1.0f };
            const Ray nextEventShadowRay{ hInfo.p + (normal * 0.002f * sign), nextEventShadowDir };
            if (!renderer.TraceShadowRay(nextEventShadowRay, nextEventInfo.dist - 0.002f, HIT_FRONT_AND_BACK))
            {
                const float cosThetaSurface{ std::max(0.0f, normal.Dot(nextEventShadowDir)) };
                if (cosThetaSurface > 0.0f && nextEventInfo.prob > 0.0f)
                {
                    const Color diffuse{ material->Diffuse().GetValue() };
                    Color brdf{ diffuse / Pi<float>() };

                    const Vec3f h{ (nextEventShadowDir - ray.dir).GetNormalized() };
                    const float blinnTerm{ std::max(0.0f, normal.Dot(h)) };
                    
                    if (blinnTerm > 0.0f)
                    {
                        const Color specular{ material->Specular().GetValue() };
                        const float gloss{ material->Glossiness().GetValue() };
                        const float specNorm{ (gloss * 8.0f) / (8.0f * Pi<float>()) };
                        brdf += specular * specNorm * pow(blinnTerm, gloss);
                    }

                    result += (brdf * cosThetaSurface * nextEventInfo.mult) / nextEventInfo.prob * throughput;
                }
            }
        }
        break;

        // Indirect bounce
        Vec3f bounceDir;
        DirSampler::Info indirectLightingInfo;
        if (!material->GenerateSample(sInfo, bounceDir, indirectLightingInfo))
            break;

        lastBounceProb = indirectLightingInfo.prob;

        ray.dir = bounceDir;
        const float sign{ (hInfo.N.Dot(bounceDir) > 0.0f) ? 1.0f : -1.0f };
        ray.p = hInfo.p + (hInfo.N * 0.002f * sign);

        throughput *= indirectLightingInfo.mult / indirectLightingInfo.prob;

        if (bounce > 2)
        {
            const float prob{ throughput.Max() };
            if (tileThreads::rng.RandomFloat() > prob)
                break;

            throughput /= prob;
        }
    }

    return result;
}

// Adaptive
void threadRenderTiles()
{
    constexpr size_t minNumSamples{ 16 };
    constexpr size_t maxNumSamples{ 64 };

    while (true)
    {
        const int tileIndex{ tileThreads::tileCounter++ };
        if (tileIndex >= tileThreads::totalNumTiles) break;

        const int imageX{ (tileIndex % tileThreads::numTilesX) * tileThreads::tileSize };
        const int imageY{ (tileIndex / tileThreads::numTilesX) * tileThreads::tileSize };
        const int tileWidth{ std::min(tileThreads::tileSize, renderer.GetCamera().imgWidth - imageX) };
        const int tileHeight{ std::min(tileThreads::tileSize, renderer.GetCamera().imgHeight - imageY) };

        for (int j{ imageY }; j < imageY + tileHeight; ++j)
        {
            for (int i{ imageX }; i < imageX + tileWidth; ++i)
            {
                Color colorSum{ 0.0f };
                Color colorSumSquared{ 0.0f };
                size_t sampleCount{ 0 };

                const float aaOffsetPixelX{ tileThreads::rng.RandomFloat() };
                const float aaOffsetPixelY{ tileThreads::rng.RandomFloat() };
                const float dofOffsetTheta{ tileThreads::rng.RandomFloat() };
                const float dofOffsetRadius{ tileThreads::rng.RandomFloat() };

                for (size_t k{ 0 }; k < maxNumSamples; ++k)
                {
                    ++sampleCount;

                    const float jitterX{ fmod(tileThreads::aaHaltonSeqX[k] + aaOffsetPixelX, 1.0f) };
                    const float jitterY{ fmod(tileThreads::aaHaltonSeqY[k] + aaOffsetPixelY, 1.0f) };
                    const float pixelX{ static_cast<float>(i) + jitterX };
                    const float pixelY{ static_cast<float>(j) + jitterY };
                    const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * pixelX };
                    const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * pixelY };
                    const Vec3f worldRayDestination{ renderer.GetCamera().pos + tileThreads::cameraToWorld * Vec3f{spaceX, spaceY, -renderer.GetCamera().focaldist} };

                    const float jitterTheta{ fmod(tileThreads::diskHaltonSeqTheta[k] + dofOffsetTheta, 1.0f) };
                    const float jitterRadius{ fmod(tileThreads::diskHaltonSeqRadius[k] + dofOffsetRadius, 1.0f) };
                    const float diskTheta{ jitterTheta * 2.0f * M_PI };
                    const float diskRadius{ sqrt(jitterRadius) };
                    const Vec3f cameraRayPosOffset{
                        diskRadius * renderer.GetCamera().dof * cos(diskTheta),
                        diskRadius * renderer.GetCamera().dof * sin(diskTheta),
                        0.0f
                    };

                    const Vec3f worldRayPos{ renderer.GetCamera().pos + tileThreads::cameraToWorld * cameraRayPosOffset };
                    const Vec3f worldRayDir{ worldRayDestination - worldRayPos };
                    const Ray worldRay{ worldRayPos, worldRayDir };

                    HitInfo hitInfo{};
                    //ShadeInfo sInfo{ renderer.GetScene().lights, renderer.GetScene().environment, tileThreads::rng };
                    //sInfo.SetPixelSample(i);
                    const Color c{ tracePath(worldRay) };
                    colorSum += c;
                    colorSumSquared += c * c;
                    //if (renderer.TraceRay(worldRay, hitInfo))
                    //{
                    //    if (hitInfo.light)
                    //    {
                    //        const Color c{ 1.0f };
                    //        colorSum += c;
                    //        colorSumSquared += c * c;
                    //    }
                    //    else
                    //    {
                    //        //sInfo.SetHit(worldRay, hitInfo);
                    //        //const Color c{ hitInfo.light ? Color{ 1.0f, 1.0f, 1.0f } : hitInfo.node->GetMaterial()->Shade(sInfo) };
                    //        // TODO: Path tracing algorithm
                    //        colorSum += c;
                    //        colorSumSquared += c * c;
                    //    }
                    //}
                    //else
                    //{
                    //    const float u{ (static_cast<float>(i) + tileThreads::aaHaltonSeqX[k]) / static_cast<float>(renderer.GetCamera().imgWidth - 1) };
                    //    const float v{ (static_cast<float>(j) + tileThreads::aaHaltonSeqY[k]) / static_cast<float>(renderer.GetCamera().imgHeight - 1) };
                    //    const Color c{ renderer.GetScene().background.Eval( Vec3f{ u, v, 1.0f } ) };
                    //    colorSum += c;
                    //    colorSumSquared += c * c;
                    //}

                    if (sampleCount < minNumSamples)
                        continue;

                    const Color meanColor{ colorSum / sampleCount };
                    Color sigma{ (colorSumSquared - ((colorSum * colorSum) / static_cast<float>(sampleCount))) / (static_cast<float>(sampleCount - 1)) };
                    sigma.r = fmaxf(0.0f, sigma.r);
                    sigma.g = fmaxf(0.0f, sigma.g);
                    sigma.b = fmaxf(0.0f, sigma.b);
                    sigma.r = sqrtf(sigma.r);
                    sigma.g = sqrtf(sigma.g);
                    sigma.b = sqrtf(sigma.b);
                    const Color delta{ 3.0f * (sigma / sqrtf(static_cast<float>(sampleCount))) };
                    constexpr float deltaMax{ 0.01f };

                    if (delta.r < deltaMax && delta.g < deltaMax && delta.b < deltaMax)
                        break;
                }

                Color c{ colorSum / static_cast<float>(sampleCount) };
                if (renderer.GetCamera().sRGB)
                    c = c.Linear2sRGB();

                renderer.GetRenderImage().GetPixels()[j * renderer.GetCamera().imgWidth + i] = Color24{ c };
                renderer.GetRenderImage().GetSampleCount()[j * renderer.GetCamera().imgWidth + i] = static_cast<int>(sampleCount);
                renderer.GetRenderImage().IncrementNumRenderPixel(1);
            }
        }
    }
}


int main()
{
    renderer.LoadScene("../assets/scene.xml");
    //PhotonMap photonMap{};
    //photonMap.Resize(100000);
    //renderer.SetPhotonMap(&photonMap);

    //PhotonMap causticsMap{};
    //causticsMap.Resize(100000);
    //renderer.SetCausticsMap(&causticsMap);

    tileThreads::numTilesX = (renderer.GetCamera().imgWidth + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::numTilesY = (renderer.GetCamera().imgHeight + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::totalNumTiles = tileThreads::numTilesX * tileThreads::numTilesY;
    tileThreads::tileCounter = 0;

    const Vec3 camZ{ -renderer.GetCamera().dir.GetNormalized() };
    const Vec3f camX{ renderer.GetCamera().up.Cross(camZ).GetNormalized() };
    const Vec3f camY{ camZ.Cross(camX) };
    tileThreads::cameraToWorld = Matrix3f{ camX, camY, camZ };

    constexpr float Pi = 3.14159265358979323846f;
    tileThreads::imagePlaneHalfHeight = renderer.GetCamera().focaldist * tanf((static_cast<float>(renderer.GetCamera().fov) * Pi / 180.0f) / 2.0f);

    const float aspectRatio{ static_cast<float>(renderer.GetCamera().imgWidth) / static_cast<float>(renderer.GetCamera().imgHeight) };
    tileThreads::imagePlaneHalfWidth = aspectRatio * tileThreads::imagePlaneHalfHeight;
    tileThreads::pixelSize = (tileThreads::imagePlaneHalfWidth * 2.0f) / static_cast<float>(renderer.GetCamera().imgWidth);

    const auto start{ std::chrono::high_resolution_clock::now() };

    // Fill in photon map

    // Direct only
    //if ((&doingIndirectWithPhotonMapping && doingDirectWithPhotonMapping) || monteCarloWithPhoton)
    //{
    //    const Light* light{ renderer.GetScene().lights[0] };
    //    while (true)
    //    {
    //        Ray photonRay;
    //        Color c;
    //        light->RandomPhoton(tileThreads::rng, photonRay, c);
    //        photonRay.p += photonRay.dir * 0.0002f;

    //        HitInfo hInfo{};
    //        if (!renderer.TraceRay(photonRay, hInfo))
    //            continue;

    //        const Material* material{ hInfo.node->GetMaterial() };
    //        if (!material->IsPhotonSurface())
    //            continue;

    //        if (!photonMap.AddPhoton(hInfo.p, photonRay.dir, c))
    //            break;
    //    }
    //}
    //// Direct + indirect
    //else if (doingIndirectWithPhotonMapping && doingDirectWithPhotonMapping)
    //{
    //    const Light* light{ renderer.GetScene().lights[0] };
    //    bool mapIsFull{ false };

    //    while (!mapIsFull)
    //    {
    //        Ray photonRay;
    //        Color c;
    //        light->RandomPhoton(tileThreads::rng, photonRay, c);

    //        while (true)
    //        {
    //            photonRay.p += photonRay.dir * 0.0002f;
    //            HitInfo hInfo{};
    //            if (!renderer.TraceRay(photonRay, hInfo) || hInfo.light)
    //                break;

    //            const Material* material{ hInfo.node->GetMaterial() };
    //            if (material->IsPhotonSurface() && !photonMap.AddPhoton(hInfo.p, photonRay.dir, c))
    //            {
    //                mapIsFull = true;
    //                break;
    //            }

    //            SamplerInfo sInfo{ tileThreads::rng };
    //            sInfo.SetHit(photonRay, hInfo);

    //            DirSampler::Info info{};
    //            Vec3f newDir{};
    //            if (!material->GenerateSample(sInfo, newDir, info))
    //                break;

    //            photonRay.dir = newDir;
    //            photonRay.p = hInfo.p;
    //            c *= info.mult / info.prob;
    //        }
    //    }
    //}
    //// Indirect. No direct
    //else if (doingIndirectWithPhotonMapping)
    //{
    //    const Light* light{ renderer.GetScene().lights[0] };
    //    bool mapIsFull{ false };

    //    while (!mapIsFull)
    //    {
    //        Ray photonRay;
    //        Color c;
    //        light->RandomPhoton(tileThreads::rng, photonRay, c);

    //        bool firstHit{ true };

    //        while (true)
    //        {
    //            photonRay.p += photonRay.dir * 0.0002f;
    //            HitInfo hInfo{};
    //            if (!renderer.TraceRay(photonRay, hInfo) || hInfo.light)
    //                break;

    //            SamplerInfo sInfo{ tileThreads::rng };
    //            sInfo.SetHit(photonRay, hInfo);
    //            DirSampler::Info info{};
    //            Vec3f newDir{};
    //            const Material* material{ hInfo.node->GetMaterial() };
    //            const bool bounce{ material->GenerateSample(sInfo, newDir, info) };

    //            if (firstHit)
    //            {
    //                if (!bounce || (doingCaustics && info.lobe != DirSampler::Lobe::DIFFUSE))
    //                    break;

    //                firstHit = false;
    //                photonRay.dir = newDir;
    //                photonRay.p = hInfo.p;
    //                c *= info.mult / info.prob;
    //                continue;
    //            }

    //            if (material->IsPhotonSurface() && !photonMap.AddPhoton(hInfo.p, photonRay.dir, c))
    //            {
    //                mapIsFull = true;
    //                break;
    //            }

    //            if (!bounce)
    //                break;

    //            photonRay.dir = newDir;
    //            photonRay.p = hInfo.p;
    //            c *= info.mult / info.prob;
    //        }
    //    }
    //}
    //photonMap.ScalePhotonPowers(1.0f / static_cast<float>(photonMap.NumPhotons()));
    //photonMap.PrepareForIrradianceEstimation();

    //if (doingCaustics)
    //{
    //    const Light* light{ renderer.GetScene().lights[0] };
    //    bool mapIsFull{ false };

    //    while (!mapIsFull)
    //    {
    //        Ray photonRay;
    //        Color c;
    //        light->RandomPhoton(tileThreads::rng, photonRay, c);

    //        bool firstHit{ true };

    //        while (true)
    //        {
    //            photonRay.p += photonRay.dir * 0.0002f;
    //            HitInfo hInfo{};
    //            if (!renderer.TraceRay(photonRay, hInfo) || hInfo.light)
    //                break;

    //            SamplerInfo sInfo{ tileThreads::rng };
    //            sInfo.SetHit(photonRay, hInfo);
    //            DirSampler::Info info{};
    //            Vec3f newDir{};
    //            const Material* material{ hInfo.node->GetMaterial() };
    //            const bool bounce{ material->GenerateSample(sInfo, newDir, info) };

    //            if (firstHit)
    //            {
    //                if (!bounce || info.lobe == DirSampler::Lobe::DIFFUSE)
    //                    break;

    //                firstHit = false;
    //                photonRay.dir = newDir;
    //                photonRay.p = hInfo.p;
    //                c *= info.mult / info.prob;
    //                continue;
    //            }

    //            if (material->IsPhotonSurface())
    //            {
    //                mapIsFull = !causticsMap.AddPhoton(hInfo.p, photonRay.dir, c);
    //                break;
    //            }

    //            if (!bounce) // No need to check if it's a diffuse bounce since we already check IsPhotonSurface()
    //                break;

    //            photonRay.dir = newDir;
    //            photonRay.p = hInfo.p;
    //            c *= info.mult / info.prob;
    //        }
    //    }
    //    causticsMap.ScalePhotonPowers(1.0f / static_cast<float>(causticsMap.NumPhotons()));
    //    causticsMap.PrepareForIrradianceEstimation();
    //}

    // Render image
    const size_t numThreads{ std::thread::hardware_concurrency() };
    //const size_t numThreads{ 1 };
    std::vector<std::thread> threads;
    for (size_t i{ 0 }; i < numThreads; ++i)
        threads.emplace_back(threadRenderTiles);

    for (auto& t : threads)
        t.join();

    const auto end{ std::chrono::high_resolution_clock::now() };
    const auto durationMilli{ std::chrono::duration_cast<std::chrono::milliseconds>(end - start) };
    const auto durationSeconds{ std::chrono::duration_cast<std::chrono::seconds>(end - start) };
    std::cout << "\nTime: " << durationSeconds << " : " << durationMilli % 1000 << '\n';

    renderer.GetRenderImage().ComputeZBufferImage();
    renderer.GetRenderImage().ComputeSampleCountImage();
    renderer.GetRenderImage().SaveZImage("../zbuffer.png");
    renderer.GetRenderImage().SaveSampleCountImage("../sampleCount.png");
    renderer.GetRenderImage().SaveImage("../image.png");

    ShowViewport(&renderer);
    return 0;
}
