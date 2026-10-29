#pragma once

#include <map>
#include <vector>

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Jobs/Job.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Threading/Atomic.h"
#include "o2/Utils/Threading/SharedRef.h"

// Vector rasters cache access macro
#define o2VectorRasters o2::VectorRasterCache::Instance()

namespace o2
{
    // ------------------------------------------------------------------------------------------------
    // Rasters of vector images at the pixel scales they are drawn at, in the cells of atlas textures. An
    // image is rasterized by a job, its cell is placed on a page as it comes; the cells of the rasters not
    // drawn for a while are freed for the new ones
    // ------------------------------------------------------------------------------------------------
    class VectorRasterCache: public Singleton<VectorRasterCache>
    {
    public:
        // -----------------------------------------------------------
        // Raster of one image at one pixel scale, in a cell of a page
        // -----------------------------------------------------------
        class Raster: public RefCounterable
        {
        public:
            // Returns is the raster in its cell, ready to be drawn
            bool IsReady() const;

            // Returns is the raster rasterized or going to be, not freed
            bool IsAlive() const;

            // Returns page texture of the cell
            const TextureRef& GetTexture() const;

            // Returns cell in page pixels: left, bottom from the page bottom, with the transparent padding
            const RectI& GetCell() const;

            // Returns pixels per image unit by axes the image is rasterized for
            const Vec2F& GetScale() const;

            // Returns image area the cell covers: left top and right bottom corners, image space (Y down)
            void GetImageArea(Vec2F& leftTop, Vec2F& rightBottom) const;

            // Returns texture coordinates of the image point, image space
            Vec2F GetUV(const Vec2F& imagePoint) const;

            // Returns version of the image rasterized
            UInt GetImageVersion() const;

        protected:
            enum class State { Rasterizing, Ready, Freed };

            State      mState = State::Rasterizing; // State of the raster, main thread only
            TextureRef mPage;                       // Page texture of the cell
            int        mPageIndex = -1;             // Index of the page
            RectI      mCell;                       // Cell in page pixels, with padding
            Vec2F      mScale;                      // Pixels per image unit by axes
            Vec2I      mOrigin;                     // Raster pixel of the cell left top corner: image point * scale
            UInt       mImageVersion = 0;           // Version of the image rasterized
            UInt64     mLastUsedFrame = 0;          // Frame the raster was drawn last at

            Ref<VectorImageMesh> mSource;           // Triangles rasterized, kept while the job runs
            std::vector<Vec2F>      mPositions;     // Positions of a scalable mesh put for the scale
            std::vector<Color32Bit> mColors;        // Colors of a scalable mesh put for the scale
            Ref<Bitmap>          mBitmap;           // Pixels drawn by the job
            SharedRef<Job>       mJob;              // Job drawing the pixels
            Atomic<bool>         mDrawn{ false };   // Has the job drawn the pixels

            friend class VectorRasterCache;
        };

        // ------------------------------------
        // Counters of the cache since the start
        // ------------------------------------
        struct Statistics
        {
            UInt64 rasterizations = 0;   // Rasters drawn
            UInt64 rasterizedPixels = 0; // Pixels of the drawn rasters
            UInt64 evictions = 0;        // Rasters freed for the new ones or after a long time unused
            UInt64 failures = 0;         // Requests without room on the pages
        };

    public:
        // Constructor
        VectorRasterCache(RefCounter* refCounter);

        // Destructor, waits for the jobs
        ~VectorRasterCache();

        // Returns the raster of the image at the scale, schedules it when there is none; it may be not ready yet.
        // Null when the raster would be larger than the cells or there is no room for it. Pixel snapped asks for the
        // mesh with hard edges on pixel bounds
        Ref<Raster> GetRaster(const AssetRef<VectorImageAsset>& image, const Vec2F& scale, bool pixelSnapped);

        // Uploads the drawn rasters and frees the long unused ones; called once a frame by the first drawing
        void Update();

        // Waits for the rasterizing jobs and uploads their rasters
        void Flush();

        // Frees every raster that is not being rasterized
        void Clear();

        // Returns count of the pages
        int GetPagesCount() const;

        // Returns count of the alive rasters
        int GetRastersCount() const;

        // Returns count of the rasters being rasterized
        int GetRasterizingCount() const;

        // Returns part of the pages area the cells take
        float GetUsage() const;

        // Returns counters since the start
        static const Statistics& GetStatistics();

        // Returns max width and height of a raster in pixels
        static int GetMaxRasterSize();

        // Sets max count of the pages
        static void SetMaxPagesCount(int count);

        // Returns max count of the pages
        static int GetMaxPagesCount();

    protected:
        // ----------------------------------------------
        // Row of cells of one height in a page, by width
        // ----------------------------------------------
        struct Shelf
        {
            int y = 0, height = 0;
            std::vector<std::pair<int, int>> cells; // Taken spans: left, width; sorted by left
        };

        // -----------------------
        // Atlas page with shelves
        // -----------------------
        struct Page
        {
            TextureRef         texture;
            std::vector<Shelf> shelves;
            int                nextY = 0;     // Bottom of the free area above the shelves
            int                usedArea = 0;  // Area of the taken cells
        };

        struct Key
        {
            UInt version;
            int  scaleX, scaleY;
            bool pixelSnapped;

            bool operator<(const Key& other) const;
        };

        static constexpr int mPageSize = 2048;        // Width and height of a page
        static constexpr int mPadding = 1;            // Transparent pixels around a raster
        static constexpr int mShelfStep = 4;          // Shelf heights are multiples of it
        static constexpr int mMinFramesUnused = 3;    // Frames a raster is not drawn before its cell can be taken
        static constexpr int mMaxFramesUnused = 600;  // Frames a raster is kept not drawn when nothing asks its cell
        static constexpr int mMaxRasterizing = 16;    // Rasters being rasterized at once

        static int        mMaxPagesCount; // Max count of the pages
        static Statistics mStatistics;    // Counters since the start

        std::vector<Page>          mPages;       // Atlas pages
        std::map<Key, Ref<Raster>> mRasters;     // Alive rasters by image version and scale
        std::vector<Ref<Raster>>   mRasterizing; // Rasters being rasterized, kept until their jobs end
        UInt64                     mUpdatedFrame = (UInt64)-1; // Frame of the last update

    protected:
        // Takes a cell on the pages, freeing the unused rasters when there is no room; false when there is no room
        bool TakeCell(const Vec2I& size, int& pageIndex, RectI& cell);

        // Takes a cell on the page; false when there is no room
        bool TakeCellOnPage(Page& page, const Vec2I& size, RectI& cell);

        // Frees the cell on the page
        void FreeCell(int pageIndex, const RectI& cell);

        // Frees the raster and its cell
        void FreeRaster(const Ref<Raster>& raster);

        // Frees the rasters unused more than the frames count, oldest first, until the area is freed; returns is
        // anything freed
        bool FreeUnused(int minFramesUnused, int area);
    };
}
