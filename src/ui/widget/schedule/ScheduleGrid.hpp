#pragma once

#include "ScheduleDayColumn.hpp"
#include "../../../core/model/PlannerModel.hpp"

#include <gtkmm/grid.h>

#include <memory>
#include <vector>

class ScheduleGrid : public Gtk::Grid
{
public:
    explicit ScheduleGrid(PlannerModel& model);
private:
    static constexpr int DAYS = 7;

    PlannerModel& m_model;
    int m_slotCount;
    std::vector<std::unique_ptr<ScheduleDayColumn>> m_cells;

    void initLayout();
    void connectCellSignals() const;
};
