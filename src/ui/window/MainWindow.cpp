#include "MainWindow.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

MainWindow::MainWindow(const stapik::theme::ThemeRegistry& themes) :
    m_mainBox(Gtk::Orientation::VERTICAL, 0),
    m_contentBox(Gtk::Orientation::VERTICAL, 0),
    m_mainMenu(*this, m_model, themes),
    m_activityCatalogView(m_model),
    m_scheduleView(m_model)
{
    init();
    initLayout();
    initCloud();
}

void MainWindow::init()
{
    set_title(WINDOW_TITLE);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    set_child(m_mainBox);
}

void MainWindow::initLayout()
{
    m_scheduleView.set_vexpand(true);
    m_activityCatalogView.set_size_request(-1, 200);

    m_contentBox.append(m_scheduleView);
    m_contentBox.append(m_separator);
    m_contentBox.append(m_activityCatalogView);

    m_mainBox.append(m_mainMenu.getMenuBar());
    m_mainBox.append(m_contentBox);

    m_syncStatus.set_margin(4);
    m_syncStatus.set_halign(Gtk::Align::END);
    m_mainBox.append(m_syncStatus);
}

void MainWindow::initCloud()
{
    // The indicator is only meaningful once the planner is connected to the cloud.
    m_syncStatus.set_visible(m_model.isCloudConnected());
    m_syncStatus.setStatus(m_model.cloudStatus());

    m_model.signalCloudStatusChanged().connect(sigc::mem_fun(*this, &MainWindow::onCloudStatusChanged));
    m_model.signalCloudOutcome().connect(sigc::mem_fun(*this, &MainWindow::onCloudOutcome));
    signal_close_request().connect(sigc::mem_fun(*this, &MainWindow::onCloseRequest), false);
}

// The changes are pushed in the background after a short delay, so they are sent now,
// before the window and with it the application goes away.
bool MainWindow::onCloseRequest()
{
    m_model.flushCloud();
    return false;
}

void MainWindow::onCloudStatusChanged(const stapik::sync::SyncStatus status)
{
    m_syncStatus.set_visible(true);
    m_syncStatus.setStatus(status);
}

void MainWindow::onCloudOutcome(const PlannerModel::CloudOutcome& outcome)
{
    if (outcome.state != stapik::sync::SyncState::LostRaceAcceptedServer)
        return;

    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(*this, localeManager.translate("cloud.conflict.header"), localeManager.translate("cloud.conflict.secondary"));
}
