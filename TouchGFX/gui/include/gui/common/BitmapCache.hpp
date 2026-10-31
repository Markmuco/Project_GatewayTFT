#ifndef BITMAPCACHE_HPP
#define BITMAPCACHE_HPP

#include <touchgfx/Bitmap.hpp>

/*
 * Compressed images (L8 + RLE/LZW) cannot be drawn straight from flash: they must be
 * decompressed into the bitmap cache (RAM, see TouchGFXHAL::initialize) first.
 * Call this in setupScreen() with the images of that screen and
 * touchgfx::Bitmap::clearCache() in tearDownScreen().
 * New image on a screen: add its BITMAP_..._ID to the list of that view.
 */
/* Work buffer for L8 images compressed with LZW9 (2048 bytes, 16 bit aligned), only used while decompressing */
inline uint16_t* lzwBuffer()
{
    static uint16_t buffer[1024];
    return buffer;
}

template <uint16_t N>
inline void decompressBitmaps(const touchgfx::BitmapId (&ids)[N])
{
    for (uint16_t i = 0; i < N; i++)
    {
        touchgfx::Bitmap::decompress(ids[i], lzwBuffer());
    }
}

#endif // BITMAPCACHE_HPP
