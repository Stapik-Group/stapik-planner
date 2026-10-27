#pragma once

#include "ScheduleDayColumn.hpp"
#include "ScheduleWeekdays.hpp"
#include "../../../core/model/PlannerModel.hpp"

#include <gtkmm/grid.h>

#include <memory>
#include <vector>

class ScheduleGrid : public Gtk::Grid
{
public:
    explicit ScheduleGrid(PlannerModel& model);
private:
    static constexpr int DAYS = ScheduleWeekdays::COUNT;

    PlannerModel& m_model;
    int m_slotCount;
    std::vector<std::unique_ptr<ScheduleDayColumn>> m_cells;

    void initLayout();
    void connectCellSignals();
    void onDayChanged(int index);
    void onCopyRequested(Weekday from, Weekday to);
    void copyDay(Weekday from, Weekday to);
};
