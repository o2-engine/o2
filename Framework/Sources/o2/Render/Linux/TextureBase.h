#pragma once

#ifdef PLATFORM_LINUX

#include "o2/Render/Linux/OpenGL.h"

namespace o2
{
	class TextureBase
	{
		friend class Render;
		friend class VectorFont;

	protected:
		GLuint mHandle;      // Texture handle
		GLuint mFrameBuffer; // Frame buffer for rendering into texture
	};
}

#endif // PLATFORM_LINUX
