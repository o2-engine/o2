#include "o2/stdafx.h"
#include "VectorRasterCache.h"

#include <algorithm>
#include <climits>

#include "o2/Render/Render.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Utils/Jobs/JobSystem.h"

namespace o2
{
    DECLARE_SINGLETON(VectorRasterCache);
    FORWARD_REF_IMPL(VectorRasterCache);

    int VectorRasterCache::mMaxPagesCount = 3;
    VectorRasterCache::Statistics VectorRasterCache::mStatistics;

    bool VectorRasterCache::Raster::IsReady() const
    {
        return mState == State::Ready;
    }

    bool VectorRasterCache::Raster::IsAlive() const
    {
        return mState != State::Freed;
    }

    const TextureRef& VectorRasterCache::Raster::GetTexture() const
    {
        return mPage;
    }

    const RectI& VectorRasterCache::Raster::GetCell() const
    {
        return mCell;
    }

    const Vec2F& VectorRasterCache::Raster::GetScale() const
    {
        return mScale;
    }

    void VectorRasterCache::Raster::GetImageArea(Vec2F& leftTop, Vec2F& rightBottom) const
    {
        leftTop = Vec2F((float)mOrigin.x/mScale.x, (float)mOrigin.y/mScale.y);
        rightBottom = Vec2F((float)(mOrigin.x + mCell.Width())/mScale.x, (float)(mOrigin.y + mCell.Height())/mScale.y);
    }

    Vec2F VectorRasterCache::Raster::GetUV(const Vec2F& imagePoint) const
    {
        Vec2F pageSize = mPage ? Vec2F(mPage->GetSize()) : Vec2F(1, 1);
        float x = imagePoint.x*mScale.x - (float)mOrigin.x;
        float y = imagePoint.y*mScale.y - (float)mOrigin.y;

        // The bitmap rows go up from the cell bottom, the image Y goes down from its top
        return Vec2F(((float)mCell.left + x)/pageSize.x, ((float)mCell.top - y)/pageSize.y);
    }

    UInt VectorRasterCache::Raster::GetImageVersion() const
    {
        return mImageVersion;
    }

    int VectorRasterCache::Raster::GetPageIndex() const
    {
        return mPageIndex;
    }

    bool VectorRasterCache::Key::operator<(const Key& other) const
    {
        if (version != other.version)
            return version < other.version;

        if (scaleX != other.scaleX)
            return scaleX < other.scaleX;

        if (scaleY != other.scaleY)
            return scaleY < other.scaleY;

        return pixelSnapped < other.pixelSnapped;
    }

    VectorRasterCache::VectorRasterCache(RefCounter* refCounter):
        Singleton<VectorRasterCache>(refCounter)
    {}

    VectorRasterCache::~VectorRasterCache()
    {
        // The jobs write into the rasters: they end before the rasters go
        if (!mRasterizing.empty() && JobSystem::IsSingletonInitialzed())
            o2Jobs.WaitForIdle();
    }

    Ref<VectorRasterCache::Raster> VectorRasterCache::GetRaster(const AssetRef<VectorImageAsset>& image,
                                                                const Vec2F& scale, bool pixelSnapped)
    {
        if (!image || !image->IsValid() || !(scale.x > 0.0f) || !(scale.y > 0.0f))
            return nullptr;

        Update();

        bool scalable = image->IsMeshScalable();
        pixelSnapped = pixelSnapped && !scalable;

        Key key{ image->GetVersion(), (int)Math::Round(scale.x*256.0f), (int)Math::Round(scale.y*256.0f), pixelSnapped };
        auto found = mRasters.find(key);
        if (found != mRasters.end())
        {
            found->second->mLastUsedFrame = o2Render.GetFrameIndex();
            return found->second;
        }

        if ((int)mRasterizing.size() >= mMaxRasterizing)
            return nullptr;

        Vec2F quantizedScale((float)key.scaleX/256.0f, (float)key.scaleY/256.0f);
        auto source = image->GetMesh(quantizedScale, pixelSnapped);
        if (!source)
            return nullptr;

        Vec2F min, max;
        if (!source->GetBounds(min, max))
            return nullptr;

        Vec2I origin((int)floor(min.x*quantizedScale.x) - mPadding, (int)floor(min.y*quantizedScale.y) - mPadding);
        Vec2I end((int)ceil(max.x*quantizedScale.x) + mPadding, (int)ceil(max.y*quantizedScale.y) + mPadding);
        Vec2I size = end - origin;

        if (size.x > GetMaxRasterSize() || size.y > GetMaxRasterSize())
            return nullptr;

        int pageIndex = -1;
        RectI cell;
        if (!TakeCell(size, pageIndex, cell))
        {
            mStatistics.failures++;
            return nullptr;
        }

        auto raster = mmake<Raster>();
        raster->mPage = mPages[pageIndex].texture;
        raster->mPageIndex = pageIndex;
        raster->mCell = cell;
        raster->mScale = quantizedScale;
        raster->mOrigin = origin;
        raster->mImageVersion = key.version;
        raster->mLastUsedFrame = o2Render.GetFrameIndex();
        raster->mSource = source;
        raster->mBitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, size);

