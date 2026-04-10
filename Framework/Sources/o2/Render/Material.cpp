#include "o2/stdafx.h"
#include "Material.h"

#include "o2/Utils/Debug/Debug.h"

#include <functional>

namespace o2
{
	const String& IShaderParam::GetName() const
	{
		return mName;
	}

	void IShaderParam::SetName(const String& name)
	{
		mName = name;
	}

	size_t IShaderParam::ComputeHash() const
	{
		return std::hash<std::string>()(std::string(mName.Data(), mName.Length()));
	}

	ShaderParamFloat::ShaderParamFloat()
	{}

	ShaderParamFloat::ShaderParamFloat(const String& name, float value)
	{
		mName = name;
		mValue = value;
	}

	float ShaderParamFloat::GetValue() const 
	{ 
		return mValue; 
	}

	void ShaderParamFloat::SetValue(float value)
	{ 
		mValue = value; 
	}

	size_t ShaderParamFloat::ComputeHash() const
	{
		size_t h = IShaderParam::ComputeHash();
		h ^= std::hash<float>()(mValue) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}

	ShaderParamVec2::ShaderParamVec2()
	{}

	ShaderParamVec2::ShaderParamVec2(const String& name, const Vec2F& value)
	{
		mName = name;
		mValue = value;
	}

	const Vec2F& ShaderParamVec2::GetValue() const { return mValue; }
	void ShaderParamVec2::SetValue(const Vec2F& value) { mValue = value; }

