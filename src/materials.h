//-------------------------------------------------------------------------------
///
/// \file       materials.h 
/// \author     Cem Yuksel (www.cemyuksel.com)
/// \version    13.0
/// \date       October 25, 2025
///
/// \brief Example source for CS 6620 - University of Utah.
///
//-------------------------------------------------------------------------------

#ifndef _MATERIALS_H_INCLUDED_
#define _MATERIALS_H_INCLUDED_

#include "renderer.h"

//-------------------------------------------------------------------------------

class MtlBasePhongBlinn : public Material
{
public:
	void Load( Loader const &loader, TextureFileList &tfl ) override;

	void SetDiffuse   ( Color const &d ) { diffuse   .SetValue(d); }
	void SetSpecular  ( Color const &s ) { specular  .SetValue(s); }
	void SetGlossiness( float        g ) { glossiness.SetValue(g); }
	void SetEmission  ( Color const &e ) { emission  .SetValue(e); }
	void SetReflection( Color const &r ) { reflection.SetValue(r); }
	void SetRefraction( Color const &r ) { refraction.SetValue(r); }
	void SetAbsorption( Color const &a ) { absorption = a; }
	void SetIOR       ( float        i ) { ior        = i; }

	void SetDiffuseTexture   ( TextureMap *tex ) { diffuse   .SetTexture(tex); }
	void SetSpecularTexture	 ( TextureMap *tex ) { specular  .SetTexture(tex); }
	void SetGlossinessTexture( TextureMap *tex ) { glossiness.SetTexture(tex); }
	void SetEmissionTexture  ( TextureMap *tex ) { emission  .SetTexture(tex); }
	void SetReflectionTexture( TextureMap *tex ) { reflection.SetTexture(tex); }
	void SetRefractionTexture( TextureMap *tex ) { refraction.SetTexture(tex); }

	const TexturedColor& Diffuse   () const { return diffuse;    }
	const TexturedColor& Specular  () const { return specular;   }
	const TexturedFloat& Glossiness() const { return glossiness; }
	const TexturedColor& Emission  () const { return emission;   }
	const TexturedColor& Reflection() const { return reflection; }
	const TexturedColor& Refraction() const { return refraction; }

	Color Absorption     ( int mtlID=0 ) const override { return absorption; }
	float IOR            ( int mtlID=0 ) const override { return ior; }
	bool  IsPhotonSurface( int mtlID=0 ) const override { return diffuse.GetValue().Sum() > 0; }

protected:
	TexturedColor diffuse    = Color(0.5f);
	TexturedColor specular   = Color(0.7f);
	TexturedFloat glossiness = 20.0f;
	TexturedColor emission   = Color(0.0f);
	TexturedColor reflection = Color(0.0f);
	TexturedColor refraction = Color(0.0f);
	Color         absorption = Color(0.0f);
	float         ior        = 1.5f;	// index of refraction
};

//-------------------------------------------------------------------------------

class MtlPhong : public MtlBasePhongBlinn
{
public:
#ifdef LEGACY_SHADING_API
	Color Shade( ShadeInfo const &shadeInfo ) const override;
#endif
	void SetViewportMaterial( int mtlID=0 ) const override;	// used for OpenGL display

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f       &dir, Info &si ) const override{};
	void GetSampleInfo ( SamplerInfo const &sInfo, Vec3f const &dir, Info &si ) const override{};
};

//-------------------------------------------------------------------------------

class MtlBlinn : public MtlBasePhongBlinn
{
public:
#ifdef LEGACY_SHADING_API
	Color Shade( ShadeInfo const &shadeInfo ) const override;
#endif
	void SetViewportMaterial ( int mtlID=0 ) const override;	// used for OpenGL display

	//bool GenerateSample( SamplerInfo const &sInfo, Vec3f &dir, Info &si ) const override 
    //{
    //    float diffuseProb{ diffuse.GetValue().Gray() };
    //    float specularProb{ specular.GetValue().Gray() };
    //    float transmissiveProb{ refraction.GetValue().Gray() };

