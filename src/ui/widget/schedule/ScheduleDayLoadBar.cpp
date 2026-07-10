#include "ScheduleDayLoadBar.hpp"

#include <format>

ScheduleDayLoadBar::ScheduleDayLoadBar() : Box(Gtk::Orientation::HORIZONTAL, 4)
{
    initLayout();
}

void ScheduleDayLoadBar::initLayout()
{
    m_levelBar.set_min_value(0.0);
    m_levelBar.set_max_value(MAX_LOAD);
    m_levelBar.set_value(0.0);
    m_levelBar.set_hexpand(true);
    m_levelBar.add_css_class("load-bar");

    set_margin_start(4);
    set_margin_end(4);
    set_margin_bottom(4);

    append(m_levelBar);
}

void ScheduleDayLoadBar::setLoad(int percent)
{
    m_levelBar.set_value(percent);
    m_loadLabel.set_text(std::format("{}%", percent));

    if (percent >= 100)
    {
        m_levelBar.remove_offset_value("low");
        m_levelBar.remove_offset_value("high");
        m_levelBar.remove_offset_value("full");
        m_levelBar.add_offset_value("danger", 100.0);
        m_levelBar.add_css_class("full");
    }
    else
        m_levelBar.remove_css_class("full");
}