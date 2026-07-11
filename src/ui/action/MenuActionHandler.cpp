#include "MenuActionHandler.hpp"

#include "../dialog/AboutDialog.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/storage/CloudStorageConfigStorage.hpp"
#include "stapik/cloud/CloudStorageClient.hpp"
#include "stapik/cloud/CloudStorageException.hpp"
#include "stapik/ui/dialog/ConnectDialog.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <tuple>

MenuActionHandler::MenuActionHandler(Gtk::ApplicationWindow &window, PlannerModel& model) :
    m_window(window),
    m_model(model)
{}

void MenuActionHandler::registerActions()
{
    m_window.add_action("quit", sigc::mem_fun(*this, &MenuActionHandler::onActionQuit));
    m_window.add_action("about", sigc::mem_fun(*this, &MenuActionHandler::onActionAbout));
    m_window.add_action("connect", sigc::mem_fun(*this, &MenuActionHandler::onActionConnect));
    m_window.add_action("sync", sigc::mem_fun(*this, &MenuActionHandler::onActionSync));
}

void MenuActionHandler::onActionQuit() const
{
    m_window.get_application()->quit();
}

void MenuActionHandler::onActionAbout() const
{
    showAutoDeletingDialog<AboutDialog>(m_window);
}

void MenuActionHandler::onActionConnect() const
{
    auto* dialog = new ConnectDialog(m_window);

    if (const auto config = CloudStorageConfigStorage::load("stapikplanner"); config.has_value())
        dialog->prefillConfig(config.value());

    dialog->signal_response().connect([this, dialog](const int responseId)
    {
        if (responseId == Gtk::ResponseType::OK)
        {
            if (const auto result = dialog->getResult(); result.has_value())
                handleConnectResult(result.value());
        }
        dialog->hide();
    });

    dialog->signal_hide().connect([dialog] { delete dialog; });
    dialog->show();
}

void MenuActionHandler::handleConnectResult(const CloudStorageConfig& config) const
{
    CloudStorageConfigStorage::save(config, "stapikplanner");
    applyCloudConfig(config);
}

void MenuActionHandler::applyCloudConfig(const CloudStorageConfig& config) const
{
    const auto& loc = LocaleManager::instance();

    try
    {
        auto client = std::make_unique<CloudStorageClient>(config, PLANNER_FILENAME);
        std::ignore = client->loadJson();

        m_model.setCloudClient(std::move(client));

        g_message("[Cloud] Connected: %s", config.apiUrl.c_str());
        showMessageDialog(m_window, loc.translate("cloud.connected"), loc.translate("cloud.connected.secondary"), Gtk::MessageType::INFO);
    }
    catch (const CloudStorageException& e)
    {
        g_warning("[Cloud] Cloud connection error: %s", e.what());
        showMessageDialog(m_window, loc.translate("cloud.failed.header"), e.what(), Gtk::MessageType::ERROR);
    }
}

void MenuActionHandler::onActionSync() const
{
    m_model.retrySync();
}