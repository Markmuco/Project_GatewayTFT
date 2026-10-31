#ifndef DSMVIEW_HPP
#define DSMVIEW_HPP

#include <gui_generated/dsm_screen/DSMViewBase.hpp>
#include <gui/dsm_screen/DSMPresenter.hpp>

class DSMView : public DSMViewBase
{
public:
    DSMView();
    virtual ~DSMView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // DSMVIEW_HPP
