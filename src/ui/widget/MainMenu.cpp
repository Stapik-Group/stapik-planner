#include "MainMenu.hpp"

#include "stapik/app/AppContext.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/update/UpdateChecker.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <string>
#include <utility>
#include <vector>

MainMenu::MainMenu(Gtk::ApplicationWindow& window, PlannerModel& model, const stapik::theme::ThemeRegistry& themes) :
    m_window(window),
    m_model(model),
    m_actionHandler(window, model),
    m_updateCheckSetting(stapik::app::AppContext::instance().settings(), stapik::update::CHECK_ENABLED_SETTING, true),
    m_standardMenu(window, StandardMenuOptions{
        .cloudItems = true,
        .undoRedoItems = true,
        .aboutItem = true,
        .themes = &themes,
        .undoStack = &model.undoStack()
    })
{
    m_actionHandler.registerActions();
    m_standardMenu.settings().addBool("settings.checkForUpdates", m_updateCheckSetting);
    initSlotCountAction();

    m_standardMenu.addToMenu(StandardMenu::Target::File, [](Gio::Menu& fileMenu) { buildDisconnectMenu(fileMenu); });
    m_standardMenu.addToMenu(StandardMenu::Target::Settings, [this](Gio::Menu& settingsMenu) { buildSlotCountMenu(settingsMenu); });
}

MainMenu::~MainMenu()
{
    m_settingsConnection.disconnect();
}

Gtk::PopoverMenuBar& MainMenu::getMenuBar()
{
    return m_standardMenu.menuBar();
}

void MainMenu::buildDisconnectMenu(Gio::Menu& fileMenu)
{
    const auto disconnectSection = Gio::Menu::create();
    disconnectSection->append(LocaleManager::instance().translate("menu.file.disconnect"), "win.disconnect");
    fileMenu.append_section(disconnectSection);
}

void MainMenu::initSlotCountAction()
{
    std::vector<stapik::ui::RadioOption<int>> options;
    for (int slotCount = MIN_SLOT_OPTION; slotCount <= MAX_SLOT_OPTION; ++slotCount)
        options.push_back({ .value = slotCount, .id = std::to_string(slotCount) });

    m_slotCountAction = std::make_unique<stapik::ui::RadioAction<int>>(
        m_window,
        "setSlotCount",
        std::move(options),
        m_model.settings().slots,
        [this](const int slotCount) { onSlotCountSelected(slotCount); });

    // The slot count also changes when a document from the cloud replaces the local one.
    m_settingsConnection = m_model.signalSettingsChanged().connect([this] { m_slotCountAction->setValue(m_model.settings().slots); });
}

void MainMenu::buildSlotCountMenu(Gio::Menu& settingsMenu) const
{
    const auto slotCountMenu = Gio::Menu::create();
    for (int slotCount = MIN_SLOT_OPTION; slotCount <= MAX_SLOT_OPTION; ++slotCount)
        slotCountMenu->append(std::to_string(slotCount), m_slotCountAction->detailedAction(slotCount));

    settingsMenu.append_submenu(LocaleManager::instance().translate("menu.settings.slots"), slotCountMenu);
}

void MainMenu::onSlotCountSelected(const int slotCount)
{
    if (!m_model.updateSlotCount(slotCount))
        return;

    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(
        m_window,
        localeManager.translate("dialog.slots.dataLost.title"),
        localeManager.translate("dialog.slots.dataLost.text"));
}
