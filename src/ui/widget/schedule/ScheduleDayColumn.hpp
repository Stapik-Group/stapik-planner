#pragma once

#include "ScheduleDaySlotContainer.hpp"
#include "ScheduleDayLoadBar.hpp"

#include "../../../core/model/PlannerModel.hpp"
#include "../../../core/model/DayPlan.hpp"

#include <gtkmm/box.h>
#include <vector>

class ScheduleDayColumn : public Gtk::Box
{
public:
    explicit ScheduleDayColumn(const PlannerModel& model);
    void setDayPlan(const DayPlan& dayPlan);
    [[nodiscard]] DayPlan getDayPlan() const;
    sigc::signal<void()>& signalChanged();
    void setActivities(const std::vector<Activity>& activities);
private:
    static constexpr int LOAD_LIMIT = 100;

    int m_slotCount;
    bool m_suppressChangeSignal = false;
    std::vector<ScheduleDaySlotContainer> m_slots;
    ScheduleDayLoadBar m_loadBar;
    Weekday m_weekday = Weekday::MONDAY;

    sigc::signal<void()> m_signalChanged;

    void initLayout();
    void onSlotChanged();
};
