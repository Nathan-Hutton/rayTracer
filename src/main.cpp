#include "scene.h"
#include "lights.h"
#include "renderer.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"
#include "rng.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

bool shootRay(const Node* const node, const Ray& ray, HitInfo& bestHitInfo, int hitSide)
{
    const Object* const obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };

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
    HaltonSeq<static_cast<int>(samplesPerPixel)> haltonSeqX{ 2 };
    HaltonSeq<static_cast<int>(samplesPerPixel)> haltonSeqY{ 3 };
}

bool Renderer::TraceRay(Ray const &ray, HitInfo &hInfo, int hitSide) const
{
    return shootRay(&scene.rootNode, ray, hInfo, HIT_FRONT_AND_BACK);
}

bool Renderer::TraceShadowRay(Ray const &ray, float t_max, int hitSide) const
{
    return shootShadowRay(&scene.rootNode, ray, t_max);
}

float ShadeInfo::TraceShadowRay(Ray const &ray, float t_max) const
{
    const Ray biasRay{ ray.p + ray.dir * 0.0002f, ray.dir };
    return renderer.TraceShadowRay(biasRay, t_max) ? 0.0f : 1.0f;
}

Color ShadeInfo::TraceSecondaryRay( Ray const &ray, float &dist ) const
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
    newShadeInfo.IncrementBounce();
    return hInfo.node->GetMaterial()->Shade(newShadeInfo);
}

// Random multi-sampling
//void threadRenderTiles()
//{
//    while (true)
//    {
//        int tileIndex{ tileThreads::tileCounter++ };
//        if (tileIndex >= tileThreads::totalNumTiles) break;
//
//        int imageX{ (tileIndex % tileThreads::numTilesX) * tileThreads::tileSize };
//        int imageY{ (tileIndex / tileThreads::numTilesX) * tileThreads::tileSize };
//        int tileWidth{ std::min(tileThreads::tileSize, renderer.GetCamera().imgWidth - imageX) };
//        int tileHeight{ std::min(tileThreads::tileSize, renderer.GetCamera().imgHeight - imageY) };
//
//        constexpr float averagingFraction{ 1.0f / static_cast<float>(tileThreads::samplesPerPixel) };
//        for (int j{ imageY }; j < imageY + tileHeight; ++j)
//        {
//            for (int i{ imageX }; i < imageX + tileWidth; ++i)
//            {
//                Color finalPixelColor{ 0.0f };
//                for (size_t sampleNum{ 0 }; sampleNum < tileThreads::samplesPerPixel; ++sampleNum)
//                {
//                    const float randomOffsetX{ tileThreads::rng.RandomFloat() - 0.5f };
//                    const float randomOffsetY{ tileThreads::rng.RandomFloat() - 0.5f };
//                    const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * (static_cast<float>(i) + 0.5f + randomOffsetX) };
//                    const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * (static_cast<float>(j) + 0.5f + randomOffsetY) };
//                    const Ray worldRay{ renderer.GetCamera().pos, (tileThreads::cameraToWorld * Vec3f{ spaceX, spaceY, -1.0f }) };
//
//                    HitInfo hitInfo{};
//                    ShadeInfo sInfo{ renderer.GetScene().lights, renderer.GetScene().environment };
//                    if (renderer.TraceRay(worldRay, hitInfo))
//                    {
//                        sInfo.SetHit(worldRay, hitInfo);
//                        finalPixelColor += hitInfo.node->GetMaterial()->Shade(sInfo) * averagingFraction;
//                    }
//                    else
//                    {
//                        const float u{ static_cast<float>(i) / static_cast<float>(renderer.GetCamera().imgWidth - 1) };
//                        const float v{ static_cast<float>(j) / static_cast<float>(renderer.GetCamera().imgHeight - 1) };
//                        finalPixelColor += renderer.GetScene().background.Eval( Vec3f{ u, v, 1.0f } ) * averagingFraction;
//                    }
//
//                    //renderer.GetRenderImage().GetZBuffer()[j * renderer.GetCamera().imgWidth + i] = hitInfo.z;
//                }
//                renderer.GetRenderImage().GetPixels()[j * renderer.GetCamera().imgWidth + i] = Color24{ finalPixelColor };
//            }
//        }
//    }
//}

