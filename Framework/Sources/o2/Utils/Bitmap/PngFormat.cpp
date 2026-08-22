#include "o2/stdafx.h"
#include "PngFormat.h"

#include "3rdPartyLibs/libpng/png.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/File.h"
#include "o2/Utils/Bitmap/Bitmap.h"

namespace o2
{
    void CustomPngReadFn(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
    {
        void* io_ptr = png_get_io_ptr(png_ptr);
        if (io_ptr == NULL) return;

        InFile* file = (InFile*)io_ptr;

        file->ReadData(outBytes, (UInt)byteCountToRead);
    }

    // Memory cursor for reading a PNG from a byte buffer
    struct PngMemoryReader
    {
        const UInt8* data;
        UInt         size;
        UInt         offset;
    };

    static void PngMemoryReadFn(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
    {
        void* io_ptr = png_get_io_ptr(png_ptr);
        if (io_ptr == NULL) return;

        PngMemoryReader* reader = (PngMemoryReader*)io_ptr;

        UInt available = reader->size - reader->offset;
        UInt toRead = (UInt)byteCountToRead < available ? (UInt)byteCountToRead : available;
        memcpy(outBytes, reader->data + reader->offset, toRead);
        reader->offset += toRead;

        if (toRead < (UInt)byteCountToRead)
            png_error(png_ptr, "Unexpected end of PNG data");
    }

    void CustomPngWriteFn(png_structp png_ptr, png_bytep bytes, png_size_t byteCountToWrite)
    {
        void* io_ptr = png_get_io_ptr(png_ptr);
        if (io_ptr == NULL) return;

        OutFile* file = (OutFile*)io_ptr;

        file->WriteData(bytes, (UInt)byteCountToWrite);
    }

    void CustomPngFlushFn(png_structp png_ptr) {}

    bool LoadPngImage(const String& fileName, Bitmap* image, bool errors /*= true*/)
    {
        InFile pngImageFile(fileName);
        if (!pngImageFile.IsOpened())
        {
            if (errors)
                o2Debug.LogError("Can't load PNG file '" + fileName + "'");

            return false;
        }

        UInt dataSize = pngImageFile.GetDataSize();
        UInt8* fileData = new UInt8[dataSize];
        pngImageFile.ReadData(fileData, dataSize);

        bool result = LoadPngImageFromMemory(fileData, dataSize, image, errors);
        delete[] fileData;

        if (!result && errors)
            o2Debug.LogError("Can't load PNG file '" + fileName + "'");

        return result;
    }

    bool LoadPngImageFromMemory(const UInt8* data, UInt size, Bitmap* image, bool errors /*= true*/)
    {
        //test if png
        if (size < 8 || png_sig_cmp((png_bytep)data, 0, 8))
        {
            if (errors)
                o2Debug.LogError("Can't load PNG from memory: not PNG");
            return false;
        }

        PngMemoryReader reader = { data, size, 8 };

        //create png struct
        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr)
        {
            if (errors)
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        //create png info struct
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);

            if (errors)
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        //create png info struct
        png_infop end_info = png_create_info_struct(png_ptr);
        if (!end_info)
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

            if (errors)
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        //png error stuff, not sure libpng man suggests this.
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

            if (errors)
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        //init png reading
        png_set_read_fn(png_ptr, &reader, PngMemoryReadFn);

        //let libpng know you already read the first 8 bytes
        png_set_sig_bytes(png_ptr, 8);

        // read all the info up to the image data
        png_read_info(png_ptr, info_ptr);

        //variables to pass to get info
        int bit_depth, color_type;
        png_uint_32 twidth, theight;

        // get info about png
        png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

        // Expand any source format (palette, gray, 16-bit, no-alpha) to the R8G8B8A8 rows read below
        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr);

        if (bit_depth == 16)
            png_set_strip_16(png_ptr);

        if ((color_type & PNG_COLOR_MASK_ALPHA) == 0 && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);

        // Update the png info struct.
        png_read_update_info(png_ptr, info_ptr);

        // Row size in bytes.
        int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        // Allocate the image_data as a big block, to be given to opengl
        image->Create(PixelFormat::R8G8B8A8, Vec2I(twidth, theight));
        png_byte *image_data = image->GetData();
        if (!image_data)
        {
            //clean up memory and close stuff
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

            if (errors) 
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        //row_pointers is for pointing to image_data for reading the png with libpng
        png_bytep *row_pointers = new png_bytep[theight];
        if (!row_pointers)
        {
            //clean up memory and close stuff
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            if (image_data)
                delete[] image_data;

            if (errors) 
                o2Debug.LogError("Can't load PNG from memory: TEXTURE_LOAD_ERROR");
            return false;
        }

        // set the individual row_pointers to point at the correct offsets of image_data
        for (int i = 0; i < (int)theight; ++i)
            row_pointers[theight - 1 - i] = image_data + i * rowbytes;

        //read the png into image_data through row_pointers
        png_read_image(png_ptr, row_pointers);

        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        if (row_pointers)
            delete[] row_pointers;

        return true;
    }

    bool SavePngImage(const String& fileName, const Bitmap* image)
    {
        OutFile pngImageFile(fileName);
        if (!pngImageFile.IsOpened())
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "'");
            return false;
        }

        png_structp png_ptr;
        png_infop info_ptr;

        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': png_create_write_struct failed");
            return false;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': png_create_info_struct failed");
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': Error during init_io");
            return false;
        }

        //png_init_io(png_ptr, fp);

        png_set_write_fn(png_ptr, &pngImageFile, CustomPngWriteFn, CustomPngFlushFn);

        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': Error during writing header");
            return false;
        }

        png_byte bit_depth = 8, color_type = PNG_COLOR_TYPE_RGB_ALPHA;

        png_set_IHDR(png_ptr, info_ptr, (unsigned int)image->GetSize().x, (unsigned int)image->GetSize().y,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': Error during writing bytes");
            return false;
        }

        // Row size in bytes.
        int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

        png_bytep *row_pointers = new png_bytep[(unsigned int)image->GetSize().y];

        // set the individual row_pointers to point at the correct offsets of image_data
        for (int i = 0; i < (int)image->GetSize().y; ++i)
            row_pointers[(unsigned int)image->GetSize().y - 1 - i] = (png_bytep)image->GetData() + i * rowbytes;

        png_write_image(png_ptr, row_pointers);

        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
        {
            o2Debug.LogError("Can't save PNG file '" + fileName + "': Error during end of write");
            return false;
        }

        png_write_end(png_ptr, NULL);

        if (row_pointers)
            delete[] row_pointers;

        return true;
    }

}
