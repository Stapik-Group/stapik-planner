#pragma once

#include "../../../core/model/Activity.hpp"
#include "../../../core/model/DayPlan.hpp"
#include "../../../core/model/PlannerModel.hpp"

#include <gtkmm/box.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/listitem.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/stringobject.h>

#include <sigc++/signal.h>
#include <sigc++/connection.h>
#include <map>
#include <optional>
#include <vector>

class ScheduleDaySlotContainer : public Gtk::Box
{
public:
    explicit ScheduleDaySlotContainer();
    void setActivities(const Activities& activities);
    void setSelected(const std::optional<Activity>& activity);
    std::optional<Activity> getSelected() const;
    sigc::signal<void()>& signalChanged();
    void setEnabled(bool enabled);
    void setCapacity(int capacity);
private:
    std::map<Gtk::ListItem*, Glib::RefPtr<Gtk::ListItem>> m_boundItems;
    int m_capacity = DayPlan::LOAD_LIMIT;

    Gtk::DropDown m_dropDown;
    Glib::RefPtr<Gtk::StringList> m_list_model;

    std::vector<Activity> m_activities;
    sigc::signal<void()> m_signalChanged;
    sigc::connection m_selectionConnection;

    sigc::connection m_localeConnection;

    void initLayout();
    void initListFactory();
    void bindItem(const Glib::RefPtr<Gtk::ListItem>& item);
    void unbindItem(const Glib::RefPtr<Gtk::ListItem>& item);
    void applyAvailability(Gtk::ListItem& item) const;
    [[nodiscard]] bool isSelectableAt(guint position) const;
    void refreshModel(const std::optional<Activity>& selected);
    void selectActivity(const std::optional<Activity>& activity);
};