    //    const float totalProb{ diffuseProb + specularProb + transmissiveProb };
    //    if (totalProb > 1.0f)
    //    {
    //        diffuseProb /= totalProb;
    //        specularProb /= totalProb;
    //        transmissiveProb /= totalProb;
    //    }

    //    const float randomNum{ sInfo.RandomFloat() };
    //    if (randomNum < diffuseProb)
    //    {
    //        si.lobe = DirSampler::Lobe::DIFFUSE;
    //        si.mult = diffuse.GetValue();
    //        si.prob = diffuseProb;
    //        //const float geometryTerm{ sInfo.N() % sInfo.V() };
    //        //si.mult = diffuse.GetValue() * geometryTerm / static_cast<float>(M_PI);
    //        //si.prob /= 2.0f * M_PI;

    //        // Direction
    //        const float r1{ sInfo.RandomFloat() };
    //        const float r2{ sInfo.RandomFloat() };
    //        const float z{ sqrtf(r1) };
    //        const float sinTheta{ sqrtf(1.0f - r1) };
    //        const float phi{ 2.0f * M_PI * r2 };
    //        const float x{ sinTheta * cos(phi) };
    //        const float y{ sinTheta * sin(phi) };

    //        Vec3f u, v;
    //        sInfo.N().GetOrthonormals(u, v);
    //        dir = (x * u) + (y * v) + (z * sInfo.N());

    //        return true;
    //    }
    //    if (randomNum < diffuseProb + specularProb)
    //    {
    //        si.lobe = DirSampler::Lobe::SPECULAR;
    //        si.prob = specularProb;

    //        // Direction
    //        const float r1{ sInfo.RandomFloat() };
    //        const float r2{ sInfo.RandomFloat() };
    //        const float phi{ 2.0f * M_PI * r1 };
    //        const float cosTheta{ pow(r2, 1.0f / (glossiness.GetValue() + 1.0f)) };
    //        const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

    //        const float x{ sinTheta * cos(phi) };
    //        const float y{ sinTheta * sin(phi) };
    //        const float z{ cosTheta };

    //        Vec3f u, v;
    //        sInfo.N().GetOrthonormals(u, v);
    //        const Vec3f h{ (x * u) + (y * v) + (z * sInfo.N()) };

    //        dir = h * 2 * sInfo.V().Dot(h) - sInfo.V();
    //        si.mult = specular.GetValue() * dir.Dot(sInfo.N());
    //        return dir.Dot(sInfo.N()) > 0.0f;
    //    }
    //    if (randomNum < diffuseProb + specularProb + transmissiveProb)
    //    {
    //        const Vec3f N{ sInfo.IsFront() ? sInfo.N() : -sInfo.N() };

    //        si.lobe = DirSampler::Lobe::TRANSMISSION;
    //        //si.mult = refraction.GetValue();
    //        si.prob = transmissiveProb;
    //        
    //        // Direction
    //        const float r1{ sInfo.RandomFloat() };
    //        const float r2{ sInfo.RandomFloat() };
    //        const float phi{ 2.0f * M_PI * r1 };
    //        const float cosTheta{ pow(r2, 1.0f / (glossiness.GetValue() + 1.0f)) };
    //        const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

    //        const float x{ sinTheta * cos(phi) };
    //        const float y{ sinTheta * sin(phi) };
    //        const float z{ cosTheta };

    //        Vec3f u, v;
    //        N.GetOrthonormals(u, v);
    //        const Vec3f h{ (x * u) + (y * v) + (z * N) };

    //        const Vec3f V{ sInfo.V() };
    //        const float refractionRatio{ sInfo.IsFront() ? 1.0f / ior : ior };
    //        const float cosTheta_i{ V.Dot(h) };

    //        const float sinTheta_t_sq{ (refractionRatio * refractionRatio) * (1.0f - cosTheta_i * cosTheta_i) };

    //        if (sinTheta_t_sq > 1.0f)
    //        {
    //            si.lobe = DirSampler::Lobe::SPECULAR;

