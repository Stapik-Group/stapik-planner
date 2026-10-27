#pragma once

#include "../action/MenuActionHandler.hpp"
#include "../../core/model/PlannerModel.hpp"

#include "stapik/settings/ObservableSetting.hpp"
#include "stapik/theme/ThemeRegistry.hpp"
#include "stapik/ui/menu/RadioAction.hpp"
#include "stapik/ui/menu/StandardMenu.hpp"

#include <giomm/menu.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/popovermenubar.h>
#include <sigc++/connection.h>

#include <memory>

class MainMenu
{
public:
    explicit MainMenu(Gtk::ApplicationWindow& window, PlannerModel& model, const stapik::theme::ThemeRegistry& themes);
    ~MainMenu();

    MainMenu(const MainMenu&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;

    Gtk::PopoverMenuBar& getMenuBar();
private:
    Gtk::ApplicationWindow& m_window;
    PlannerModel& m_model;
    MenuActionHandler m_actionHandler;
    stapik::settings::ObservableSetting<bool> m_updateCheckSetting;
    StandardMenu m_standardMenu;
    std::unique_ptr<stapik::ui::RadioAction<int>> m_slotCountAction;
    sigc::connection m_settingsConnection;

    void initSlotCountAction();
    void buildSlotCountMenu(Gio::Menu& settingsMenu) const;
    static void buildDisconnectMenu(Gio::Menu& fileMenu);
    static void buildRestoreBackupMenu(Gio::Menu& fileMenu);
    void onSlotCountSelected(int slotCount);
};