// Halton
//void threadRenderTiles()
//{
//    while (true)
//    {
//        int tileIndex{ tileThreads::tileCounter++ };
//        if (tileIndex >= tileThreads::totalNumTiles) break;
//
//        int imageX{ (tileIndex % tileThreads::numTilesX) * tileThreads::tileSize };
//        int imageY{ (tileIndex / tileThreads::numTilesX) * tileThreads::tileSize };
//        int tileWidth{ std::min(tileThreads::tileSize, renderer.GetCamera().imgWidth - imageX) };
//        int tileHeight{ std::min(tileThreads::tileSize, renderer.GetCamera().imgHeight - imageY) };
//
//        constexpr float averagingFraction{ 1.0f / static_cast<float>(tileThreads::samplesPerPixel) };
//        for (int j{ imageY }; j < imageY + tileHeight; ++j)
//        {
//            for (int i{ imageX }; i < imageX + tileWidth; ++i)
//            {
//                Color finalPixelColor{ 0.0f };
//                for (size_t sampleNum{ 0 }; sampleNum < tileThreads::samplesPerPixel; ++sampleNum)
//                {
//                    const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * (static_cast<float>(i) + tileThreads::haltonSeqX[sampleNum]) };
//                    const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * (static_cast<float>(j) + tileThreads::haltonSeqY[sampleNum]) };
//                    const Ray worldRay{ renderer.GetCamera().pos, (tileThreads::cameraToWorld * Vec3f{ spaceX, spaceY, -1.0f }) };
//
//                    HitInfo hitInfo{};
//                    ShadeInfo sInfo{ renderer.GetScene().lights, renderer.GetScene().environment };
//                    if (renderer.TraceRay(worldRay, hitInfo))
//                    {
//                        sInfo.SetHit(worldRay, hitInfo);
//                        finalPixelColor += hitInfo.node->GetMaterial()->Shade(sInfo) * averagingFraction;
//                    }
//                    else
//                    {
//                        const float u{ static_cast<float>(i) / static_cast<float>(renderer.GetCamera().imgWidth - 1) };
//                        const float v{ static_cast<float>(j) / static_cast<float>(renderer.GetCamera().imgHeight - 1) };
//                        finalPixelColor += renderer.GetScene().background.Eval( Vec3f{ u, v, 1.0f } ) * averagingFraction;
//                    }
//                }
//                renderer.GetRenderImage().GetPixels()[j * renderer.GetCamera().imgWidth + i] = Color24{ finalPixelColor };
//            }
//        }
//    }
//}

