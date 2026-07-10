#pragma once

#include <gtkmm/box.h>
#include <gtkmm/levelbar.h>
#include <gtkmm/label.h>

class ScheduleDayLoadBar : public Gtk::Box
{
public:
    explicit ScheduleDayLoadBar();
    void setLoad(int percent);
private:
    Gtk::LevelBar m_levelBar;
    Gtk::Label m_loadLabel;
    static constexpr int MAX_LOAD = 100;
    void initLayout();
};