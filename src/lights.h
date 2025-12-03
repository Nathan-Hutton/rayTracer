//-------------------------------------------------------------------------------
///
/// \file       lights.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    13.0
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
#ifdef LEGACY_SHADING_API
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override { return intensity; }
#endif
	Color Intensity() const override { return intensity; }
	bool  IsAmbient() const override { return true; }
	void  SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(intensity),ColorA(0.0f),Vec4f(0,0,0,1)); }
	void  Load( Loader const &loader ) override;
    float GetSize() const override { return 0.0f; };

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f &dir, Info &si ) const override
	{
		si.prob=1; si.mult=intensity; si.dist=0; dir=sInfo.N(); si.lobe=DirSampler::Lobe::ALL; return true;
	}

    bool IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const override { return false; }
    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }

protected:
	Color intensity = Color(0,0,0);
};

//-------------------------------------------------------------------------------

class DirectLight : public GenLight
{
public:
#ifdef LEGACY_SHADING_API
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override { dir=-direction; return intensity * sInfo.TraceShadowRay(-direction); }
#endif
	Color Intensity() const override { return intensity; }
	void  SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(0.0f),ColorA(intensity),Vec4f(-direction,0.0f)); }
	void  Load( Loader const &loader ) override;
    float GetSize() const override { return 0.0f; };

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f &dir, Info &si ) const override
	{
		si.prob=1; si.mult=intensity; si.dist=BIGFLOAT; dir=-direction; si.lobe=DirSampler::Lobe::ALL; return true;
	}

    bool IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const override { return false; }
    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }

protected:
	Color intensity = Color(0,0,0);
	Vec3f direction = Vec3f(0,0,0);
};

//-------------------------------------------------------------------------------

class PointLight : public GenLight
{
public:
#ifdef LEGACY_SHADING_API
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override;
#endif
	Color Radiance( SamplerInfo const &sInfo ) const override { return intensity / (Pi<float>()*size*size); }
	Color Intensity     () const override { return intensity; }
	bool  IsRenderable  () const override { return size > 0.0f; }
	bool  IsPhotonSource() const override { return true; }
    float GetSize() const override { return size; };

	void  RandomPhoton( RNG &rng, Ray &r, Color &c ) const override
    {
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

        c = intensity * 8.0f * M_PI * size * size * cosTheta;
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

	//bool GenerateSample( SamplerInfo const &sInfo, Vec3f       &dir, Info &si ) const override 
    //{
    //    //const float r1{ sInfo.RandomFloat() };
    //    //const float r2{ sInfo.RandomFloat() };
    //    //const float theta{ acosf(sqrtf(r1)) * r2 };
    //    //const float phi{ 2.0f * static_cast<float>(M_PI) * r2 };
    //    //const Vec3f localSample{ sinf(theta) * cosf(phi), cosf(theta), sinf(theta) * sinf(phi) };
    //    
    //    const float z{ sInfo.RandomFloat() };
    //    const float sinTheta{ sqrtf(1.0f - z * z) };
    //    const float phi{ 2.0f * M_PI * sInfo.RandomFloat() };
    //    const float x{ sinTheta * cos(phi) };
    //    const float y{ sinTheta * sin(phi) };

    //    const Vec3f dirToMatPoint{ (sInfo.P() - position).GetNormalized() };
    //    Vec3f u, v;
    //    dirToMatPoint.GetOrthonormals(u, v);
    //    const Vec3f sample{ position + (((x * u) + (y * v) + (z * dirToMatPoint)) * size) };

    //    //const float theta{ 2.0f * M_PI * r1 };
    //    //const float posZ{ size * (1.0f - 2.0f * r2) };
    //    //const float rProj{ sqrtf(size * size - posZ * posZ) };
    //    //const float posX{ rProj * cos(theta) };
    //    //const float posY{ rProj * sin(theta) };
    //    //const Vec3f pos{ Vec3f{ posX, posY, posZ } + position };

    //    //dir = pos - sInfo.P();
    //    dir = sample - sInfo.P();
    //    si.dist = dir.Length();
    //    dir.Normalize();

    //    si.norm = (sample - position).GetNormalized();

    //    //const Vec3f norm{ (sample - position).GetNormalized() };
    //    //const float cosThetaLight{ norm.Dot(-dir) };
    //    //if (cosThetaLight <= 0.0f)
    //        //return false;

    //    //intensity / (Pi<float>()*size*size)
    //    si.mult = intensity * (static_cast<float>(M_PI) / (si.dist * si.dist));
    //    //si.mult = intensity * (static_cast<float>(M_PI) / (si.dist * si.dist));
    //    //si.mult = intensity / (si.dist * si.dist);
    //    //si.mult = Radiance(sInfo) * (Pi<float>() / (si.dist * si.dist));
    //    //si.mult = Radiance(sInfo);
    //    //si.mult = intensity / (si.dist * si.dist);
    //    //si.mult = Radiance(sInfo);
    //    si.prob = 1.0f / (2.0f * Pi<float>() * size * size);

    //    return true;
    //}
	bool GenerateSample( SamplerInfo const &sInfo, Vec3f       &dir, Info &si ) const override 
    {
        Vec3f dirMatToCenter{ position - sInfo.P() };
        const float distFromCenterToMat{ dirMatToCenter.Length() };
        dirMatToCenter.Normalize();

        const float sinThetaMax{ size / distFromCenterToMat };
        const float cosThetaMax{ sqrtf(1.0f - (sinThetaMax * sinThetaMax)) };

        const float r1{ sInfo.RandomFloat() };
        const float cosTheta{ 1.0f - r1 + r1 * cosThetaMax };
        const float sinTheta{ sqrtf(1.0f - (cosTheta * cosTheta)) };
        const float phi{ 2.0f * Pi<float>() * sInfo.RandomFloat() };

        const float x{ sinTheta * cosf(phi) };
        const float y{ sinTheta * sinf(phi) };
        const float z{ cosTheta };

        Vec3f u, v;
        dirMatToCenter.GetOrthonormals(u, v);

        const float hypotenuse{ distFromCenterToMat };
        const float adjacent{ hypotenuse * cosTheta };
        const float oppositeSquared{ (hypotenuse * hypotenuse) - (adjacent * adjacent) };
        const float distSquaredInside{ (size * size) - oppositeSquared };
        const float tOffset{ sqrtf(std::max(0.0f, distSquaredInside)) };
        si.dist = adjacent - tOffset;

        dir = (x * u) + (y * v) + (z * dirMatToCenter);
        si.mult = Radiance(sInfo);
        //si.mult = intensity;
        const float oneMinusCosThetaMax = (sinThetaMax * sinThetaMax) / (1.0f + cosThetaMax);
        si.prob = 1.0f / (2.0f * Pi<float>() * oneMinusCosThetaMax);

        return true;
    }
	void GetSampleInfo ( SamplerInfo const &sInfo, Vec3f const &dir, Info &si ) const override {}

protected:
	Color intensity   = Color(0,0,0);
	Vec3f position    = Vec3f(0,0,0);
	float size        = 0.0f;
	float attenuation = 0.0f;	// Zero means no attenuation. If non-zero, light distance is scaled by attenuation.
};

//-------------------------------------------------------------------------------

#endif
