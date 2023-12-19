#include "o2Editor/stdafx.h"

#include "o2/Render/TextureRef.h"

using namespace o2;

namespace Editor
{
	// ----------------------
	// Editor common textures
	// ----------------------
	struct CommonTextures
	{
		// Initializes common textures
		static void Initialize();

		static Ref<Texture> checkedBackground;
	};
}
