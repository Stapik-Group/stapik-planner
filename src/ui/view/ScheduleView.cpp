#include "ScheduleView.hpp"

ScheduleView::ScheduleView(PlannerModel& model): Box(Gtk::Orientation::VERTICAL, 0), m_grid(model), m_content(Gtk::Orientation::VERTICAL, 0)
{
    initLayout();
}

void ScheduleView::initLayout()
{
    set_hexpand(true);
    set_vexpand(true);

    m_content.append(m_header);
    m_content.append(m_grid);

    m_scroller.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_scroller.set_min_content_width(MIN_VIEWPORT_WIDTH);
    m_scroller.set_hexpand(true);
    m_scroller.set_vexpand(true);
    m_scroller.set_child(m_content);

    append(m_scroller);
}