    //            const Vec3f V{ sInfo.V() };
    //            dir = h * 2 * V.Dot(h) - V;
    //            si.mult = refraction.GetValue() * std::abs(dir.Dot(sInfo.N()));
    //            return dir.Dot(N) > 0.0f;
    //        }

    //        const float cosTheta_t{ sqrtf(1.0f - sinTheta_t_sq) };

    //        dir = -V * refractionRatio + h * (refractionRatio * cosTheta_i - cosTheta_t);
    //        si.mult = refraction.GetValue() * std::abs(dir.Dot(sInfo.N()));

    //        return dir.Dot(N) < 0.0f;
    //    }
    //    return false;

    //}

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f &dir, Info &si ) const override 
    {
        float diffuseProb{ diffuse.GetValue().Gray() };
        float specularProb{ specular.GetValue().Gray() };
        float transmissiveProb{ refraction.GetValue().Gray() };

        //if (transmissiveProb > 0.0f) {
        //    const float R0{ 0.04f };
        //    const float cosTheta{ std::abs(sInfo.N().Dot(sInfo.V())) };
        //    const float F{ R0 + (1.0f - R0) * powf(1.0f - cosTheta, 5.0f) };
        //    const float totalSpecTrans{ specularProb + transmissiveProb };
        //    specularProb = totalSpecTrans * F;
        //    transmissiveProb = totalSpecTrans * (1.0f - F);
        //}

        const float totalProb{ diffuseProb + specularProb + transmissiveProb };
        diffuseProb /= totalProb;
        specularProb /= totalProb;
        transmissiveProb /= totalProb;

        const float randomNum{ sInfo.RandomFloat() };
        //if (randomNum < diffuseProb)
        //{
        //    si.lobe = DirSampler::Lobe::DIFFUSE;

        //    // Uniform hemisphere sample
        //    const float r1{ sInfo.RandomFloat() };
        //    const float cosTheta{ sInfo.RandomFloat() };
        //    const float sinTheta{ sqrtf(1.0f - (cosTheta * cosTheta)) };
        //    const float phi{ 2.0f * Pi<float>() * r1 };

        //    const float x{ cosf(phi) * sinTheta };
        //    const float y{ sinf(phi) * sinTheta };

        //    Vec3f u, v;
        //    sInfo.N().GetOrthonormals(u, v);

        //    dir = (u * x) + (v * y) + (sInfo.N() * cosTheta);

        //    si.prob = diffuseProb / (2.0f * Pi<float>());
        //    si.mult = diffuse.GetValue() / Pi<float>();

        //    return true;
        //}
        if (randomNum < diffuseProb)
        {
            si.lobe = DirSampler::Lobe::DIFFUSE;

            // Cosine weighted hemisphere sample
            const float r{ sqrtf(sInfo.RandomFloat()) };
            const float theta{ 2.0f * Pi<float>() * sInfo.RandomFloat() };
            const float x{ r * cosf(theta) };
            const float y{ r * sinf(theta) };
            const float z{ sqrtf(std::max(0.0f, 1.0f - x*x - y*y)) };

            Vec3f u, v;
            sInfo.N().GetOrthonormals(u, v);

            dir = (u * x) + (v * y) + (sInfo.N() * z);

            const float geometryTerm{ std::max(0.0f, sInfo.N().Dot(dir)) };

            si.mult = diffuse.GetValue() / Pi<float>();
            si.prob = diffuseProb * (geometryTerm / Pi<float>());

            return true;
        }
        if (randomNum < diffuseProb + specularProb)
        {
            si.lobe = DirSampler::Lobe::SPECULAR;
            //dir = -sInfo.V() - (sInfo.N() * 2.0f * sInfo.N().Dot(-sInfo.V()));

            const float phi{ 2.0f * Pi<float>() * sInfo.RandomFloat() };
            const float r2{ sInfo.RandomFloat() };
            const float alpha{ sqrtf(2.0f / (glossiness.GetValue() + 2.0f)) };
            const float alphaSquared{ alpha * alpha };
            const float cosTheta{ sqrtf((1.0f - r2) / (1.0f + (alphaSquared - 1.0f) * r2)) };
            const float sinTheta{ sqrtf(1.0f - cosTheta * cosTheta) };
            const float x{ cosf(phi) * sinTheta };
            const float y{ sinf(phi) * sinTheta };

            Vec3f u, v;
            sInfo.N().GetOrthonormals(u, v);
            const Vec3f h{ (x * u) + (y * v) + (cosTheta * sInfo.N()) };
            dir = h * 2.0f * std::max(0.0f, sInfo.V().Dot(h)) - sInfo.V();
            if (sInfo.N().Dot(dir) < 0.0f)
                return false;

            const float nDotH{ sInfo.N().Dot(h) };
            if (nDotH < 0.0f)
                return false;

            const float denom{ (nDotH * nDotH) * (alphaSquared - 1.0f) + 1.0f };
            const float ggxNdf{ alphaSquared / (Pi<float>() * denom * denom) };
            si.prob = specularProb * (ggxNdf * nDotH) / (4.0f * dir.Dot(h));

            const float geometryTerm{ std::max(0.0f, sInfo.N().Dot(dir)) };
            si.mult = geometryTerm < 1e-6 ? Color{ 0.0f } : specular.GetValue() / geometryTerm;
            //si.prob = specularProb;

            return true;
        }
        //if (randomNum < diffuseProb + specularProb + transmissiveProb)
        //{
        //    const Vec3f N{ sInfo.IsFront() ? sInfo.N() : -sInfo.N() };

        //    si.lobe = DirSampler::Lobe::TRANSMISSION;
        //    //si.mult = refraction.GetValue();
        //    //si.prob = transmissiveProb;
        //    
        //    // Direction
        //    const float r1{ sInfo.RandomFloat() };
        //    const float r2{ sInfo.RandomFloat() };
        //    const float phi{ 2.0f * M_PI * r1 };
        //    const float cosTheta{ pow(r2, 1.0f / (glossiness.GetValue() + 1.0f)) };
        //    const float sinTheta{ sqrt(1.0f - cosTheta * cosTheta) };

        //    const float x{ sinTheta * cos(phi) };
        //    const float y{ sinTheta * sin(phi) };
        //    const float z{ cosTheta };

        //    Vec3f u, v;
        //    N.GetOrthonormals(u, v);
        //    const Vec3f h{ (x * u) + (y * v) + (z * N) };

        //    const Vec3f V{ sInfo.V() };
        //    const float refractionRatio{ sInfo.IsFront() ? 1.0f / ior : ior };
        //    const float cosTheta_i{ V.Dot(h) };

        //    const float sinTheta_t_sq{ (refractionRatio * refractionRatio) * (1.0f - cosTheta_i * cosTheta_i) };

        //    if (sinTheta_t_sq > 1.0f)
        //    {
        //        si.lobe = DirSampler::Lobe::SPECULAR;

        //        const Vec3f V{ sInfo.V() };
        //        dir = h * 2 * V.Dot(h) - V;
        //        //si.mult = refraction.GetValue() * std::abs(dir.Dot(sInfo.N()));
        //        si.mult = refraction.GetValue() / transmissiveProb;

        //        const float halfPDF{ ((glossiness.GetValue() + 1.0f) / (2.0f * M_PI)) * powf(cosTheta, glossiness.GetValue()) };
        //        const float geoPDF{ halfPDF / (4.0f * sInfo.V().Dot(h)) };
        //        si.prob = transmissiveProb * geoPDF;

        //        return dir.Dot(N) > 0.0f;
        //    }

        //    const float cosTheta_t{ sqrtf(1.0f - sinTheta_t_sq) };

        //    dir = -V * refractionRatio + h * (refractionRatio * cosTheta_i - cosTheta_t);
        //    //si.mult = refraction.GetValue() * std::abs(dir.Dot(sInfo.N()));
        //    si.mult = refraction.GetValue() / transmissiveProb;

        //    const float halfPDF{ ((glossiness.GetValue() + 1.0f) / (2.0f * M_PI)) * powf(cosTheta, glossiness.GetValue()) };
        //    float denom{ dir.Dot(h) + refractionRatio * V.Dot(h) };
        //    denom *= denom;
        //    const float numerator{ (refractionRatio * refractionRatio) * std::abs(dir.Dot(h)) };
        //    const float geoPDF{ halfPDF * (numerator / denom) };

        //    //const float geoPDF{ halfPDF / (4.0f * sInfo.V().Dot(h)) };
        //    si.prob = transmissiveProb * geoPDF;

        //    return dir.Dot(N) < 0.0f;
        //}
        return false;

    }

	void GetSampleInfo ( SamplerInfo const &sInfo, Vec3f const &dir, Info &si ) const override
    {
        float diffuseProb{ diffuse.GetValue().Gray() };
        float specularProb{ specular.GetValue().Gray() };
        float transmissiveProb{ refraction.GetValue().Gray() };

        if (transmissiveProb > 0.0f) {
            const float R0{ 0.04f };
            // Note: Use sInfo.V() (View Vector) and sInfo.N() (Normal) for Fresnel
            const float cosTheta{ std::abs(sInfo.N().Dot(sInfo.V())) };
            const float F{ R0 + (1.0f - R0) * powf(1.0f - cosTheta, 5.0f) };
            
            const float totalSpecTrans{ specularProb + transmissiveProb };
            specularProb = totalSpecTrans * F;
            transmissiveProb = totalSpecTrans * (1.0f - F);
        }

        const float totalProb{ diffuseProb + specularProb + transmissiveProb };
        diffuseProb /= totalProb;
        specularProb /= totalProb;
        transmissiveProb /= totalProb;

        const float nDotL{ sInfo.N().Dot(dir) };
        const bool isReflection{ nDotL > 0.0f };

        si.prob = 0.0f;
        if (diffuseProb > 0.0f && isReflection)
        {
            const float pdfDiff{ nDotL / M_PI };
            si.prob += diffuseProb * pdfDiff;
        }
        if (specularProb > 0.0f && isReflection)
        {
            const Vec3f h{ (sInfo.V() + dir).GetNormalized() };
            const float nDotH{ std::max(0.0f, sInfo.N().Dot(h)) };
            const float vDotH{ std::max(0.001f, sInfo.V().Dot(h)) };

            if (nDotH > 0.0f)
            {
                const float gloss{ glossiness.GetValue() };
                const float halfPDF{ ((gloss + 1.0f) / (2.0f * M_PI)) * powf(nDotH, gloss) };
                const float specularPDF{ halfPDF / (4.0f * vDotH) };
                si.prob += specularProb * specularPDF;
            }
        }
        if (transmissiveProb > 0.0f && !isReflection)
        {
            const float eta{ sInfo.IsFront() ? (1.0f / ior) : ior };

            Vec3f h{ -(eta * sInfo.V() + dir).GetNormalized() };
            if (h.Dot(sInfo.N()) < 0.0f) h = -h;

            const float nDotH{ std::max(0.0f, sInfo.N().Dot(h)) };
            const float vDotH{ std::abs(sInfo.V().Dot(h)) };
            const float lDotH{ std::abs(dir.Dot(h)) };

            if (nDotH > 0.0f)
            {
                const float gloss{ glossiness.GetValue() };

                const float halfPDF{ ((gloss + 1.0f) / (2.0f * static_cast<float>(M_PI))) * powf(nDotH, gloss) };

                float denom{ (lDotH + eta * vDotH) };
                denom = denom * denom;

                const float numerator{ lDotH * (eta * eta) };
                const float transPDF{ halfPDF * (numerator / denom) };

                si.prob += transmissiveProb * transPDF;
            }
        }
    }
};

