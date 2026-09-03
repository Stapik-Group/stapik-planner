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
    PlannerModel& m_model;
    Glib::RefPtr<Gio::Menu> m_menuModel;
    Gtk::PopoverMenuBar m_menuBar;

    MenuActionHandler m_actionHandler;

    static constexpr int MIN_SLOT_OPTION = 3;
    static constexpr int MAX_SLOT_OPTION = 6;

    void buildModel();
    void initLanguageAction() const;
    void initThemeAction() const;
    void initSlotCountAction() const;
};