#pragma once

#include "../../core/model/PlannerModel.hpp"
#include "../widget/schedule/ScheduleHeader.hpp"
#include "../widget/schedule/ScheduleGrid.hpp"

#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>

class ScheduleView : public Gtk::Box
{
public:
    explicit ScheduleView(PlannerModel& model);
private:
    static constexpr int MIN_VIEWPORT_WIDTH = 200;

    ScheduleHeader m_header;
    ScheduleGrid m_grid;
    Gtk::Box m_content;
    Gtk::ScrolledWindow m_scroller;

    void initLayout();
};
