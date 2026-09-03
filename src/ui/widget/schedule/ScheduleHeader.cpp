#include "ScheduleHeader.hpp"

#include "stapik/locale/LocaleManager.hpp"

ScheduleHeader::ScheduleHeader() : Box(Gtk::Orientation::HORIZONTAL, 0)
{
    initLayout();
    refreshLabels();
    LocaleManager::instance().signalLocaleChanged().connect([this] { refreshLabels(); });
}

void ScheduleHeader::initLayout()
{
    add_css_class("todo-header");
    set_hexpand(true);

    for (int i = 0; i < COLUMNS; ++i)
    {
        m_labels[i].set_halign(Gtk::Align::CENTER);
        m_labels[i].set_hexpand(true);
        m_labels[i].set_margin(8);

        append(m_labels[i]);
    }
}

void ScheduleHeader::refreshLabels()
{
    auto const& loc = LocaleManager::instance();
    for (int i = 0; i < COLUMNS; ++i)
        m_labels[i].set_text(loc.translate(DAY_KEYS[i]));
}