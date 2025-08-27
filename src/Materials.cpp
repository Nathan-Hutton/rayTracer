#include "Materials.h"

Color MtlPhong::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    return Color();
}

Color MtlBlinn::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const
{
    return Color();
}

Color MtlGGX::Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const 
{
    return Color();
}
