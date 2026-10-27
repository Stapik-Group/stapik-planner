#pragma once

#include "../../core/model/PlannerModel.hpp"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>

#include <optional>
#include <string>

class ActivityCatalogView : public Gtk::Box
{
public:
    explicit ActivityCatalogView(PlannerModel& model);
private:
    PlannerModel& m_model;

    Box m_headerBox;
    Gtk::Label m_titleLabel;
    Gtk::Button m_addButton;
    Gtk::ScrolledWindow m_scrolledWindow;
    Box m_listBox;

    void initLayout();
    void refreshList();

    void onAddRequested();
    void onEditRequested(const std::string& id);
    void onDeleteRequested(const std::string& id) const;

    void showActivityDialog(const std::optional<std::string>& editedId);
    void saveEditedActivity(Gtk::Window& window, const std::string& id, Activity activity);
};
