#pragma once

#include "../../core/model/PlannerModel.hpp"
#include "../view/ActivityCatalogView.hpp"
#include "../view/ScheduleView.hpp"
#include "../widget/MainMenu.hpp"

#include "stapik/sync/SyncStatus.hpp"
#include "stapik/theme/ThemeRegistry.hpp"
#include "stapik/ui/widget/StatusIndicator.hpp"

#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/separator.h>


class MainWindow : public Gtk::ApplicationWindow
{
public:
    explicit MainWindow(const stapik::theme::ThemeRegistry& themes);
    ~MainWindow() override = default;
private:
    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 800;
    static constexpr auto WINDOW_TITLE = "Stapik Planner";

    PlannerModel m_model;

    Gtk::Box m_mainBox;
    Gtk::Box m_contentBox; // append to main box
    MainMenu m_mainMenu; // append to main box
    StatusIndicator m_syncStatus; // append to main box

    ActivityCatalogView m_activityCatalogView;
    ScheduleView m_scheduleView;

    Gtk::Separator m_separator;

    void init();
    void initLayout();
    void initCloud();

    bool onCloseRequest();
    void onCloudStatusChanged(stapik::sync::SyncStatus status);
    void onCloudOutcome(const PlannerModel::CloudOutcome& outcome);
    void onApiKeyExpiring(PlannerModel::TimePoint expiresAt);
    void onSaveFailed(const std::string& message);
};
