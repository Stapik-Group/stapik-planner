#pragma once

#include "../../core/model/PlannerModel.hpp"
#include "../widget/schedule/ScheduleHeader.hpp"
#include "../widget/schedule/ScheduleGrid.hpp"

#include <gtkmm/box.h>

class ScheduleView : public Gtk::Box
{
public:
    explicit ScheduleView(PlannerModel& model);
private:
    ScheduleHeader m_header;
    ScheduleGrid m_grid;

    void initLayout();
};
