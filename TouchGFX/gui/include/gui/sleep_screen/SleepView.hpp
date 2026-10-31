#ifndef SLEEPVIEW_HPP
#define SLEEPVIEW_HPP

#include <gui_generated/sleep_screen/SleepViewBase.hpp>
#include <gui/sleep_screen/SleepPresenter.hpp>

class SleepView : public SleepViewBase
{
public:
    SleepView();
    virtual ~SleepView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SLEEPVIEW_HPP
