#pragma once

#include "../action/MenuActionHandler.hpp"
#include "../../core/model/PlannerModel.hpp"

#include <gtkmm/applicationwindow.h>
#include <giomm/menu.h>
#include <gtkmm/popovermenubar.h>

class MainMenu
{
public:
    explicit MainMenu(Gtk::ApplicationWindow& window, PlannerModel& model);
    ~MainMenu() = default;
    Gtk::PopoverMenuBar& getMenuBar();
private:
    Gtk::ApplicationWindow& m_window;
    Glib::RefPtr<Gio::Menu> m_menuModel;
    Gtk::PopoverMenuBar m_menuBar;

    MenuActionHandler m_actionHandler;

    void buildModel();
    void initLanguageAction() const;
    void initThemeAction() const;
};