#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/Bitmap.hpp>

// RAM cache for the decompressed (L8 RLE) images of the active screen, largest screen needs ~176 KB.
// Here (not in TouchGFXHAL) so simulator and target both get it.
static uint16_t bitmapCache[200 * 1024 / 2] __attribute__((aligned(4)));

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
    // NB: FrontendApplicationBase has already opened the startup screen (Sleep, has no images).
    // A startup screen with compressed images must get its own setCache() before decompress.
    touchgfx::Bitmap::setCache(bitmapCache, sizeof(bitmapCache));
}