        if (source->mesh.IsScalable())
        {
            raster->mPositions.resize(source->mesh.anchors.Count());
            raster->mColors.resize(source->mesh.anchors.Count());
        }

        // The job reads the shared triangles and writes the buffers of the raster only, both kept until it ends
        Raster* target = raster.Get();
        auto draw = [target]()
        {
            Bitmap& bitmap = *target->mBitmap;
            memset(bitmap.GetData(), 0, (size_t)bitmap.GetSize().x*bitmap.GetSize().y*4);

            const VectorMesh& mesh = target->mSource->mesh;
            const Vec2F* positions = mesh.positions.Data();
            const Color32Bit* colors = mesh.colors.Data();

            if (mesh.IsScalable())
            {
                mesh.FillScaled(target->mScale, target->mPositions.data(), target->mColors.data());
                positions = target->mPositions.data();
                colors = target->mColors.data();
            }

            VectorRasterizer::Rasterize(positions, colors, mesh.indexes.Data(), (UInt)mesh.indexes.Count(),
                                        bitmap, target->mScale,
                                        Vec2F(-(float)target->mOrigin.x, -(float)target->mOrigin.y));

            VectorRasterizer::Unpremultiply(bitmap);
            target->mDrawn.Store(true);
        };

        mRasters[key] = raster;
        mRasterizing.push_back(raster);

        if (JobSystem::IsSingletonInitialzed())
            raster->mJob = o2Jobs.Schedule(draw, JobPriority::Normal, JobThread::Any);
        else
            draw();

