#include "scene.h"

#include <iostream>

int LoadScene( RenderScene &scene, char const *filename );

int main()
{
    RenderScene scene{};
    LoadScene(scene, "../scene.xml");
}
