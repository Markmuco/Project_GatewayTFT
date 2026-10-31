#ifndef ENERGYVIEW_HPP
#define ENERGYVIEW_HPP

#include <gui_generated/energy_screen/EnergyViewBase.hpp>
#include <gui/energy_screen/EnergyPresenter.hpp>

class EnergyView : public EnergyViewBase
{
public:
    EnergyView();
    virtual ~EnergyView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // ENERGYVIEW_HPP