//-------------------------------------------------------------------------------

class MtlMicrofacet : public Material
{
public:
	void Load( Loader const &loader, TextureFileList &tfl ) override;

	void SetBaseColor    ( Color const &c ) { baseColor    .SetValue(c); }
	void SetRoughness    ( float        r ) { roughness    .SetValue(r); }
	void SetMetallic     ( float        m ) { metallic     .SetValue(m); }
	void SetEmission     ( Color const &e ) { emission     .SetValue(e); }
	void SetTransmittance( Color const &t ) { transmittance.SetValue(t); }
	void SetAbsorption   ( Color const &a ) { absorption = a; }
	void SetIOR          ( float        i ) { ior        = i; }

	void SetBaseColorTexture    ( TextureMap *tex ) { baseColor    .SetTexture(tex); }
	void SetRoughnessTexture    ( TextureMap *tex ) { roughness    .SetTexture(tex); }
	void SetMetallicTexture     ( TextureMap *tex ) { metallic     .SetTexture(tex); }
	void SetEmissionTexture     ( TextureMap *tex ) { emission     .SetTexture(tex); }
	void SetTransmittanceTexture( TextureMap *tex ) { transmittance.SetTexture(tex); }

#ifdef LEGACY_SHADING_API
	Color Shade( ShadeInfo const &shadeInfo ) const override;
#endif
	Color Absorption         ( int mtlID=0 ) const override { return absorption; }
	float IOR                ( int mtlID=0 ) const override { return ior;        }
	bool  IsPhotonSurface    ( int mtlID=0 ) const override { return baseColor.GetValue().Sum() > 0; }
	void  SetViewportMaterial( int mtlID=0 ) const override;	// used for OpenGL display

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f       &dir, Info &si ) const override{};
	void GetSampleInfo ( SamplerInfo const &sInfo, Vec3f const &dir, Info &si ) const override{};

