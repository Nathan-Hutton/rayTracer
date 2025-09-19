#include "scene.h"
#include "lights.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

int LoadScene( RenderScene &scene, char const *filename );
void ShowViewport( RenderScene *scene );

Node* lightsGlobalVars::rootNode{ nullptr };

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

    RenderScene scene{};
    float imagePlaneHalfWidth{};
    float imagePlaneHalfHeight{};
    float pixelSize{};
    Matrix3f cameraToWorld{};

    Color24* pixels{ nullptr };
    float* depthValues{ nullptr };
}

void threadRenderTiles()
{
    while (true)
    {
        int tileIndex{ tileThreads::tileCounter++ };
        if (tileIndex >= tileThreads::totalNumTiles) break;

        int imageX{ (tileIndex % tileThreads::numTilesX) * tileThreads::tileSize };
        int imageY{ (tileIndex / tileThreads::numTilesX) * tileThreads::tileSize };
        int tileWidth{ std::min(tileThreads::tileSize, tileThreads::scene.camera.imgWidth - imageX) };
        int tileHeight{ std::min(tileThreads::tileSize, tileThreads::scene.camera.imgHeight - imageY) };
        for (int j{ imageY }; j < imageY + tileHeight; ++j)
        {
            for (int i{ imageX }; i < imageX + tileWidth; ++i)
            {
                const float spaceX{ -tileThreads::imagePlaneHalfWidth + tileThreads::pixelSize * (static_cast<float>(i) + 0.5f) };
                const float spaceY{ tileThreads::imagePlaneHalfHeight - tileThreads::pixelSize * (static_cast<float>(j) + 0.5f) };
                const Ray worldRay{ tileThreads::scene.camera.pos, (tileThreads::cameraToWorld * Vec3f{ spaceX, spaceY, -1.0f }) };

                HitInfo hitInfo{};
                if (shootRay(&tileThreads::scene.rootNode, worldRay, hitInfo))
                    tileThreads::scene.renderImage.GetPixels()[j * tileThreads::scene.camera.imgWidth + i] = Color24{ hitInfo.node->GetMaterial()->Shade(worldRay, hitInfo, tileThreads::scene.lights, 5) };

                tileThreads::scene.renderImage.GetZBuffer()[j * tileThreads::scene.camera.imgWidth + i] = hitInfo.z;
            }
        }
    }
}

int main()
{
    LoadScene(tileThreads::scene, "../assets/scene.xml");
    lightsGlobalVars::rootNode = &tileThreads::scene.rootNode;

    tileThreads::numTilesX = (tileThreads::scene.camera.imgWidth + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::numTilesY = (tileThreads::scene.camera.imgHeight + tileThreads::tileSize - 1) / tileThreads::tileSize;
    tileThreads::totalNumTiles = tileThreads::numTilesX * tileThreads::numTilesY;
    tileThreads::tileCounter = 0;

    const Vec3 camZ{ -tileThreads::scene.camera.dir.GetNormalized() };
    const Vec3f camX{ tileThreads::scene.camera.up.Cross(camZ).GetNormalized() };
    const Vec3f camY{ camZ.Cross(camX) };
    tileThreads::cameraToWorld = Matrix3f{ camX, camY, camZ };

    constexpr float Pi = 3.14159265358979323846f;
    tileThreads::imagePlaneHalfHeight = tanf((static_cast<float>(tileThreads::scene.camera.fov) * Pi / 180.0f) / 2.0f);

    const float aspectRatio{ static_cast<float>(tileThreads::scene.camera.imgWidth) / static_cast<float>(tileThreads::scene.camera.imgHeight) };
    tileThreads::imagePlaneHalfWidth = aspectRatio * tileThreads::imagePlaneHalfHeight;
    tileThreads::pixelSize = (tileThreads::imagePlaneHalfWidth * 2.0f) / static_cast<float>(tileThreads::scene.camera.imgWidth);

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

    tileThreads::scene.renderImage.ComputeZBufferImage();
    tileThreads::scene.renderImage.SaveZImage("../zbuffer.png");
    tileThreads::scene.renderImage.SaveImage("../image.png");

    ShowViewport(&tileThreads::scene);
    return 0;
}
