#pragma once

#include "../../core/model/PlannerModel.hpp"

#include "stapik/cloud/CloudStorageConfig.hpp"

#include <gtkmm/applicationwindow.h>

class MenuActionHandler
{
public:
    explicit MenuActionHandler(Gtk::ApplicationWindow& window, PlannerModel& model);
    ~MenuActionHandler() = default;
    void registerActions();
private:
    static constexpr auto PLANNER_FILENAME = "planner.json";

    Gtk::ApplicationWindow& m_window;
    PlannerModel& m_model;

    void onActionQuit() const;
    void onActionAbout() const;
    void onActionConnect() const;
    void onActionSync() const;

    void handleConnectResult(const CloudStorageConfig& config) const;
    void applyCloudConfig(const CloudStorageConfig& config) const;
};