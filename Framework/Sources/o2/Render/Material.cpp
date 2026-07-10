#include "o2/stdafx.h"
#include "Material.h"

#include "o2/EngineSettings.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"

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

	ShaderParamFloatVector::ShaderParamFloatVector()
	{}

	ShaderParamFloatVector::ShaderParamFloatVector(const String& name, const Vector<float>& value)
	{
		mName = name;
		mValue = value;
	}

	const Vector<float>& ShaderParamFloatVector::GetValue() const
	{
		return mValue;
	}

	void ShaderParamFloatVector::SetValue(const Vector<float>& value)
	{
		mValue = value;
	}

	size_t ShaderParamFloatVector::ComputeHash() const
	{
		size_t h = IShaderParam::ComputeHash();
		for (float value : mValue)
			h ^= std::hash<float>()(value) + 0x9e3779b9 + (h << 6) + (h >> 2);

		return h;
	}

	TextureRef TextureSampler::GetTexture() const
	{
		if (textureOverride)
			return textureOverride;

		if (image)
			return image->GetTextureSource().texture;

		return TextureRef();
	}

	RectI TextureSampler::GetSrcRect() const
	{
		if (textureOverride)
			return RectI();

		if (image)
			return image->GetTextureSource().sourceRect;

		return RectI();
	}


	bool TextureSampler::operator==(const TextureSampler& other) const
	{
		return samplerUniformName == other.samplerUniformName &&
			texCoordsAttrName == other.texCoordsAttrName &&
			image == other.image &&
			textureOverride == other.textureOverride;
	}

	Material::Material()
	{}

	Material::Material(const Material& other):
		mVertexShader(other.mVertexShader), mFragmentShader(other.mFragmentShader), mTexture(other.mTexture),
		mBlendMode(other.mBlendMode), mColorAttachmentsCount(other.mColorAttachmentsCount),
		mColorAttachmentFormats(other.mColorAttachmentFormats), mVertexLayoutSkinned(other.mVertexLayoutSkinned),
		mSamplers(other.mSamplers), mHashDirty(true)
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

	void Material::SetSamplerTextureOverride(const String& samplerUniformName, const TextureRef& texture)
	{
		for (auto& sampler : mSamplers)
		{
			if (sampler.samplerUniformName == samplerUniformName)
			{
				sampler.textureOverride = texture;
				mHashDirty = true;
				return;
			}
		}
	}

	int Material::GetTotalTextureChannelsCount() const
	{
		return 1 + mSamplers.Count();
	}

	void Material::SetColorAttachmentsCount(int count)
	{
		count = Math::Max(count, 1);
		if (mColorAttachmentsCount == count)
			return;

		PlatformDestroy();
		mReady = false;
		mColorAttachmentsCount = count;
		mHashDirty = true;
	}

	int Material::GetColorAttachmentsCount() const
	{
		return mColorAttachmentsCount;
	}

	void Material::SetColorAttachmentFormats(const Vector<TextureFormat>& formats)
	{
		if (mColorAttachmentFormats == formats)
			return;

		PlatformDestroy();
		mReady = false;
		mColorAttachmentFormats = formats;
		mColorAttachmentsCount = Math::Max(mColorAttachmentsCount, formats.Count());
		mHashDirty = true;
	}

	const Vector<TextureFormat>& Material::GetColorAttachmentFormats() const
	{
		return mColorAttachmentFormats;
	}

	void Material::SetVertexLayoutSkinned(bool skinned)
	{
		mVertexLayoutSkinned = skinned;
	}

	bool Material::IsVertexLayoutSkinned() const
	{
		return mVertexLayoutSkinned;
	}

	Ref<Material> Material::CreateFromBuiltinShaders(const String& shadersName)
	{
		String basePath = String(GetBuiltinAssetsPath()) + "Shaders/" + shadersName;
		String vertexPath = Shader::ResolvePlatformSourcePath(basePath + ".vsh");
		String fragmentPath = Shader::ResolvePlatformSourcePath(basePath + ".fsh");

		String vertexSource = o2FileSystem.ReadFile(vertexPath);
		String fragmentSource = o2FileSystem.ReadFile(fragmentPath);

		if (vertexSource.IsEmpty() || fragmentSource.IsEmpty())
		{
			o2Debug.LogError("Failed to load builtin shader files: " + vertexPath + ", " + fragmentPath);
			return nullptr;
		}

		Ref<Shader> vertexShader = mmake<Shader>();
		Ref<Shader> fragmentShader = mmake<Shader>();
		vertexShader->SetFileName(vertexPath);
		fragmentShader->SetFileName(fragmentPath);
		vertexShader->Compile(vertexSource, Shader::Type::Vertex);
		fragmentShader->Compile(fragmentSource, Shader::Type::Fragment);

		if (!vertexShader->IsReady() || !fragmentShader->IsReady())
		{
			o2Debug.LogError("Failed to compile builtin shaders: " + shadersName);
			return nullptr;
		}

		Ref<Material> material = mmake<Material>();
		material->SetVertexShader(vertexShader);
		material->SetFragmentShader(fragmentShader);
		material->SetBlendMode(BlendMode::Normal);

		return material;
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
		h ^= std::hash<int>()(mColorAttachmentsCount) + 0x9e3779b9 + (h << 6) + (h >> 2);

		for (auto format : mColorAttachmentFormats)
			h ^= std::hash<int>()(static_cast<int>(format)) + 0x9e3779b9 + (h << 6) + (h >> 2);

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

DECLARE_CLASS(o2::ShaderParamFloatVector, o2__ShaderParamFloatVector);

DECLARE_CLASS(o2::TextureSampler, o2__TextureSampler);

DECLARE_CLASS(o2::Material, o2__Material);
// --- END META ---
