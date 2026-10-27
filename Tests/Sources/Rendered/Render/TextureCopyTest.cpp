#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Texture.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Types/WeakRef.h"
#include "o2/Utils/Bitmap/Bitmap.h"

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

using namespace o2;

// The vector font atlas relies on Copy when it grows: the old texture content
// must survive at the same pixel positions in the new one, or already
// rendered glyphs go blank
TEST(TextureCopy, KeepsSourceContentInGrownTexture)
{
	const Vec2I sourceSize(64, 64);

	Bitmap sourceBitmap(PixelFormat::R8G8B8A8, sourceSize);
	sourceBitmap.Fill(Color4(255, 0, 0, 255));
	sourceBitmap.FillRect(10, 40, 40, 10, Color4(0, 255, 0, 255));

	TextureRef source(sourceSize);
	source->SetData(sourceBitmap);

	TextureRef target(sourceSize*2, TextureFormat::R8G8B8A8, Texture::Usage::Default);
	target->Copy(*source.Get(), RectI(Vec2I(0, 0), sourceSize));

	Ref<Bitmap> result = target->GetData();
	ASSERT_TRUE(result);
	ASSERT_EQ(result->GetSize(), sourceSize*2);

	auto pixel = [](const Bitmap& bitmap, int x, int y) {
		const UInt8* data = bitmap.GetData();
		int idx = (y*bitmap.GetSize().x + x)*4;
		return Color4(data[idx], data[idx + 1], data[idx + 2], data[idx + 3]);
	};

	// The copied region keeps every pixel at its buffer position
	for (int y = 0; y < sourceSize.y; y += 7)
	{
		for (int x = 0; x < sourceSize.x; x += 7)
			ASSERT_EQ(pixel(*result, x, y), pixel(sourceBitmap, x, y)) << "at " << x << ", " << y;
	}
}

// A texture made from data goes with its last reference: the render only tracks it, so it is not kept forever
TEST(TextureRegistry, DataTexturesAreFreedWithTheirOwners)
{
	Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(8, 8));
	bitmap.Fill(Color4(10, 20, 30, 255));

	WeakRef<Texture> weak;
	{
		TextureRef texture(bitmap);
		ASSERT_TRUE(texture);
		weak = WeakRef<Texture>(texture.Get());
		EXPECT_FALSE(weak.IsExpired());
	}
	EXPECT_TRUE(weak.IsExpired());
}

#if defined(__APPLE__)
static double FootprintMb()
{
	task_vm_info_data_t info;
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&info, &count) != KERN_SUCCESS)
		return 0.0;
	return info.phys_footprint/(1024.0*1024.0);
}

// A freed texture gives its GPU memory back: 64 MB of textures made and dropped over and over stay 64 MB
TEST(TextureRegistry, FreedTexturesReleaseTheirMemory)
{
	Bitmap bitmap(PixelFormat::R8G8B8A8, Vec2I(1024, 1024));
	bitmap.Fill(Color4(10, 20, 30, 255));

	auto round = [&]()
	{
		Vector<TextureRef> textures;
		for (int i = 0; i < 16; i++)
			textures.Add(TextureRef(bitmap));
	};

	round();
	double start = FootprintMb();
	for (int i = 0; i < 8; i++)
		round();
	EXPECT_LT(FootprintMb() - start, 100.0);
}
#endif
