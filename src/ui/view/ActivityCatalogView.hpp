#pragma once

#include "../../core/model/PlannerModel.hpp"
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

#include <optional>
#include <string>
#include <vector>

class ActivityCatalogView : public Gtk::Box
{
public:
    explicit ActivityCatalogView(PlannerModel& model);

    sigc::signal<void(bool)>& signalCollapsedChanged();
private:
    PlannerModel& m_model;

    Box m_headerBox;
    Gtk::Label m_titleLabel;
    Gtk::Button m_addButton;
    Gtk::ToggleButton m_collapseButton;
    sigc::signal<void(bool)> m_signalCollapsedChanged;
    Gtk::ScrolledWindow m_scrolledWindow;
    Box m_listBox;

    void initLayout();
    void refreshList();
    void refreshCollapseButton();
    void onCollapseToggled();

    void onAddRequested();
    void onEditRequested(const std::string& id);
    void onDeleteRequested(const std::string& id);

    [[nodiscard]] std::vector<std::string> namesExcept(const std::optional<std::size_t>& index) const;
    void showActivityDialog(const std::optional<std::string>& editedId);
    void saveEditedActivity(Gtk::Window& window, const std::string& id, Activity activity);
};
