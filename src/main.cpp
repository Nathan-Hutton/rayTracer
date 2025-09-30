#include "scene.h"
#include "lights.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

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

namespace
{
    Renderer renderer{};
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
        return Color(0,0,0);
    }

    dist = hInfo.z;
    ShadeInfo newShadeInfo{ *this };
    newShadeInfo.SetHit(ray, hInfo);
    newShadeInfo.IncrementBounce();
    return hInfo.node->GetMaterial()->Shade(newShadeInfo);
}

void threadRenderTiles()
{
    while (true)
    {
        int tileIndex{ tileThreads::tileCounter++ };
        if (tileIndex >= tileThreads::totalNumTiles) break;

        int imageX{ (tileIndex % tileThreads::numTilesX) * tileThreads::tileSize };
        int imageY{ (tileIndex / tileThreads::numTilesX) * tileThreads::tileSize };
        int tileWidth{ std::min(tileThreads::tileSize, renderer.GetCamera().imgWidth - imageX) };
        int tileHeight{ std::min(tileThreads::tileSize, renderer.GetCamera().imgHeight - imageY) };
        for (int j{ imageY }; j < imageY + tileHeight; ++j)
        {
            for (int i{ imageX }; i < imageX + tileWidth; ++i)
            {
                const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * (static_cast<float>(i) + 0.5f) };
                const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * (static_cast<float>(j) + 0.5f) };
                const Ray worldRay{ renderer.GetCamera().pos, (tileThreads::cameraToWorld * Vec3f{ spaceX, spaceY, -1.0f }) };

                HitInfo hitInfo{};
                if (renderer.TraceRay(worldRay, hitInfo))
                {
                    ShadeInfo sInfo{ renderer.GetScene().lights };
                    sInfo.SetHit(worldRay, hitInfo);
                    renderer.GetRenderImage().GetPixels()[j * renderer.GetCamera().imgWidth + i] = Color24{ hitInfo.node->GetMaterial()->Shade(sInfo) };
                }

                renderer.GetRenderImage().GetZBuffer()[j * renderer.GetCamera().imgWidth + i] = hitInfo.z;
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
    renderer.GetRenderImage().SaveImage("../image.png");

    ShowViewport(&renderer);
    return 0;
}
