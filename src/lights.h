//-------------------------------------------------------------------------------
///
/// \file       lights.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    10.0
/// \date       September 24, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
//-------------------------------------------------------------------------------

#ifndef _LIGHTS_H_INCLUDED_
#define _LIGHTS_H_INCLUDED_

#include "renderer.h"

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
	bool IsAmbient() const override { return true; }
	void SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(intensity),ColorA(0.0f),Vec4f(0,0,0,1)); }
	void Load( Loader const &loader ) override;

    bool IntersectRay(const Ray& localRay, HitInfo& hitInfo, int hitSide) const override { return false; }
    bool IntersectShadowRay( Ray const &localRay, float t_max ) const override { return false; }
protected:
	Color intensity = Color(0,0,0);
};

//-------------------------------------------------------------------------------

class DirectLight : public GenLight
{
public:
	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override { dir=-direction; return intensity*sInfo.TraceShadowRay(-direction); }
	void SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(0.0f),ColorA(intensity),Vec4f(-direction,0.0f)); }
	void Load( Loader const &loader ) override;

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

	Color Illuminate( ShadeInfo const &sInfo, Vec3f &dir ) const override 
    { 
        Vec3f d=position-sInfo.P();
        dir=d.GetNormalized();
        return intensity*sInfo.TraceShadowRay(d,1); 
    }

	Color Radiance  ( ShadeInfo const &sInfo ) const override { return intensity; }
	bool IsRenderable() const override { return size > 0.0f; }
	void SetViewportLight( int lightID ) const override { SetViewportParam(lightID,ColorA(0.0f),ColorA(intensity),Vec4f(position,1.0f)); }
	void Load( Loader const &loader ) override;

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

    bool IntersectShadowRay( Ray const &ray, float t_max ) const override 
    {
        const float a{ ray.dir.Dot(ray.dir) };
        const float b{ 2 * ray.dir.Dot(ray.p) };
        const float c{ ray.p.Dot(ray.p) - 1.0f };

        const float discriminant{ b*b - 4*a*c };
        if (discriminant < 0.0f) return false;

        const float discriminantSquareRoot{ sqrtf(discriminant) };
        const float inverse2A{ 1.0f / (2.0f * a) };

        const float t1{ (-b - discriminantSquareRoot) * inverse2A };
        const float t2{ (-b + discriminantSquareRoot) * inverse2A };

        if (t1 >= 0.0f) return t1 < t_max;
        return t2 >= 0.0f && t2 < t_max;
    }

	Box  GetBoundBox() const override { return Box( position-size, position+size ); }	// empty box
	void ViewportDisplay( Material const *mtl ) const override;	// used for OpenGL display

protected:
	Color intensity = Color(0,0,0);
	Vec3f position  = Vec3f(0,0,0);
	float size      = 0.0f;
};

//-------------------------------------------------------------------------------

#endif
