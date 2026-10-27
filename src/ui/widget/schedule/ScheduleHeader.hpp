#pragma once

#include "ScheduleWeekdays.hpp"

#include <gtkmm/box.h>
#include <gtkmm/label.h>

#include <array>

class ScheduleHeader : public Gtk::Box
{
public:
    explicit ScheduleHeader();

private:
    static constexpr int COLUMNS = ScheduleWeekdays::COUNT;

    std::array<Gtk::Label, COLUMNS> m_labels;

    void initLayout();
    void refreshLabels();
};