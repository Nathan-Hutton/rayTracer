#include "scene.h"
#include "cyCore/cyVector.h"
#include "cyCore/cyMatrix.h"

#include <iostream>
#include <cmath>

int LoadScene( RenderScene &scene, char const *filename );

//class Camera
//{
//public:
//	Vec3f pos, dir, up;
//	float fov;
//	int imgWidth, imgHeight;
//
//	void Init()
//	{
//		pos.Set(0,0,0);
//		dir.Set(0,0,-1);
//		up.Set(0,1,0);
//		fov = 40;
//		imgWidth = 1920;
//		imgHeight = 1080;
//	}
//};

int main()
{
    // Contains a Camera, RenderImage, and the rootNode
    // The rootNode contains all the other nodes. Each node lets us access an object. The rootNode is just the sceneNode though.
    // The renderImage is how we actually make the PNG and contains depth buffer info.
    
    // We make the zbufferImg by first filling in the zbuffer. Then, once we call ComputeZBufferImg, it makes a normalized zbuffer image.
    // The person who turned in the assignment where the spheres looked cooler must have just turned the zbuffer directly 
    // instead of first calling ComputeZBufferImg then saving those contents to a PNG.
    // We fill in the zbuffer by just calling GetZBuffer() and modifying it directly.

    // Camera has a pos, up, and dir
    // For this scene, the dir {0, 20, 0}

    // Shoot out one ray per pixel. For each object (sphere), call IntersectRay
    // If no hit, return false.
    // Else, fill in hitInfo with z, Node*, and front (bool)
    // Test hitInfo.z againt what's currently in zbuffer, replace if less
    // * The hitSide bullshit is just a flag saying that we will only count intersections that hit the front. It's a filter
    // To test if it's the front or back side, compute surface normal at that point. If dot(rayDir, normal) > 0, it's back. Otherwise front.

    // For completeness, have code to handle all hitSide conditions
    RenderScene scene{};
    LoadScene(scene, "../scene.xml");

    scene.camera.up.Normalize();
    scene.camera.dir.Normalize();
    const Vec3f right{ scene.camera.up.Cross(scene.camera.dir).GetNormalized() };
    const Matrix3f cameraMat{ right, scene.camera.up, -scene.camera.dir };
    Transformation cameraTransform{};
    cameraTransform.Transform(cameraMat);
    cameraTransform.Translate(scene.camera.pos);

    constexpr float Pi = 3.14159265358979323846f;
    const float imagePlaneHeight{ 2 * tanf((static_cast<float>(scene.camera.fov) * Pi / 180.0f) / 2.0f) };

    const float aspectRatio{ static_cast<float>(scene.camera.imgWidth) / static_cast<float>(scene.camera.imgHeight) };
    const float imagePlaneWidth{ aspectRatio * imagePlaneHeight };
    const float pixelSize{ imagePlaneWidth / static_cast<float>(scene.camera.imgWidth) };

    // Just move the top left corner half a pixel down and to the right I guess. So all pixels will be shifted by this much?
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

            // Next, traverse tree. Get ray dir and pos in world space
        }
    }

    return 0;
}
