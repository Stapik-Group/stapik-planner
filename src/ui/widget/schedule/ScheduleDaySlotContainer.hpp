#pragma once

#include "../../../core/model/Activity.hpp"
#include "../../../core/model/DayPlan.hpp"
#include "../../../core/model/DragPayload.hpp"
#include "../../../core/model/PlannerModel.hpp"

#include <gtkmm/box.h>
#include <gdkmm/contentprovider.h>
#include <gtkmm/dragsource.h>
#include <gtkmm/dropdown.h>
#include <gtkmm/droptarget.h>
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
    void setPosition(Weekday weekday, std::size_t index);
    sigc::signal<void(const DragPayload&)>& signalDropped();
private:
    std::map<Gtk::ListItem*, Glib::RefPtr<Gtk::ListItem>> m_boundItems;
    int m_capacity = DayPlan::LOAD_LIMIT;

    Gtk::DropDown m_dropDown;
    Glib::RefPtr<Gtk::StringList> m_list_model;

    std::vector<Activity> m_activities;
    Weekday m_weekday = Weekday::MONDAY;
    std::size_t m_index = 0;
    Glib::RefPtr<Gtk::DragSource> m_dragSource;
    Glib::RefPtr<Gtk::DropTarget> m_dropTarget;
    sigc::signal<void()> m_signalChanged;
    sigc::signal<void(const DragPayload&)> m_signalDropped;
    sigc::connection m_selectionConnection;

    sigc::connection m_localeConnection;

    void initLayout();
    void initListFactory();
    void initDragAndDrop();
    [[nodiscard]] Glib::RefPtr<Gdk::ContentProvider> prepareDrag() const;
    bool onDrop(const Glib::ValueBase& value, double x, double y);
    void bindItem(const Glib::RefPtr<Gtk::ListItem>& item);
    void unbindItem(const Glib::RefPtr<Gtk::ListItem>& item);
    void applyAvailability(Gtk::ListItem& item) const;
    [[nodiscard]] bool isSelectableAt(guint position) const;
    void refreshModel(const std::optional<Activity>& selected);
    void selectActivity(const std::optional<Activity>& activity);
};
