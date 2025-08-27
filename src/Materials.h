//-------------------------------------------------------------------------------
///
/// \file       materials.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    2.0
/// \date       August 23, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
//-------------------------------------------------------------------------------
 
#ifndef _MATERIALS_H_INCLUDED_
#define _MATERIALS_H_INCLUDED_
 
#include "scene.h"
 
//-------------------------------------------------------------------------------
 
class MtlBasePhongBlinn : public Material
{
public:
    MtlBasePhongBlinn() : diffuse(0.5f,0.5f,0.5f), specular(0.7f,0.7f,0.7f), glossiness(20.0f) {}
 
    void SetDiffuse   ( Color const &d ) { diffuse    = d; }
    void SetSpecular  ( Color const &s ) { specular   = s; }
    void SetGlossiness( float        g ) { glossiness = g; }
 
    const Color& Diffuse   () const { return diffuse;    }
    const Color& Specular  () const { return specular;   }
    float        Glossiness() const { return glossiness; }
 
protected:
    Color diffuse, specular;
    float glossiness;
};
 
//-------------------------------------------------------------------------------
 
class MtlPhong : public MtlBasePhongBlinn
{
public:
    Color Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const override;
    void SetViewportMaterial(int subMtlID=0) const override;    // used for OpenGL display
};
 
//-------------------------------------------------------------------------------
 
class MtlBlinn : public MtlBasePhongBlinn
{
public:
    Color Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const override;
    void SetViewportMaterial(int subMtlID=0) const override;    // used for OpenGL display
};
 
//-------------------------------------------------------------------------------
 
class MtlGGX : public Material
{
public:
    MtlGGX() : baseColor(0.5f,0.5f,0.5f), roughness(1.0f), metallic(0.0f), reflectance(0.5f) {}
 
    void SetBaseColor  ( Color const &c ) { baseColor   = c; }
    void SetRoughness  ( float        r ) { roughness   = r; }
    void SetMetallic   ( float        m ) { metallic    = m; }
    void SetReflectance( float        r ) { reflectance = r; }
 
    Color Shade(Ray const &ray, HitInfo const &hInfo, LightList const &lights) const override;
    void SetViewportMaterial(int subMtlID=0) const override;    // used for OpenGL display
 
private:
    Color baseColor;    // albedo for dielectrics, F0 for metals
    float roughness;
    float metallic;
    float reflectance;  // Fresnel reflectance for dielectrics
};
 
//-------------------------------------------------------------------------------
 
#endif

