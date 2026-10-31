#include <gui/main_screen/MainView.hpp>
#include <gui/common/BitmapCache.hpp>
#include <images/BitmapDatabase.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // compressed images to RAM (bitmap cache)
    static const touchgfx::BitmapId ids[] = {
        BITMAP_HEADER_BG_ID,
        BITMAP_ICONS8_OK_40_ID,
        BITMAP_CARD_SQUARE_ID,
        BITMAP_ICONS8_ENERGY_100_ID,
        BITMAP_ICONS8_HOME_ALARM_100_ID,
        BITMAP_ICONS8_IDEA_100_ID,
        BITMAP_ICONS8_NATURE_100_ID,
        BITMAP_ICONS8_CAR_100_ID,
        BITMAP_ICONS8_HEATING_100_ID,
        BITMAP_ICONS8_INFO_POPUP_100_ID,
        BITMAP_ICONS8_SETTINGS_100_ID
    };
    decompressBitmaps(ids);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
    touchgfx::Bitmap::clearCache();
}
