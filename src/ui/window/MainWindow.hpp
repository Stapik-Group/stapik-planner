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
#include <gtkmm/paned.h>


class MainWindow : public Gtk::ApplicationWindow
{
public:
    explicit MainWindow(const stapik::theme::ThemeRegistry& themes);
    ~MainWindow() override = default;
private:
    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 800;
    static constexpr auto WINDOW_TITLE = "Stapik Planner";
    static constexpr int DEFAULT_CATALOG_HEIGHT = 330;
    static constexpr int MIN_SCHEDULE_HEIGHT = 200;
    static constexpr auto CATALOG_DIVIDER_NAME = "catalog";

    PlannerModel m_model;

    Gtk::Box m_mainBox;
    Gtk::Paned m_paned; // append to main box
    MainMenu m_mainMenu; // append to main box
    StatusIndicator m_syncStatus; // append to main box

    ActivityCatalogView m_activityCatalogView;
    ScheduleView m_scheduleView;

    int m_expandedDividerPosition = 0;
    bool m_dividerConfigured = false;

    void init();
    void updateTitle();
    void initLayout();
    void initCloud();
    void configureCatalogDivider();
    void onCatalogCollapsedChanged(bool collapsed);

    bool onCloseRequest();
    void onCloudStatusChanged(stapik::sync::SyncStatus status);
    void onCloudOutcome(const PlannerModel::CloudOutcome& outcome);
    void onApiKeyExpiring(PlannerModel::TimePoint expiresAt);
    void onSaveFailed(const std::string& message);
};
