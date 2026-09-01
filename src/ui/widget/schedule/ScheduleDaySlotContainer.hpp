#pragma once

#include "../../../core/model/Activity.hpp"
#include "../../../core/model/PlannerModel.hpp"

#include <gtkmm/box.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/stringlist.h>

#include <sigc++/signal.h>
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
private:
    Gtk::DropDown m_dropDown;
    Glib::RefPtr<Gtk::StringList> m_list_model;

    std::vector<Activity> m_activities;
    sigc::signal<void()> m_signalChanged;
    sigc::connection m_selectionConnection;

    static constexpr auto EMPTY_LABEL = "##########";

    void initLayout();
    void refreshModel(const std::vector<Activity>& activities);
};
