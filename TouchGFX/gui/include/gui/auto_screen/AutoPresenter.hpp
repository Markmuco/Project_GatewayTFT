#ifndef AUTOPRESENTER_HPP
#define AUTOPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class AutoView;

class AutoPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    AutoPresenter(AutoView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~AutoPresenter() {}

private:
    AutoPresenter();

    AutoView& view;
};

#endif // AUTOPRESENTER_HPP
