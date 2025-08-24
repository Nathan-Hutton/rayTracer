#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );

//struct HitInfo
//{
//	float       z;		// the distance from the ray center to the hit point
//	Node const *node;	// the object node that was hit
//	bool        front;	// true if the ray hits the front side, false if the ray hits the back side
//
//	HitInfo() { Init(); }
//	void Init() { z=BIGFLOAT; node=nullptr; front=true; }
//};

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
    // Shoot out one ray per pixel. For each object (sphere), call IntersectRay
    // If no hit, return false.
    // Else, fill in hitInfo with z, Node*, and front (bool)
    // Test hitInfo.z againt what's currently in zbuffer, replace if less
    // * The hitSide bullshit is just a flag saying that we will only count intersections that hit the front. It's a filter

    RenderScene scene{};
    LoadScene(scene, "../scene.xml");

    //scene.camera.up.Normalize();
    //scene.camera.dir.Normalize();
    scene.camera.dir = -(scene.camera.dir).GetNormalized();
    const Vec3f right{ scene.camera.up.Cross(scene.camera.dir).GetNormalized() };
    scene.camera.up = scene.camera.dir.Cross(right).GetNormalized();
    const Matrix3f cameraMat{ right, scene.camera.up, scene.camera.dir };
    //Transformation cameraTransform{};
    //cameraTransform.Transform(cameraMat);
    //cameraTransform.Translate(scene.camera.pos);

    constexpr float Pi = 3.14159265358979323846f;
    const float imagePlaneHeight{ 2 * tanf((static_cast<float>(scene.camera.fov) * Pi / 180.0f) / 2.0f) };

    const float aspectRatio{ static_cast<float>(scene.camera.imgWidth) / static_cast<float>(scene.camera.imgHeight) };
    const float imagePlaneWidth{ aspectRatio * imagePlaneHeight };
    const float pixelSize{ imagePlaneWidth / static_cast<float>(scene.camera.imgWidth) };

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

            for (int nodeIndex{ 0 }; nodeIndex < scene.rootNode.GetNumChild(); ++nodeIndex)
                scene.renderImage.GetZBuffer()[j * scene.camera.imgWidth + i] = getClosestCollision(scene.rootNode.GetChild(nodeIndex), worldRay);
        }
        break;
    }

    scene.renderImage.ComputeZBufferImage();
    scene.renderImage.SaveZImage("../image.png");

    return 0;
}
