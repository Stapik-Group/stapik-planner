#include "MenuActionHandler.hpp"

#include "stapik/app/AppContext.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/storage/CloudStorageConfigStorage.hpp"
#include "stapik/ui/dialog/ConfirmDialog.hpp"
#include "stapik/ui/dialog/ConnectDialog.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"
#include "stapik/ui/dialog/RestoreBackupDialog.hpp"

#include <format>
#include <string>

namespace
{
    std::string connectedSecondaryText()
    {
        const auto& localeManager = LocaleManager::instance();
        const auto& applicationName = stapik::app::AppContext::instance().info().internalName;
        auto text = localeManager.translate("cloud.connected.secondary");

        if (CloudStorageConfigStorage::secretsKeptInConfigFile(applicationName).any())
            text = std::format("{}\n\n{}", text, localeManager.translate("cloud.plainFileSecrets"));

        return text;
    }
}

MenuActionHandler::MenuActionHandler(Gtk::ApplicationWindow &window, PlannerModel& model) :
    m_window(window),
    m_model(model)
{}

MenuActionHandler::~MenuActionHandler()
{
    m_outcomeConnection.disconnect();
    m_statusConnection.disconnect();
}

void MenuActionHandler::registerActions()
{
    m_window.add_action("quit", sigc::mem_fun(*this, &MenuActionHandler::onActionQuit));
    m_window.add_action("connect", sigc::mem_fun(*this, &MenuActionHandler::onActionConnect));
    m_disconnectAction = m_window.add_action("disconnect", sigc::mem_fun(*this, &MenuActionHandler::onActionDisconnect));
    m_window.add_action("sync", sigc::mem_fun(*this, &MenuActionHandler::onActionSync));
    m_window.add_action("restoreBackup", sigc::mem_fun(*this, &MenuActionHandler::onActionRestoreBackup));

    m_disconnectAction->set_enabled(m_model.isCloudConnected());
    m_statusConnection = m_model.signalCloudStatusChanged().connect([this](const stapik::sync::SyncStatus) { m_disconnectAction->set_enabled(m_model.isCloudConnected()); });
    m_outcomeConnection = m_model.signalCloudOutcome().connect(sigc::mem_fun(*this, &MenuActionHandler::showConnectFeedback));
}

// Closing the window, rather than quitting the application, lets the window push the pending changes first.
void MenuActionHandler::onActionQuit()
{
    m_window.close();
}

void MenuActionHandler::onActionConnect()
{
    showConnectDialog(m_window, m_model.cloudConfig(), [this](const CloudStorageConfig& config) { handleConfirmedConfig(config); });
}

void MenuActionHandler::onActionDisconnect()
{
    const auto& localeManager = LocaleManager::instance();

    showConfirmDialog(
        m_window,
        ConfirmDialogOptions{
            .title = localeManager.translate("dialog.disconnect.title"),
            .message = localeManager.translate("dialog.disconnect.message"),
            .confirmLabel = localeManager.translate("dialog.disconnect.button"),
            .destructive = true
        },
        [this] { m_model.disconnectCloud(); });
}

void MenuActionHandler::onActionSync()
{
    m_model.syncCloud();
}

void MenuActionHandler::onActionRestoreBackup()
{
    showRestoreBackupDialog(m_window, m_model.listBackups(), [this](const stapik::document::BackupInfo& backup) { handleChosenBackup(backup); });
}

void MenuActionHandler::handleChosenBackup(const stapik::document::BackupInfo& backup)
{
    if (m_model.restoreBackup(backup))
        return;

    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(m_window, localeManager.translate("dialog.backup.restoreFailed.title"), localeManager.translate("dialog.backup.restoreFailed.text"));
}

void MenuActionHandler::handleConfirmedConfig(const CloudStorageConfig& config)
{
    const auto& localeManager = LocaleManager::instance();

    if (!m_model.connectCloud(config))
    {
        showMessageDialog(m_window, localeManager.translate("cloud.failed.header"), "");
        return;
    }

    // The connection is checked by the first synchronization, which runs in the background.
    m_connectFeedbackPending = true;
}

void MenuActionHandler::showConnectFeedback(const PlannerModel::CloudOutcome& outcome)
{
    if (!m_connectFeedbackPending)
        return;

    m_connectFeedbackPending = false;

    const auto& localeManager = LocaleManager::instance();
    using stapik::sync::SyncState;

    switch (outcome.state)
    {
        case SyncState::Synchronized:
        case SyncState::ServerWon:
            showMessageDialog(m_window, localeManager.translate("cloud.connected"), connectedSecondaryText());
            break;
        case SyncState::OfflineKeptLocal:
        case SyncState::RemoteUnreadable:
        case SyncState::AuthRejected:
        case SyncState::DecryptionFailed:
        case SyncState::DocumentTooLarge:
            showMessageDialog(m_window, localeManager.translate("cloud.failed.header"), outcome.message);
            break;
        case SyncState::LostRaceAcceptedServer:
        case SyncState::LostRaceKeptLocal:
            break;
    }
}