        return raster;
    }

    void VectorRasterCache::Update()
    {
        UInt64 frame = o2Render.GetFrameIndex();
        if (frame == mUpdatedFrame)
            return;

        mUpdatedFrame = frame;

        for (size_t i = 0; i < mRasterizing.size();)
        {
            Ref<Raster> raster = mRasterizing[i];
            if (!raster->mDrawn.Load())
            {
                i++;
                continue;
            }

            if (raster->mState == Raster::State::Rasterizing)
            {
                raster->mPage->SetSubData(Vec2I(raster->mCell.left, raster->mCell.bottom), *raster->mBitmap);
                raster->mState = Raster::State::Ready;

                mStatistics.rasterizations++;
                mStatistics.rasterizedPixels += (UInt64)raster->mCell.Width()*raster->mCell.Height();
            }

            raster->mSource = nullptr;
            raster->mBitmap = nullptr;
            raster->mJob = SharedRef<Job>();
            std::vector<Vec2F>().swap(raster->mPositions);
            std::vector<Color32Bit>().swap(raster->mColors);

            mRasterizing.erase(mRasterizing.begin() + i);
        }

        if (frame%60 == 0)
            FreeUnused(mMaxFramesUnused, INT_MAX);
    }

    void VectorRasterCache::Flush()
    {
        if (JobSystem::IsSingletonInitialzed())
            o2Jobs.WaitForIdle();

        mUpdatedFrame = (UInt64)-1;
        Update();
    }

    void VectorRasterCache::Clear()
    {
        std::vector<Ref<Raster>> rasters;
        for (auto& kv : mRasters)
            rasters.push_back(kv.second);

        for (auto& raster : rasters)
            FreeRaster(raster);
    }

    int VectorRasterCache::GetPagesCount() const
    {
        return (int)mPages.size();
    }

    int VectorRasterCache::GetRastersCount() const
    {
        return (int)mRasters.size();
    }

    int VectorRasterCache::GetRasterizingCount() const
    {
        return (int)mRasterizing.size();
    }

    float VectorRasterCache::GetUsage() const
    {
        if (mPages.empty())
            return 0.0f;

        double used = 0, total = 0;
        for (auto& page : mPages)
        {
            used += page.usedArea;
            total += (double)page.texture->GetSize().x*page.texture->GetSize().y;
        }

        return (float)(used/total);
    }

    TextureRef VectorRasterCache::GetPageTexture(int index) const
    {
        if (index < 0 || index >= (int)mPages.size())
            return TextureRef();

        return mPages[index].texture;
    }

    Vector<Ref<VectorRasterCache::Raster>> VectorRasterCache::GetRasters() const
    {
        Vector<Ref<Raster>> rasters;
        for (auto& kv : mRasters)
            rasters.Add(kv.second);

        return rasters;
    }

    const VectorRasterCache::Statistics& VectorRasterCache::GetStatistics()
    {
        return mStatistics;
    }

    int VectorRasterCache::GetMaxRasterSize()
    {
        return mPageSize*3/4;
    }

    void VectorRasterCache::SetMaxPagesCount(int count)
    {
        mMaxPagesCount = Math::Max(count, 1);
    }

    int VectorRasterCache::GetMaxPagesCount()
    {
        return mMaxPagesCount;
    }

    bool VectorRasterCache::TakeCell(const Vec2I& size, int& pageIndex, RectI& cell)
    {
        auto tryPages = [&]()
        {
            for (size_t i = 0; i < mPages.size(); i++)
            {
                if (TakeCellOnPage(mPages[i], size, cell))
                {
                    pageIndex = (int)i;
                    return true;
                }
            }

            return false;
        };

        if (tryPages())
            return true;

        // Rasters not drawn for a few frames give their cells first, a new page comes when they are not enough
        while (FreeUnused(mMinFramesUnused, size.x*size.y))
        {
            if (tryPages())
                return true;
        }

        if ((int)mPages.size() >= mMaxPagesCount)
            return false;

        int pageSize = Math::Min(mPageSize, o2Render.GetMaxTextureSize().x);

        Page page;
        page.texture = TextureRef(Vec2I(pageSize, pageSize), TextureFormat::R8G8B8A8, Texture::Usage::Default);
        page.texture->SetName("Vector rasters page " + (String)(int)mPages.size());
        mPages.push_back(page);

        if (TakeCellOnPage(mPages.back(), size, cell))
        {
            pageIndex = (int)mPages.size() - 1;
            return true;
        }

        return false;
    }

    bool VectorRasterCache::TakeCellOnPage(Page& page, const Vec2I& size, RectI& cell)
    {
        Vec2I pageSize = page.texture->GetSize();
        int height = (size.y + mShelfStep - 1)/mShelfStep*mShelfStep;

        auto place = [&](Shelf& shelf, size_t at, int left)
        {
            shelf.cells.insert(shelf.cells.begin() + at, { left, size.x });

            cell.left = left;
            cell.right = left + size.x;
            cell.bottom = shelf.y;
            cell.top = shelf.y + size.y;

            page.usedArea += size.x*size.y;
            return true;
        };

        // A shelf a lot higher than the cell is left for the higher ones, unless it is empty
        for (Shelf& shelf : page.shelves)
        {
            if (shelf.height < height || (shelf.height > height*2 && !shelf.cells.empty()))
                continue;

            int left = 0;
            for (size_t i = 0; i <= shelf.cells.size(); i++)
            {
                int right = i < shelf.cells.size() ? shelf.cells[i].first : pageSize.x;
                if (right - left >= size.x)
                    return place(shelf, i, left);

                if (i < shelf.cells.size())
                    left = shelf.cells[i].first + shelf.cells[i].second;
            }
        }

        if (page.nextY + height > pageSize.y || size.x > pageSize.x)
            return false;

        Shelf shelf;
        shelf.y = page.nextY;
        shelf.height = height;
        page.shelves.push_back(shelf);
        page.nextY += height;

        return place(page.shelves.back(), 0, 0);
    }

    void VectorRasterCache::FreeCell(int pageIndex, const RectI& cell)
    {
        if (pageIndex < 0 || pageIndex >= (int)mPages.size())
            return;

        Page& page = mPages[pageIndex];
        for (Shelf& shelf : page.shelves)
        {
            if (shelf.y != cell.bottom)
                continue;

            for (size_t i = 0; i < shelf.cells.size(); i++)
            {
                if (shelf.cells[i].first == cell.left)
                {
                    shelf.cells.erase(shelf.cells.begin() + i);
                    page.usedArea -= cell.Width()*cell.Height();
                    break;
                }
            }

            break;
        }

        // Empty shelves at the top give their rows back to the free area
        while (!page.shelves.empty() && page.shelves.back().cells.empty())
        {
            page.nextY = page.shelves.back().y;
            page.shelves.pop_back();
        }
    }

    void VectorRasterCache::FreeRaster(const Ref<Raster>& raster)
    {
        if (!raster || raster->mState == Raster::State::Freed)
            return;

        FreeCell(raster->mPageIndex, raster->mCell);
        raster->mState = Raster::State::Freed;
        raster->mPage = TextureRef();

        for (auto it = mRasters.begin(); it != mRasters.end(); ++it)
        {
            if (it->second == raster)
            {
                mRasters.erase(it);
                break;
            }
        }

        mStatistics.evictions++;
    }

    bool VectorRasterCache::FreeUnused(int minFramesUnused, int area)
    {
        UInt64 frame = o2Render.GetFrameIndex();

        std::vector<Ref<Raster>> unused;
        for (auto& kv : mRasters)
        {
            const Ref<Raster>& raster = kv.second;
            if (raster->mState == Raster::State::Ready && frame >= raster->mLastUsedFrame + (UInt64)minFramesUnused)
                unused.push_back(raster);
        }

        std::sort(unused.begin(), unused.end(), [](const Ref<Raster>& a, const Ref<Raster>& b)
        {
            return a->mLastUsedFrame < b->mLastUsedFrame;
        });

        int freed = 0;
        for (auto& raster : unused)
        {
            if (freed >= area)
                break;

            freed += raster->mCell.Width()*raster->mCell.Height();
            FreeRaster(raster);
        }

        return freed > 0;
    }
}
