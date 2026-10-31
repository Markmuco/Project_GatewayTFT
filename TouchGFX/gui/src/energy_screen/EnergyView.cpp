#include <gui/energy_screen/EnergyView.hpp>
#include <gui/common/BitmapCache.hpp>
#include <images/BitmapDatabase.hpp>

EnergyView::EnergyView()
{

}

void EnergyView::setupScreen()
{
    EnergyViewBase::setupScreen();

    // compressed images to RAM (bitmap cache)
    static const touchgfx::BitmapId ids[] = {
        BITMAP_CARD_WIDE_ID,
        BITMAP_HEADER_BG_ID,
        BITMAP_ICONS8_OK_40_ID,
        BITMAP_ICONS8_LIGHTNING_BOLT_100_ID,
        BITMAP_ICONS8_COUNTER_80_ID,
        BITMAP_ICONS8_SOLAR_PANEL_100_ID,
        BITMAP_ICONS8_GAS_STATION_100_ID,
        BITMAP_ICONS8_GAS_100_ID
    };
    decompressBitmaps(ids);
}

void EnergyView::tearDownScreen()
{
    EnergyViewBase::tearDownScreen();
    touchgfx::Bitmap::clearCache();
}
