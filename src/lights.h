//-------------------------------------------------------------------------------
///
/// \file       lights.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    12.0
/// \date       October 25, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
//-------------------------------------------------------------------------------

#ifndef _LIGHTS_H_INCLUDED_
#define _LIGHTS_H_INCLUDED_

#include "renderer.h"
#include <iostream>

//-------------------------------------------------------------------------------

class GenLight : public Light
{
protected:
	void SetViewportParam( int lightID, ColorA const &ambient, ColorA const &intensity, Vec4f const &pos ) const;
};

//-------------------------------------------------------------------------------

class AmbientLight : public GenLight
{
public:
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override { return intensity; }
	Color Intensity() const override { return intensity; }
	bool  IsAmbient() const override { return true; }
	void  SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(intensity),ColorA(0.0f),Vec4f(0,0,0,1)); }
	void  Load( Loader const &loader ) override;

    bool IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const override { return false; }
    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }
protected:
	Color intensity = Color(0,0,0);
};

//-------------------------------------------------------------------------------

class DirectLight : public GenLight
{
public:
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override { dir=-direction; return intensity * sInfo.TraceShadowRay(-direction); }
	Color Intensity() const override { return intensity; }
	void  SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(0.0f),ColorA(intensity),Vec4f(-direction,0.0f)); }
	void  Load( Loader const &loader ) override;

    bool IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const override { return false; }
    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }
protected:
	Color intensity = Color(0,0,0);
	Vec3f direction = Vec3f(0,0,0);
};

//-------------------------------------------------------------------------------

class PointLight : public GenLight
{
private:
    HaltonSeq<static_cast<int>(16)> diskHaltonSeqTheta{ 5 };
    HaltonSeq<static_cast<int>(16)> diskHaltonSeqRadius{ 7 };

public:
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override 
    { 
        Vec3f d{ position - sInfo.P() };
        const float distSquared{ d.LengthSquared() };
        dir = d.GetNormalized();

        Vec3f u;
        Vec3f v;
        dir.GetOrthonormals(u, v);

        const float offsetX{ sInfo.RandomFloat() };
        const float offsetY{ sInfo.RandomFloat() };

        constexpr size_t minNumSamples{ 4 };
        constexpr size_t maxNumSamples{ 64 };
        size_t numHits{ 0 };
        size_t numSamples{ 0 };
        for (size_t i{ 0 }; i < maxNumSamples; ++i)
        {
            const float jitterTheta{ fmod(offsetX + diskHaltonSeqTheta[i], 1.0f) };
            const float jitterRadius{ fmod(offsetY + diskHaltonSeqRadius[i], 1.0f) };

            const float diskTheta{ jitterTheta * 2.0f * M_PI };
            const float diskRadius{ sqrt(jitterRadius) };

            const float x{ diskRadius * cos(diskTheta) };
            const float y{ diskRadius * sin(diskTheta) };
            const Vec3f destination{ position + ((x * size) * u) + (y * size) * v };

            const Vec3f sampleRayDir{ destination - sInfo.P() };

            if (sInfo.TraceShadowRay(sampleRayDir, 1.0f) == 1.0f)
                ++numHits;

            if (i + 1 == minNumSamples && (numHits == 0 || numHits == minNumSamples))
                return (intensity * static_cast<float>(numHits) / minNumSamples) / distSquared;
        }

        return (intensity * static_cast<float>(numHits) / maxNumSamples) / distSquared;
    }
	Color Radiance( SamplerInfo const &sInfo ) const override { return intensity / (Pi<float>()*size*size); }
	Color Intensity     () const override { return intensity; }
	bool  IsRenderable  () const override { return size > 0.0f; }
	bool  IsPhotonSource() const override { return true; }

	void  RandomPhoton( RNG &rng, Ray &r, Color &c ) const override
    {
        c = intensity * 4.0f * M_PI * size * size;

        // Ray pos
        float randU{ rng.RandomFloat() };
        float randV{ rng.RandomFloat() };
        const float theta{ 2.0f * M_PI * randU };
        const float posZ{ size * (1.0f - 2.0f * randV) };
        const float rProj{ sqrtf(size * size - posZ * posZ) };
        const float posX{ rProj * cos(theta) };
        const float posY{ rProj * sin(theta) };
        const Vec3f rayPos{ Vec3f{ posX, posY, posZ } + position };
        r.p = rayPos;

        // Ray dir
        randU = rng.RandomFloat();
        randV = rng.RandomFloat();
        const float phi{ 2.0f * M_PI * randU };
        const float cosTheta{ randV };
        const float sinTheta{ sqrtf(1.0f - cosTheta * cosTheta) };

        const float dirX{ sinTheta * cos(phi) };
        const float dirY{ sinTheta * sin(phi) };
        const float dirZ{ cosTheta };

        const Vec3f norm{ (rayPos - position).GetNormalized() };
        Vec3f u, v;
        norm.GetOrthonormals(u, v);
        const Vec3f dir{ (dirX * u) + (dirY * v) + (dirZ * norm) };

        r.dir = dir;
    }

	void  SetViewportLight( int lightID ) const override;
	void  Load( Loader const &loader ) override;

	bool IntersectRay( Ray const &ray, HitInfo &hInfo, int hitSide=HIT_FRONT ) const override 
    {
        const Ray localRay{ (ray.p - position) / size, ray.dir / size };
        const float a{ localRay.dir.Dot(localRay.dir) };
        const float b{ 2 * localRay.dir.Dot(localRay.p) };
        const float c{ localRay.p.Dot(localRay.p) - 1.0f };

        const float discriminant{ b*b - 4*a*c };
        if (discriminant < 0.0f) return hitSide == HIT_NONE;

        const float discriminantSquareRoot{ sqrtf(discriminant) };
        const float inverse2A{ 1.0f / (2.0f * a) };
        const float t1{ (-b - discriminantSquareRoot) * inverse2A };

        if (hitSide == HIT_FRONT)
        {
            if (t1 <= 0.0f)
                return false;

            hInfo.z = t1;
            hInfo.p = localRay.p + localRay.dir * t1;
            hInfo.N = hInfo.p;
            hInfo.front = true;
            hInfo.light = true;

            return true;

        }

        const float t2{ (-b + discriminantSquareRoot) * inverse2A };
        if (hitSide == HIT_BACK)
        {
            if (t1 > 0.0f || t2 < 0.0f)
                return false;

            hInfo.z = t2;
            hInfo.p = localRay.p + localRay.dir * t2;
            hInfo.N = hInfo.p;
            hInfo.front = false;
            hInfo.light = true;

            return true;
        }


        if (hitSide == HIT_FRONT_AND_BACK)
        {
            if (t1 < 0.0f && t2 < 0.0f)
                return false;

            if (t1 > 0.0f)
            {
                hInfo.z = t1;
                hInfo.p = localRay.p + localRay.dir * t1;
                hInfo.N = hInfo.p;

                hInfo.front = true;
            }
            else
            {
                hInfo.z = t2;
                hInfo.p = localRay.p + localRay.dir * t2;
                hInfo.N = hInfo.p;

                hInfo.front = false;
            }

            hInfo.light = true;
            return true;
        }

        return false;
    }

    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }

	Box  GetBoundBox() const override { return Box( position-size, position+size ); }
	void ViewportDisplay( Material const *mtl ) const override;	// used for OpenGL display

protected:
	Color intensity   = Color(0,0,0);
	Vec3f position    = Vec3f(0,0,0);
	float size        = 0.0f;
	float attenuation = 0.0f;	// Zero means no attenuation. If non-zero, light distance is scaled by attenuation.
};

//-------------------------------------------------------------------------------

#endif
