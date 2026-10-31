#ifndef SLEEPPRESENTER_HPP
#define SLEEPPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SleepView;

class SleepPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SleepPresenter(SleepView& v);

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

    virtual ~SleepPresenter() {}

private:
    SleepPresenter();

    SleepView& view;
};

#endif // SLEEPPRESENTER_HPP
