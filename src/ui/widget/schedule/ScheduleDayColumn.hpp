#pragma once

#include "ScheduleDaySlotContainer.hpp"
#include "ScheduleDayLoadBar.hpp"

#include "../../../core/model/PlannerModel.hpp"
#include "../../../core/model/DayPlan.hpp"

#include <gdkmm/rectangle.h>
#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/box.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/popovermenu.h>
#include <vector>

class ScheduleDayColumn : public Gtk::Box
{
public:
    explicit ScheduleDayColumn(const PlannerModel& model);
    ~ScheduleDayColumn() override;
    void setDayPlan(const DayPlan& dayPlan);
    [[nodiscard]] DayPlan getDayPlan() const;
    sigc::signal<void()>& signalChanged();
    sigc::signal<void(Weekday)>& signalCopyRequested();
    void setActivities(const std::vector<Activity>& activities);
    void setSlotCount(int newSlotCount);
private:
    int m_slotCount;
    bool m_suppressChangeSignal = false;
    std::vector<ScheduleDaySlotContainer> m_slots;
    ScheduleDayLoadBar m_loadBar;
    Weekday m_weekday = Weekday::MONDAY;

    Gtk::PopoverMenu m_contextMenu;
    Glib::RefPtr<Gio::SimpleActionGroup> m_actions;
    Glib::RefPtr<Gtk::GestureClick> m_rightClick;

    sigc::signal<void()> m_signalChanged;
    sigc::signal<void(Weekday)> m_signalCopyRequested;

    void initLayout();
    void initContextMenu();
    void showContextMenu(double x, double y);
    [[nodiscard]] Glib::RefPtr<Gio::Menu> buildContextMenu() const;
    void onSlotChanged();
};
