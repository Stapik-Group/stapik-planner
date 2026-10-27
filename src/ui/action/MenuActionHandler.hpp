#pragma once

#include "../../core/model/PlannerModel.hpp"

#include "stapik/cloud/CloudStorageConfig.hpp"

#include "stapik/sync/SyncStatus.hpp"

#include <giomm/simpleaction.h>
#include <gtkmm/applicationwindow.h>
#include <sigc++/connection.h>

class MenuActionHandler
{
public:
    explicit MenuActionHandler(Gtk::ApplicationWindow& window, PlannerModel& model);
    ~MenuActionHandler();

    MenuActionHandler(const MenuActionHandler&) = delete;
    MenuActionHandler& operator=(const MenuActionHandler&) = delete;

    void registerActions();
private:
    Gtk::ApplicationWindow& m_window;
    PlannerModel& m_model;
    Glib::RefPtr<Gio::SimpleAction> m_disconnectAction;
    sigc::connection m_outcomeConnection;
    sigc::connection m_statusConnection;
    bool m_connectFeedbackPending = false;

    void onActionQuit();
    void onActionConnect();
    void onActionDisconnect();
    void onActionSync();

    void handleConfirmedConfig(const CloudStorageConfig& config);
    void showConnectFeedback(const PlannerModel::CloudOutcome& outcome);
};