private:
	TexturedColor baseColor     = Color(0.5f);	// albedo for dielectrics, F0 for metals
	TexturedFloat roughness     = 1.0f;
	TexturedFloat metallic      = 0.0f;
	TexturedColor emission      = Color(0.0f);
	TexturedColor transmittance = Color(0.0f);
	Color         absorption    = Color(0.0f);
	float         ior           = 1.5f;	// index of refraction
};

//-------------------------------------------------------------------------------

class MultiMtl : public Material
{
public:
	virtual ~MultiMtl() { for ( Material *m : mtls ) delete m; }

#ifdef LEGACY_SHADING_API
	Color Shade( ShadeInfo const &sInfo ) const override { int m = sInfo.MaterialID(); return m<(int)mtls.size() ? mtls[m]->Shade(sInfo) : Color(1,1,1); }
#endif
	Color Absorption         ( int mtlID=0 ) const override { return mtlID<(int)mtls.size() ? mtls[mtlID]->Absorption     (mtlID) : Material::Absorption     (mtlID); }
	float IOR                ( int mtlID=0 ) const override { return mtlID<(int)mtls.size() ? mtls[mtlID]->IOR            (mtlID) : Material::IOR            (mtlID); }
	bool  IsPhotonSurface    ( int mtlID=0 ) const override { return mtlID<(int)mtls.size() ? mtls[mtlID]->IsPhotonSurface(mtlID) : Material::IsPhotonSurface(mtlID); }
	void  SetViewportMaterial( int mtlID=0 ) const override { if   ( mtlID<(int)mtls.size() ) mtls[mtlID]->SetViewportMaterial(); }

	void AppendMaterial( Material *m ) { mtls.push_back(m); }

	bool GenerateSample( SamplerInfo const &sInfo, Vec3f &dir, Info &si ) const override
	{
		int m = sInfo.MaterialID();
		if ( m < (int)mtls.size() ) return mtls[m]->GenerateSample(sInfo,dir,si);
		else return Material::GenerateSample(sInfo,dir,si);
	}

	// Set the material sample information for the given direction sample.
	void GetSampleInfo( SamplerInfo const &sInfo, Vec3f const &dir, Info &si ) const override
	{
		int m = sInfo.MaterialID();
		if ( m < (int)mtls.size() ) mtls[m]->GetSampleInfo(sInfo,dir,si);
		else Material::GetSampleInfo(sInfo,dir,si);
	}

private:
	std::vector<Material*> mtls;
};

//-------------------------------------------------------------------------------

#endif
