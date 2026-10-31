#include <gui/dsm_screen/DSMView.hpp>
#include <gui/common/BitmapCache.hpp>
#include <images/BitmapDatabase.hpp>

DSMView::DSMView()
{

}

void DSMView::setupScreen()
{
    DSMViewBase::setupScreen();

    // compressed images to RAM (bitmap cache)
    static const touchgfx::BitmapId ids[] = {
        BITMAP_HEADER_BG_ID,
        BITMAP_ICONS8_OK_40_ID
    };
    decompressBitmaps(ids);
}

void DSMView::tearDownScreen()
{
    DSMViewBase::tearDownScreen();
    touchgfx::Bitmap::clearCache();
}
