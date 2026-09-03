#pragma once

#include "../../core/model/PlannerModel.hpp"
#include "../view/ActivityCatalogView.hpp"
#include "../view/ScheduleView.hpp"
#include "../widget/MainMenu.hpp"

#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/separator.h>


class MainWindow : public Gtk::ApplicationWindow
{
public:
    explicit MainWindow();
    ~MainWindow() override = default;
private:
    static constexpr auto PLANNER_FILENAME = "planner.json";

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 800;
    static constexpr auto WINDOW_TITLE = "Stapik Planner";

    PlannerModel m_model;

    Gtk::Box m_mainBox;
    Gtk::Box m_contentBox; // append to main box
    MainMenu m_mainMenu; // append to main box

    ActivityCatalogView m_activityCatalogView;
    ScheduleView m_scheduleView;

    Gtk::Separator m_separator;

    void init();
    void initLayout();
    void initCloud();
};