// Adaptive
void threadRenderTiles()
{
    constexpr size_t minNumSamples{ 4 };
    constexpr size_t maxNumSamples{ 64 };
    constexpr std::array<float, maxNumSamples> tLookupTable
    {
        12.706f,
        4.303f,
        3.182f,
        2.776f,
        2.571f,
        2.447f,
        2.365f,
        2.306f,
        2.262f,
        2.228f,
        2.201f,
        2.179f,
        2.160f,
        2.145f,
        2.131f,
        2.120f,
        2.110f,
        2.101f,
        2.093f,
        2.086f,
        2.080f,
        2.074f,
        2.069f,
        2.064f,
        2.060f,
        2.056f,
        2.052f,
        2.048f,
        2.045f,
        2.042f,
        2.040f,
        2.037f,
        2.035f,
        2.032f,
        2.030f,
        2.028f,
        2.026f,
        2.024f,
        2.023f,
        2.021f,
        2.020f,
        2.018f,
        2.017f,
        2.015f,
        2.014f,
        2.013f,
        2.012f,
        2.011f,
        2.010f,
        2.009f,
        2.008f,
        2.007f,
        2.006f,
        2.005f,
        2.004f,
        2.003f,
        2.002f,
        2.001f,
        2.000f,
        1.999f,
        1.998f,
        1.997f,
        1.996f
    };

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
                for (size_t k{ 0 }; k < maxNumSamples; ++k)
                {
                    ++sampleCount;
                    const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * (static_cast<float>(i) + tileThreads::haltonSeqX[k]) };
                    const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * (static_cast<float>(j) + tileThreads::haltonSeqY[k]) };
                    const Ray worldRay{ renderer.GetCamera().pos, (tileThreads::cameraToWorld * Vec3f{ spaceX, spaceY, -1.0f }) };

                    HitInfo hitInfo{};
                    ShadeInfo sInfo{ renderer.GetScene().lights, renderer.GetScene().environment };
                    if (renderer.TraceRay(worldRay, hitInfo))
                    {
                        sInfo.SetHit(worldRay, hitInfo);
                        const Color c{ hitInfo.node->GetMaterial()->Shade(sInfo) };
                        colorSum += c;
                        colorSumSquared += c * c;
                    }
                    else
                    {
                        const float u{ static_cast<float>(i) / static_cast<float>(renderer.GetCamera().imgWidth - 1) };
                        const float v{ static_cast<float>(j) / static_cast<float>(renderer.GetCamera().imgHeight - 1) };
                        const Color c{ renderer.GetScene().background.Eval( Vec3f{ u, v, 1.0f } ) };
                        colorSum += c;
                        colorSumSquared += c * c;
                    }

                    if (sampleCount < minNumSamples)
                        continue;

                    const Color meanColor{ colorSum / sampleCount };
                    Color sigma{ (colorSumSquared - ((colorSum * colorSum) / static_cast<float>(sampleCount))) / (static_cast<float>(sampleCount - 1)) };
                    sigma.r = sqrtf(sigma.r);
                    sigma.g = sqrtf(sigma.g);
                    sigma.b = sqrtf(sigma.b);
                    const Color delta{ tLookupTable[sampleCount - 1] * (sigma / sqrtf(static_cast<float>(sampleCount))) };
                    constexpr float deltaMax{ 0.01f };

                    if (delta.r < deltaMax && delta.g < deltaMax && delta.b < deltaMax)
                        break;
                }
                renderer.GetRenderImage().GetPixels()[j * renderer.GetCamera().imgWidth + i] = Color24{ colorSum / static_cast<float>(sampleCount) };
                renderer.GetRenderImage().GetSampleCount()[j * renderer.GetCamera().imgWidth + i] = sampleCount;
            }
        }
    }
}


int main()
{
    renderer.LoadScene("../assets/scene.xml");
    //renderer.LoadScene("../assets/simpleScene.xml");

    tileThreads::numTilesX = (renderer.GetCamera().imgWidth + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::numTilesY = (renderer.GetCamera().imgHeight + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::totalNumTiles = tileThreads::numTilesX * tileThreads::numTilesY;
    tileThreads::tileCounter = 0;

    const Vec3 camZ{ -renderer.GetCamera().dir.GetNormalized() };
    const Vec3f camX{ renderer.GetCamera().up.Cross(camZ).GetNormalized() };
    const Vec3f camY{ camZ.Cross(camX) };
    tileThreads::cameraToWorld = Matrix3f{ camX, camY, camZ };

    constexpr float Pi = 3.14159265358979323846f;
    tileThreads::imagePlaneHalfHeight = tanf((static_cast<float>(renderer.GetCamera().fov) * Pi / 180.0f) / 2.0f);

    const float aspectRatio{ static_cast<float>(renderer.GetCamera().imgWidth) / static_cast<float>(renderer.GetCamera().imgHeight) };
    tileThreads::imagePlaneHalfWidth = aspectRatio * tileThreads::imagePlaneHalfHeight;
    tileThreads::pixelSize = (tileThreads::imagePlaneHalfWidth * 2.0f) / static_cast<float>(renderer.GetCamera().imgWidth);

    const auto start{ std::chrono::high_resolution_clock::now() };

    const size_t numThreads{ std::thread::hardware_concurrency() };
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
    renderer.GetRenderImage().SaveZImage("../zbuffer.png");
    renderer.GetRenderImage().SaveSampleCountImage("../sampleCount.png");
    renderer.GetRenderImage().SaveImage("../image.png");

    ShowViewport(&renderer);
    return 0;
}
