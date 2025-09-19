#include "scene.h"
#include "lights.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <atomic>
#include <chrono>

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

int main()
{
    RenderScene scene{};
    LoadScene(scene, "../assets/scene.xml");
    lightsGlobalVars::rootNode = &scene.rootNode;

    const int tileSize{ 16 };
    const int numTilesX{ (scene.camera.imgWidth + tileSize - 1) / tileSize };
    const int numTilesY{ (scene.camera.imgHeight + tileSize - 1) / tileSize };
    const int totalNumTiles{ numTilesX * numTilesY };
    std::atomic<int> tileCounter{ 0 };

    const Vec3 camZ{ -scene.camera.dir.GetNormalized() };
    const Vec3f camX{ scene.camera.up.Cross(camZ).GetNormalized() };
    const Vec3f camY{ camZ.Cross(camX) };
    const Matrix3f cameraToWorld{ camX, camY, camZ };

    constexpr float Pi = 3.14159265358979323846f;
    const float imagePlaneHalfHeight{ tanf((static_cast<float>(scene.camera.fov) * Pi / 180.0f) / 2.0f) };

    const float aspectRatio{ static_cast<float>(scene.camera.imgWidth) / static_cast<float>(scene.camera.imgHeight) };
    const float imagePlaneHalfWidth{ aspectRatio * imagePlaneHalfHeight };
    const float pixelSize{ (imagePlaneHalfWidth * 2.0f) / static_cast<float>(scene.camera.imgWidth) };

    Color24* pixels{ scene.renderImage.GetPixels() };
    float* depthValues{ scene.renderImage.GetZBuffer() };
    const auto start{ std::chrono::high_resolution_clock::now() };
    for (int j{ 0 }; j < scene.camera.imgWidth; ++j)
    {
        for (int i{ 0 }; i < scene.camera.imgHeight; ++i)
        {
            const float x{ -imagePlaneHalfWidth + pixelSize * (static_cast<float>(j) + 0.5f) };
            const float y{ imagePlaneHalfHeight - pixelSize * (static_cast<float>(i) + 0.5f) };
            const Ray worldRay{ scene.camera.pos, (cameraToWorld * Vec3f{ x, y, -1.0f }) };

            HitInfo hitInfo{};
            if (shootRay(&scene.rootNode, worldRay, hitInfo))
                pixels[i * scene.camera.imgWidth + j] = Color24{ hitInfo.node->GetMaterial()->Shade(worldRay, hitInfo, scene.lights, 5) };

            depthValues[i * scene.camera.imgWidth + j] = hitInfo.z;
        }
    }
    const auto end{ std::chrono::high_resolution_clock::now() };
    const auto durationMilli{ std::chrono::duration_cast<std::chrono::milliseconds>(end - start) };
    const auto durationSeconds{ std::chrono::duration_cast<std::chrono::seconds>(end - start) };
    std::cout << "\nTime: " << durationSeconds << " : " << durationMilli % 1000 << '\n';

    scene.renderImage.ComputeZBufferImage();
    scene.renderImage.SaveZImage("../zbuffer.png");
    scene.renderImage.SaveImage("../image.png");

    ShowViewport(&scene);
    return 0;
}
