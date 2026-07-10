#pragma once

#include <gtkmm/box.h>
#include <gtkmm/label.h>

#include <array>

class ScheduleHeader : public Gtk::Box
{
public:
    explicit ScheduleHeader();

private:
    static constexpr int COLUMNS = 7;
    static constexpr std::array<const char*, COLUMNS> DAY_KEYS = { "day.mon", "day.tue", "day.wed", "day.thu", "day.fri", "day.sat", "day.sun" };

    std::array<Gtk::Label, COLUMNS> m_labels;

    void initLayout();
    void refreshLabels();
};