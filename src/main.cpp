#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );
void ShowViewport( RenderScene *scene );

Color shade(const Ray& worldRay, const HitInfo& hitInfo, const LightList& lightList)
{
    Color finalColor{};

    const Node* node{ hitInfo.node };
    const Vec3f worldSpaceHitPoint{ node->TransformFrom(hitInfo.p) };
    const Vec3f worldSpaceNormal{ node->NormalTransformFrom(hitInfo.N) };

    for (const Light* const light : lightList)
    {
        if (light->IsAmbient())
        {
            finalColor += Color{ light->Illuminate(worldSpaceHitPoint, worldSpaceNormal) };
            continue;
        }
    }

    return finalColor;
}

bool shootRay(const Node* const node, const Ray& ray, HitInfo& bestHitInfo)
{
    const Object* obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };

    bool hitObject{ false };
    if (obj != nullptr)
    {
        HitInfo hitInfo{};
        hitInfo.Init();
        if (obj->IntersectRay(localRay, hitInfo))
        {
            bestHitInfo = hitInfo;
            bestHitInfo.node = node;
            hitObject = true;
        }
    }

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        HitInfo childHitInfo{};
        childHitInfo.Init();
        if (shootRay(node->GetChild(i), localRay, childHitInfo))
        {
            if (childHitInfo.z < bestHitInfo.z)
            {
                bestHitInfo = childHitInfo;
                bestHitInfo.node = node->GetChild(i);
            }
            hitObject = true;
        }
    }

    return hitObject;
}

int main()
{
    RenderScene scene{};
    LoadScene(scene, "../scene.xml");

    const Vec3 camZ{ -scene.camera.dir.GetNormalized() };
    const Vec3f camX{ scene.camera.up.Cross(camZ).GetNormalized() };
    const Vec3f camY{ camZ.Cross(camX) };
    const Matrix3f cameraToWorld{ camX, camY, camZ };

    constexpr float Pi = 3.14159265358979323846f;
    const float imagePlaneHeight{ 2 * tanf((static_cast<float>(scene.camera.fov) * Pi / 180.0f) / 2.0f) };

    const float aspectRatio{ static_cast<float>(scene.camera.imgWidth) / static_cast<float>(scene.camera.imgHeight) };
    const float imagePlaneWidth{ aspectRatio * imagePlaneHeight };
    const float pixelSize{ imagePlaneWidth / static_cast<float>(scene.camera.imgWidth) };

    Color24* pixels{ scene.renderImage.GetPixels() };
    float* depthValues{ scene.renderImage.GetZBuffer() };
    for (int i{ 0 }; i < scene.camera.imgWidth; ++i)
    {
        for (int j{ 0 }; j < scene.camera.imgHeight; ++j)
        {
            const float x{ -imagePlaneWidth * 0.5f + pixelSize * (static_cast<float>(i) + 0.5f) };
            const float y{ imagePlaneHeight * 0.5f - pixelSize * (static_cast<float>(j) + 0.5f) };
            const Ray worldRay{ scene.camera.pos, (cameraToWorld * Vec3f{ x, y, -1.0f }) };


            HitInfo hitInfo{};
            hitInfo.Init();
            if (shootRay(&scene.rootNode, worldRay, hitInfo))
            {
                pixels[j * scene.camera.imgWidth + i] = Color24{ shade(worldRay, hitInfo, scene.lights) };
                //pixels[j * scene.camera.imgWidth + i] = Color24{ 255, 255, 255 };
            }

            depthValues[j * scene.camera.imgWidth + i] = hitInfo.z;
        }
    }

    scene.renderImage.ComputeZBufferImage();
    scene.renderImage.SaveZImage("../zbuffer.png");
    scene.renderImage.SaveImage("../image.png");

    ShowViewport(&scene);
    return 0;
}
