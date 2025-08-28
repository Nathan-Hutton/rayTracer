#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );
void ShowViewport( RenderScene *scene );

bool shootRay(const Node* const node, const Ray& ray, HitInfo& bestHitInfo)
{
    const Object* const obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };

    bool hitObject{ false };
    if (obj != nullptr)
    {
        HitInfo hitInfo{};
        hitInfo.Init();
        if (obj->IntersectRay(localRay, hitInfo))
        {
            bestHitInfo.node = node;
            bestHitInfo.z = hitInfo.z;
            bestHitInfo.N = node->NormalTransformFrom(hitInfo.N);
            bestHitInfo.p = node->TransformFrom(hitInfo.p);
            hitObject = true;
        }
    }

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        HitInfo childHitInfo{};
        childHitInfo.Init();
        const Node* const childNode{ node->GetChild(i) };
        if (shootRay(childNode, localRay, childHitInfo))
        {
            hitObject = true;
            if (childHitInfo.z < bestHitInfo.z)
            {
                bestHitInfo.node = childNode;
                bestHitInfo.z = childHitInfo.z;
                bestHitInfo.N = node->NormalTransformFrom(childHitInfo.N);
                bestHitInfo.p = node->TransformFrom(childHitInfo.p);
            }
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
                pixels[j * scene.camera.imgWidth + i] = Color24{ hitInfo.node->GetMaterial()->Shade(worldRay, hitInfo, scene.lights) };
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
