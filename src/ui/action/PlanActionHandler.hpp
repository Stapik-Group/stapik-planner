#pragma once

#include "../../core/model/PlannerModel.hpp"

#include <giomm/simpleaction.h>
#include <glibmm/refptr.h>
#include <glibmm/variant.h>
#include <gtkmm/applicationwindow.h>

#include <functional>
#include <string>

class PlanActionHandler
{
public:
    explicit PlanActionHandler(Gtk::ApplicationWindow& window, PlannerModel& model);

    PlanActionHandler(const PlanActionHandler&) = delete;
    PlanActionHandler& operator=(const PlanActionHandler&) = delete;

    void registerActions();
    void refresh();
private:
    Gtk::ApplicationWindow& m_window;
    PlannerModel& m_model;
    Glib::RefPtr<Gio::SimpleAction> m_switchAction;
    Glib::RefPtr<Gio::SimpleAction> m_deleteAction;

    void onSwitchRequested(const Glib::VariantBase& parameter);
    void onNewRequested();
    void onDuplicateRequested();
    void onRenameRequested();
    void onDeleteRequested();

    void askForName(const char* titleKey, const char* buttonKey, const std::string& initialText, std::function<bool(const std::string&)> apply);
    void showNameTaken();
};
