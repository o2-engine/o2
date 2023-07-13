#pragma once

#include "IAssetConverter.h"
#include "o2/Utils/Tools/RectPacker.h"
#include "o2AssetBuilder/AssetsBuilder.h"

namespace o2
{
	class Bitmap;

	// -----------------
	// Atlases converter
	// -----------------
	class AtlasAssetConverter : public IAssetConverter
	{
	public:
		// Returns vector of processing assets types
		Vector<const Type*> GetProcessingAssetsTypes() const;

		// Converts atlas by path
		void ConvertAsset(const AssetInfo& node);

		// Removes atlas by path
		void RemoveAsset(const AssetInfo& node);

		// Moves atlas
		void MoveAsset(const AssetInfo& nodeFrom, const AssetInfo& nodeTo);

		// Post processing atlases. Here checking atlases for rebuild
		Vector<UID> AssetsPostProcess();

		// Resets converter
		void Reset();

		IOBJECT(AtlasAssetConverter);

	public:
		// ----------------
		// Atlas image info
		// ----------------
		struct Image : public ISerializable
		{
			UID       id;   // Image asset id @SERIALIZABLE
			TimeStamp time; // Image asset edited date @SERIALIZABLE

		public:
			// Default constructor
			Image() {}

			// Constructor
			Image(const UID& id, const TimeStamp& time);

			// Check equal operator
			bool operator==(const Image& other) const;

			SERIALIZABLE(Image);
		};

		// ------------------------
		// Image packing definition
		// ------------------------
		struct ImagePackDef
		{
			Bitmap* bitmap = nullptr;    // Image bitmap pointer
			RectsPacker::Rect* packRect = nullptr;  // Image pack rectangle pointer
			AssetInfo* assetInfo = nullptr; // Asset information

			// Check equal operator
			bool operator==(const ImagePackDef& other) const;
		};

	protected:
		// Checks atlases for rebuilding
		Vector<UID> CheckRebuildingAtlases();

		// Checks atlas for rebuilding
		bool CheckAtlasRebuilding(AssetInfo* atlasInfo);

		// Returns true if atlas needs to rebuild
		bool IsAtlasNeedRebuild(Vector<Image>& currentImages, Vector<Image>& lastImages);

		// Rebuilds atlas
		void RebuildAtlas(AssetInfo* atlasInfo, Vector<Image>& images);

		// Saves image asset data
		void SaveImageAsset(ImagePackDef& imgDef);
	};
}
// --- META ---

CLASS_BASES_META(o2::AtlasAssetConverter)
{
	BASE_CLASS(o2::IAssetConverter);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAssetConverter)
{
}
END_META;
CLASS_METHODS_META(o2::AtlasAssetConverter)
{

	FUNCTION().PUBLIC().SIGNATURE(Vector<const Type*>, GetProcessingAssetsTypes);
	FUNCTION().PUBLIC().SIGNATURE(void, ConvertAsset, const AssetInfo&);
	FUNCTION().PUBLIC().SIGNATURE(void, RemoveAsset, const AssetInfo&);
	FUNCTION().PUBLIC().SIGNATURE(void, MoveAsset, const AssetInfo&, const AssetInfo&);
	FUNCTION().PUBLIC().SIGNATURE(Vector<UID>, AssetsPostProcess);
	FUNCTION().PUBLIC().SIGNATURE(void, Reset);
	FUNCTION().PROTECTED().SIGNATURE(Vector<UID>, CheckRebuildingAtlases);
	FUNCTION().PROTECTED().SIGNATURE(bool, CheckAtlasRebuilding, AssetInfo*);
	FUNCTION().PROTECTED().SIGNATURE(bool, IsAtlasNeedRebuild, Vector<Image>&, Vector<Image>&);
	FUNCTION().PROTECTED().SIGNATURE(void, RebuildAtlas, AssetInfo*, Vector<Image>&);
	FUNCTION().PROTECTED().SIGNATURE(void, SaveImageAsset, ImagePackDef&);
}
END_META;

CLASS_BASES_META(o2::AtlasAssetConverter::Image)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::AtlasAssetConverter::Image)
{
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(id);
	FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(time);
}
END_META;
CLASS_METHODS_META(o2::AtlasAssetConverter::Image)
{

	FUNCTION().PUBLIC().CONSTRUCTOR();
	FUNCTION().PUBLIC().CONSTRUCTOR(const UID&, const TimeStamp&);
}
END_META;
// --- END META ---
