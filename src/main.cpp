#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );

bool shootRay(const Node* const node, const Ray& ray, HitInfo& hitInfo)
{
    const Object* obj{ node->GetNodeObj() };
    Ray localRay{ node->ToNodeCoords(ray) };
    localRay.dir.Normalize();

    if (obj != nullptr)
    {
        if (obj->IntersectRay(localRay, hitInfo))
        {
            hitInfo.node = node;
            return true;
        }
    }

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        if (shootRay(node->GetChild(i), localRay, hitInfo))
        {
            hitInfo.node = node->GetChild(i);
            return true;
        }
    }

    return false;
}

float getClosestCollision(const Node* const node, const Ray& ray)
{
    HitInfo hitInfo{};
    hitInfo.Init();
    hitInfo.node = node;

    Ray localRay{ node->ToNodeCoords(ray) };
    localRay.dir.Normalize();

    node->GetNodeObj()->IntersectRay(localRay, hitInfo);
    float closestDepth{ hitInfo.z };

    for (int i{ 0 }; i < node->GetNumChild(); ++i)
    {
        const float closestChildCollisionDepth{ getClosestCollision(node->GetChild(i), localRay) };
        closestDepth = fmin(closestDepth, closestChildCollisionDepth);
    }

    return closestDepth;
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
            //const Ray ray{ Vec3f{ 0.0f }, rayDir.GetNormalized() };
            const Ray worldRay{ scene.camera.pos, (cameraMat * rayDir).GetNormalized() };


            HitInfo hitInfo{};
            hitInfo.Init();
            if (shootRay(&scene.rootNode, worldRay, hitInfo))
            {
                pixels[j * scene.camera.imgWidth + i] = Color24{ 255, 255, 255 };
            }
            //float closestDepth{ BIGFLOAT };
            //for (int nodeIndex{ 0 }; nodeIndex < scene.rootNode.GetNumChild(); ++nodeIndex)
                //closestDepth = fmin(closestDepth, getClosestCollision(scene.rootNode.GetChild(nodeIndex), worldRay));
                //shootRay();
            //scene.renderImage.GetZBuffer()[j * scene.camera.imgWidth + i] = closestDepth;
        }
    }

    //scene.renderImage.ComputeZBufferImage();
    //scene.renderImage.SaveZImage("../image.png");
    scene.renderImage.SaveImage("../image.png");

    return 0;
}
