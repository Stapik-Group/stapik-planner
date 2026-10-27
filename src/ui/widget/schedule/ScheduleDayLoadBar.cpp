#include "ScheduleDayLoadBar.hpp"

#include <algorithm>
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
    m_levelBar.set_valign(Gtk::Align::CENTER);

    m_loadLabel.set_width_chars(LABEL_WIDTH_CHARS);
    m_loadLabel.set_xalign(1.0F);
    m_loadLabel.add_css_class("load-label");
    m_loadLabel.set_text("0%");

    set_margin_start(4);
    set_margin_end(4);
    set_margin_bottom(4);

    append(m_levelBar);
    append(m_loadLabel);
}

void ScheduleDayLoadBar::setLoad(int percent)
{
    m_levelBar.set_value(std::min(percent, MAX_LOAD));
    m_loadLabel.set_text(std::format("{}%", percent));

    if (percent >= MAX_LOAD)
        m_levelBar.add_css_class("full");
    else
        m_levelBar.remove_css_class("full");
}