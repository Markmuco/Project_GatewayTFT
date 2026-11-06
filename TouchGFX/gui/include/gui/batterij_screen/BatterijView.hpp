#ifndef BATTERIJVIEW_HPP
#define BATTERIJVIEW_HPP

#include <gui_generated/batterij_screen/BatterijViewBase.hpp>
#include <gui/batterij_screen/BatterijPresenter.hpp>

class BatterijView : public BatterijViewBase
{
public:
    BatterijView();
    virtual ~BatterijView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // BATTERIJVIEW_HPP
