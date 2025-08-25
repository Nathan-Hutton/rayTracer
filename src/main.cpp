#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );

bool shootRay(const Node* const node, const Ray& ray, HitInfo& bestHitInfo)
{
    const Object* obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };
    localRay.dir.Normalize();

    bool hitObject{ false };
    if (obj != nullptr)
    {
        HitInfo hitInfo{};
        hitInfo.Init();
        if (obj->IntersectRay(localRay, hitInfo))
        {
            const Vec3f hitPoint{ localRay.p + localRay.dir.GetNormalized() * hitInfo.z };
            const Vec3f hitPointInParentSpace{ node->TransformFrom(hitPoint) };
            const float parentSpaceHitPointDepth{ (hitPointInParentSpace - ray.p).Length() };

            bestHitInfo.node = node;
            bestHitInfo.z = parentSpaceHitPointDepth;
            hitObject = true;
        }
    }

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        HitInfo childHitInfo{};
        childHitInfo.Init();
        if (shootRay(node->GetChild(i), localRay, childHitInfo))
        {
            const Vec3 hitPoint{ localRay.p + localRay.dir.GetNormalized() * childHitInfo.z };
            const Vec3f hitPointInParentSpace{ node->TransformFrom(hitPoint) };
            const float parentSpaceHitPointDepth{ (hitPointInParentSpace - ray.p).Length() };

            if (parentSpaceHitPointDepth < bestHitInfo.z)
            {
                bestHitInfo.node = node->GetChild(i);
                bestHitInfo.z = parentSpaceHitPointDepth;
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

    scene.camera.dir = -(scene.camera.dir).GetNormalized();
    const Vec3f right{ scene.camera.up.Cross(scene.camera.dir).GetNormalized() };
    scene.camera.up = scene.camera.dir.Cross(right).GetNormalized();
    const Matrix3f cameraMat{ right, scene.camera.up, scene.camera.dir };

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
            Vec3f rayDir {
                x,
                y,
                -1.0f
            };
            const Ray worldRay{ scene.camera.pos, (cameraMat * rayDir).GetNormalized() };


            HitInfo hitInfo{};
            hitInfo.Init();
            if (shootRay(&scene.rootNode, worldRay, hitInfo))
                pixels[j * scene.camera.imgWidth + i] = Color24{ 255, 255, 255 };

            depthValues[j * scene.camera.imgWidth + i] = hitInfo.z;
        }
    }

    scene.renderImage.ComputeZBufferImage();
    scene.renderImage.SaveZImage("../zbuffer.png");
    scene.renderImage.SaveImage("../image.png");

    return 0;
}
