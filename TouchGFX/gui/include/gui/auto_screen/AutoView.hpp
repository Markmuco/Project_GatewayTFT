#ifndef AUTOVIEW_HPP
#define AUTOVIEW_HPP

#include <gui_generated/auto_screen/AutoViewBase.hpp>
#include <gui/auto_screen/AutoPresenter.hpp>

class AutoView : public AutoViewBase
{
public:
    AutoView();
    virtual ~AutoView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // AUTOVIEW_HPP
