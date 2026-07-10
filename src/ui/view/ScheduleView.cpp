#include "ScheduleView.hpp"

ScheduleView::ScheduleView(PlannerModel& model): Box(Gtk::Orientation::VERTICAL, 0), m_grid(model)
{
    initLayout();
}

void ScheduleView::initLayout()
{
    set_hexpand(true);
    set_vexpand(true);

    append(m_header);
    append(m_grid);
}