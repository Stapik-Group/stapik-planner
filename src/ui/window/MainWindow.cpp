#include "MainWindow.hpp"

#include "../widget/PlanLabel.hpp"

#include "stapik/app/StapikApplication.hpp"
#include "stapik/locale/DateFormatter.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <glibmm/main.h>

#include <algorithm>
#include <format>
#include <memory>

MainWindow::MainWindow(const stapik::theme::ThemeRegistry& themes) :
    m_mainBox(Gtk::Orientation::VERTICAL, 0),
    m_paned(Gtk::Orientation::VERTICAL),
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
    updateTitle();
    m_model.signalPlansChanged().connect([this] { updateTitle(); });
    LocaleManager::instance().signalLocaleChanged().connect([this] { updateTitle(); });
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    set_child(m_mainBox);
}

void MainWindow::updateTitle()
{
    if (m_model.plans().size() < 2)
    {
        set_title(WINDOW_TITLE);
        return;
    }

    set_title(std::format("{} – {}", WINDOW_TITLE, planDisplayName(m_model.activePlanName())));
}

void MainWindow::initLayout()
{
    m_scheduleView.set_vexpand(true);

    m_paned.set_start_child(m_scheduleView);
    m_paned.set_end_child(m_activityCatalogView);
    m_paned.set_resize_start_child(true);
    m_paned.set_resize_end_child(false);
    m_paned.set_shrink_start_child(false);
    m_paned.set_shrink_end_child(false);
    m_paned.set_vexpand(true);

    m_activityCatalogView.signalCollapsedChanged().connect(sigc::mem_fun(*this, &MainWindow::onCatalogCollapsedChanged));
    signal_map().connect([this] { Glib::signal_idle().connect_once([this] { configureCatalogDivider(); }); });

    m_mainBox.append(m_mainMenu.getMenuBar());
    m_mainBox.append(m_paned);

    m_syncStatus.set_margin(4);
    m_syncStatus.set_halign(Gtk::Align::END);
    m_mainBox.append(m_syncStatus);
}

void MainWindow::configureCatalogDivider()
{
    if (m_dividerConfigured || m_paned.get_height() <= 0)
        return;

    m_dividerConfigured = true;
    m_paned.set_position(std::max(MIN_SCHEDULE_HEIGHT, m_paned.get_height() - DEFAULT_CATALOG_HEIGHT));

    if (const auto application = std::dynamic_pointer_cast<stapik::app::StapikApplication>(get_application()))
        application->rememberPanedPosition(m_paned, CATALOG_DIVIDER_NAME);

    m_expandedDividerPosition = m_paned.get_position();
}

void MainWindow::onCatalogCollapsedChanged(const bool collapsed)
{
    if (collapsed)
    {
        m_expandedDividerPosition = m_paned.get_position();
        m_paned.set_position(m_paned.get_height());
        return;
    }

    m_paned.set_position(m_expandedDividerPosition);
}

void MainWindow::initCloud()
{
    // The indicator is only meaningful once the planner is connected to the cloud.
    m_syncStatus.set_visible(m_model.isCloudConnected());
    m_syncStatus.setStatus(m_model.cloudStatus());

    m_model.signalCloudStatusChanged().connect(sigc::mem_fun(*this, &MainWindow::onCloudStatusChanged));
    m_model.signalCloudOutcome().connect(sigc::mem_fun(*this, &MainWindow::onCloudOutcome));
    m_model.signalCloudApiKeyExpiring().connect(sigc::mem_fun(*this, &MainWindow::onApiKeyExpiring));
    m_model.signalSaveFailed().connect(sigc::mem_fun(*this, &MainWindow::onSaveFailed));
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
    m_syncStatus.set_visible(m_model.isCloudConnected());
    m_syncStatus.setStatus(status);
}

void MainWindow::onSaveFailed(const std::string& message)
{
    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(*this, localeManager.translate("dialog.save.failed.title"), localeManager.translate("dialog.save.failed.text", { { "error", message } }));
}

void MainWindow::onApiKeyExpiring(const PlannerModel::TimePoint expiresAt)
{
    const auto& localeManager = LocaleManager::instance();
    const auto date = stapik::locale::formatDate(stapik::locale::toLocalTime(expiresAt), localeManager.languageCode());

    showMessageDialog(*this, localeManager.translate("cloud.apiKeyExpiring.header"), localeManager.translate("cloud.apiKeyExpiring", { { "date", date } }));
}

void MainWindow::onCloudOutcome(const PlannerModel::CloudOutcome& outcome)
{
    if (outcome.state != stapik::sync::SyncState::LostRaceAcceptedServer)
        return;

    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(*this, localeManager.translate("cloud.conflict.header"), localeManager.translate("cloud.conflict.secondary"));
}