	size_t ShaderParamVec2::ComputeHash() const
	{
		size_t h = IShaderParam::ComputeHash();
		h ^= std::hash<float>()(mValue.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
		h ^= std::hash<float>()(mValue.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}

	ShaderParamColor::ShaderParamColor()
	{}

	ShaderParamColor::ShaderParamColor(const String& name, const Color4& value)
	{
		mName = name;
		mValue = value;
	}

	const Color4& ShaderParamColor::GetValue() const 
	{ 
		return mValue;
	}

	void ShaderParamColor::SetValue(const Color4& value) 
	{ 
		mValue = value; 
	}

	size_t ShaderParamColor::ComputeHash() const
	{
		size_t h = IShaderParam::ComputeHash();
		h ^= std::hash<ULong>()(mValue.ABGR()) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}

	ShaderParamInt::ShaderParamInt() 
	{}

	ShaderParamInt::ShaderParamInt(const String& name, int value)
	{
		mName = name;
		mValue = value;
	}

	int ShaderParamInt::GetValue() const 
	{ 
		return mValue; 
	}

	void ShaderParamInt::SetValue(int value) 
	{ 
		mValue = value;
	}

	size_t ShaderParamInt::ComputeHash() const
	{
		size_t h = IShaderParam::ComputeHash();
		h ^= std::hash<int>()(mValue) + 0x9e3779b9 + (h << 6) + (h >> 2);
		return h;
	}

	TextureRef TextureSampler::GetTexture() const
	{
		if (image)
			return image->GetTextureSource().texture;

		return TextureRef();
	}

	RectI TextureSampler::GetSrcRect() const
	{
		if (image)
			return image->GetTextureSource().sourceRect;

		return RectI();
	}


	bool TextureSampler::operator==(const TextureSampler& other) const
	{
		return samplerUniformName == other.samplerUniformName &&
			texCoordsAttrName == other.texCoordsAttrName &&
			image == other.image;
	}

	Material::Material()
	{}

	Material::Material(const Material& other):
		mVertexShader(other.mVertexShader), mFragmentShader(other.mFragmentShader), mTexture(other.mTexture),
		mBlendMode(other.mBlendMode), mSamplers(other.mSamplers), mHashDirty(true)
	{
		for (auto& param : other.mParams)
			mParams.Add(param->CloneAsRef<IShaderParam>());

		if (other.mReady)
			Build();
	}

	Material::~Material()
	{
		PlatformDestroy();
	}

	void Material::SetVertexShader(const Ref<Shader>& shader)
	{
		if (mVertexShader == shader)
			return;

		PlatformDestroy();
		mReady = false;
		mVertexShader = shader;
		mHashDirty = true;
	}

	const Ref<Shader>& Material::GetVertexShader() const
	{
		return mVertexShader;
	}

	void Material::SetFragmentShader(const Ref<Shader>& shader)
	{
		if (mFragmentShader == shader)
			return;

		PlatformDestroy();
		mReady = false;
		mFragmentShader = shader;
		mHashDirty = true;
	}

	const Ref<Shader>& Material::GetFragmentShader() const
	{
		return mFragmentShader;
	}

	void Material::SetTexture(const TextureRef& texture)
	{
		mTexture = texture;
		mHashDirty = true;
	}

	const TextureRef& Material::GetTexture() const
	{
		return mTexture;
	}

	void Material::SetBlendMode(BlendMode blendMode)
	{
		if (mBlendMode == blendMode)
			return;

		PlatformDestroy();
		mReady = false;
		mBlendMode = blendMode;
		mHashDirty = true;
	}

	BlendMode Material::GetBlendMode() const
	{
		return mBlendMode;
	}

	Ref<IShaderParam> Material::GetShaderParam(const String& name) const
	{
		for (auto& param : mParams)
		{
			if (param->GetName() == name)
				return param;
		}

		return nullptr;
	}

	Map<String, Ref<IShaderParam>> Material::GetAllShaderParamsMap() const
	{
		Map<String, Ref<IShaderParam>> result;

		for (auto& param : mParams)
			result[param->GetName()] = param;

		return result;
	}

	void Material::AddParam(const Ref<IShaderParam>& param)
	{
		for (int i = 0; i < mParams.Count(); i++)
		{
			if (mParams[i]->GetName() == param->GetName())
			{
				mParams[i] = param;
				mHashDirty = true;
				return;
			}
		}

		mParams.Add(param);

		mHashDirty = true;
	}

	void Material::RemoveParam(const String& name)
	{
		mParams.RemoveFirst([&](const Ref<IShaderParam>& p) { return p->GetName() == name; });
		mHashDirty = true;
	}

	const Vector<Ref<IShaderParam>>& Material::GetParams() const
	{
		return mParams;
	}

	void Material::SetParams(const Vector<Ref<IShaderParam>>& params)
	{
		mParams = params;
		mHashDirty = true;
	}

	void Material::AddTextureSampler(const TextureSampler& sampler)
	{
		for (int i = 0; i < mSamplers.Count(); i++)
		{
			if (mSamplers[i].samplerUniformName == sampler.samplerUniformName)
			{
				PlatformDestroy();
				mReady = false;
				mSamplers[i] = sampler;
				mHashDirty = true;
				return;
			}
		}

		PlatformDestroy();
		mReady = false;
		mSamplers.Add(sampler);

		mHashDirty = true;
	}

	void Material::RemoveTextureSampler(const String& samplerUniformName)
	{
		int samplerIdx = mSamplers.IndexOf([&](const TextureSampler& s) { return s.samplerUniformName == samplerUniformName; });
		if (samplerIdx < 0)
			return;

		mSamplers.RemoveAt(samplerIdx);
		PlatformDestroy();
		mReady = false;
		mHashDirty = true;
	}

	const Vector<TextureSampler>& Material::GetTextureSamplers() const
	{
		return mSamplers;
	}

	int Material::GetTotalTextureChannelsCount() const
	{
		return 1 + mSamplers.Count();
	}

	bool Material::Build()
	{
		PlatformDestroy();

		if (!mVertexShader || !mVertexShader->IsReady() || !mFragmentShader || !mFragmentShader->IsReady())
			return false;

		mReady = PlatformBuild();
		mHashDirty = true;

		return mReady;
	}

	bool Material::IsReady() const
	{
		return mReady;
	}

	size_t Material::GetHash()
	{
		if (mHashDirty)
		{
			mHash = ComputeHash();
			mHashDirty = false;
		}

		return mHash;
	}

	void Material::InvalidateHash()
	{
		mHashDirty = true;
	}

	int Material::GetTransformUniform() const
	{
		return mTransformUniform;
	}

	int Material::GetTextureUniform() const
	{
		return mTextureUniform;
	}

	int Material::GetPositionAttribute() const
	{
		return mPositionAttribute;
	}

	int Material::GetColorAttribute() const
	{
		return mColorAttribute;
	}

	int Material::GetTexCoordsAttribute() const
	{
		return mTexCoordsAttribute;
	}

	int Material::GetNormalAttribute() const
	{
		return mNormalAttribute;
	}

	void Material::ApplyParams() const
	{
		PlatformApplyParams();
	}

	size_t Material::ComputeHash() const
	{
		size_t h = 0;
		h ^= std::hash<UInt>()(mProgram) + 0x9e3779b9 + (h << 6) + (h >> 2);

		if (mVertexShader)
			h ^= std::hash<size_t>()((size_t)mVertexShader.Get()) + 0x9e3779b9 + (h << 6) + (h >> 2);

		if (mFragmentShader)
			h ^= std::hash<size_t>()((size_t)mFragmentShader.Get()) + 0x9e3779b9 + (h << 6) + (h >> 2);

		if (mTexture)
			h ^= std::hash<size_t>()((size_t)mTexture.Get()) + 0x9e3779b9 + (h << 6) + (h >> 2);

		h ^= std::hash<int>()(static_cast<int>(mBlendMode)) + 0x9e3779b9 + (h << 6) + (h >> 2);

		for (const auto& sampler : mSamplers)
		{
			h ^= std::hash<std::string>()(std::string(sampler.samplerUniformName.Data(), sampler.samplerUniformName.Length())) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<std::string>()(std::string(sampler.texCoordsAttrName.Data(), sampler.texCoordsAttrName.Length())) + 0x9e3779b9 + (h << 6) + (h >> 2);

			TextureRef samplerTexture = sampler.GetTexture();
			if (samplerTexture)
				h ^= std::hash<size_t>()((size_t)samplerTexture.Get()) + 0x9e3779b9 + (h << 6) + (h >> 2);
		}

		for (const auto& param : mParams)
			h ^= param->ComputeHash() + 0x9e3779b9 + (h << 6) + (h >> 2);

		return h;
	}

	void Material::OnDeserializedDelta(const DataValue& node, const IObject& origin)
	{
		OnDeserialized(node);
	}

}
// --- META ---

DECLARE_CLASS(o2::IShaderParam, o2__IShaderParam);

DECLARE_CLASS(o2::ShaderParamFloat, o2__ShaderParamFloat);

DECLARE_CLASS(o2::ShaderParamVec2, o2__ShaderParamVec2);

DECLARE_CLASS(o2::ShaderParamColor, o2__ShaderParamColor);

DECLARE_CLASS(o2::ShaderParamInt, o2__ShaderParamInt);

DECLARE_CLASS(o2::TextureSampler, o2__TextureSampler);

DECLARE_CLASS(o2::Material, o2__Material);
// --- END META ---
