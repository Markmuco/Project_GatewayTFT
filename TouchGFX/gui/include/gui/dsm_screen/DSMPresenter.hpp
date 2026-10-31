#ifndef DSMPRESENTER_HPP
#define DSMPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class DSMView;

class DSMPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    DSMPresenter(DSMView& v);

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

    virtual ~DSMPresenter() {}

private:
    DSMPresenter();

    DSMView& view;
};

#endif // DSMPRESENTER_HPP
