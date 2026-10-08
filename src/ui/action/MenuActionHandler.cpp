#include "MenuActionHandler.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/AboutDialog.hpp"
#include "stapik/ui/dialog/ConnectDialog.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

MenuActionHandler::MenuActionHandler(Gtk::ApplicationWindow &window, PlannerModel& model) :
    m_window(window),
    m_model(model)
{}

MenuActionHandler::~MenuActionHandler()
{
    m_outcomeConnection.disconnect();
}

void MenuActionHandler::registerActions()
{
    m_window.add_action("quit", sigc::mem_fun(*this, &MenuActionHandler::onActionQuit));
    m_window.add_action("about", sigc::mem_fun(*this, &MenuActionHandler::onActionAbout));
    m_window.add_action("connect", sigc::mem_fun(*this, &MenuActionHandler::onActionConnect));
    m_window.add_action("sync", sigc::mem_fun(*this, &MenuActionHandler::onActionSync));

    m_outcomeConnection = m_model.signalCloudOutcome().connect(sigc::mem_fun(*this, &MenuActionHandler::showConnectFeedback));
}

// Closing the window, rather than quitting the application, lets the window push the pending changes first.
void MenuActionHandler::onActionQuit()
{
    m_window.close();
}

void MenuActionHandler::onActionAbout()
{
    showAboutDialog(m_window);
}

void MenuActionHandler::onActionConnect()
{
    showConnectDialog(m_window, m_model.cloudConfig(), [this](const CloudStorageConfig& config) { handleConfirmedConfig(config); });
}

void MenuActionHandler::onActionSync()
{
    m_model.syncCloud();
}

void MenuActionHandler::handleConfirmedConfig(const CloudStorageConfig& config)
{
    const auto& localeManager = LocaleManager::instance();

    if (!m_model.connectCloud(config))
    {
        showMessageDialog(m_window, localeManager.translate("cloud.failed.header"), "", Gtk::MessageType::ERROR);
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
            showMessageDialog(m_window, localeManager.translate("cloud.connected"), localeManager.translate("cloud.connected.secondary"), Gtk::MessageType::INFO);
            break;
        case SyncState::OfflineKeptLocal:
        case SyncState::RemoteUnreadable:
            showMessageDialog(m_window, localeManager.translate("cloud.failed.header"), outcome.message, Gtk::MessageType::ERROR);
            break;
        case SyncState::LostRaceAcceptedServer:
            // The window explains this one to the user.
            break;
    }
}
