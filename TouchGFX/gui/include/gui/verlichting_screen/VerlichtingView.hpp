#ifndef VERLICHTINGVIEW_HPP
#define VERLICHTINGVIEW_HPP

#include <gui_generated/verlichting_screen/VerlichtingViewBase.hpp>
#include <gui/verlichting_screen/VerlichtingPresenter.hpp>

class VerlichtingView : public VerlichtingViewBase
{
public:
    VerlichtingView();
    virtual ~VerlichtingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // VERLICHTINGVIEW_HPP
