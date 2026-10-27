#include "MainMenu.hpp"

#include "PlanLabel.hpp"

#include "stapik/app/AppContext.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/update/UpdateChecker.hpp"
#include "stapik/text/Collation.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <glibmm/main.h>

#include <algorithm>
#include <format>

#include <string>
#include <utility>
#include <vector>

MainMenu::MainMenu(Gtk::ApplicationWindow& window, PlannerModel& model, const stapik::theme::ThemeRegistry& themes) :
    m_window(window),
    m_model(model),
    m_actionHandler(window, model),
    m_planActions(window, model),
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
    m_planActions.registerActions();
    m_plansConnection = m_model.signalPlansChanged().connect([this] { onPlansChanged(); });
    m_standardMenu.settings().addBool("settings.checkForUpdates", m_updateCheckSetting);
    initSlotCountAction();

    m_standardMenu.addMenu("menu.plan", [this](Gio::Menu& planMenu) { buildPlanMenu(planMenu); });
    m_standardMenu.addToMenu(StandardMenu::Target::File, [](Gio::Menu& fileMenu) { buildDisconnectMenu(fileMenu); });
    m_standardMenu.addToMenu(StandardMenu::Target::File, [](Gio::Menu& fileMenu) { buildRestoreBackupMenu(fileMenu); });
    m_standardMenu.addToMenu(StandardMenu::Target::Settings, [this](Gio::Menu& settingsMenu) { buildSlotCountMenu(settingsMenu); });
}

MainMenu::~MainMenu()
{
    m_settingsConnection.disconnect();
    m_plansConnection.disconnect();
    m_rebuildConnection.disconnect();
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

void MainMenu::buildPlanMenu(Gio::Menu& planMenu) const
{
    const auto& localeManager = LocaleManager::instance();

    auto plans = m_model.plans();
    std::ranges::sort(plans, stapik::text::TextLess(localeManager.languageCode()), [](const PlannerModel::PlanSummary& plan)
    {
        return planDisplayName(plan.name);
    });

    const auto planSection = Gio::Menu::create();
    for (const auto& plan : plans)
        planSection->append(planDisplayName(plan.name), std::format("win.switchPlan::{}", plan.id));
    planMenu.append_section(planSection);

    const auto manageSection = Gio::Menu::create();
    manageSection->append(localeManager.translate("menu.plan.new"), "win.newPlan");
    manageSection->append(localeManager.translate("menu.plan.duplicate"), "win.duplicatePlan");
    manageSection->append(localeManager.translate("menu.plan.rename"), "win.renamePlan");
    manageSection->append(localeManager.translate("menu.plan.delete"), "win.deletePlan");
    planMenu.append_section(manageSection);
}

void MainMenu::onPlansChanged()
{
    m_planActions.refresh();
    scheduleMenuRebuild();
}

void MainMenu::scheduleMenuRebuild()
{
    if (m_rebuildConnection.connected())
        return;

    m_rebuildConnection = Glib::signal_idle().connect([this]
    {
        m_standardMenu.rebuild();
        return false;
    });
}

void MainMenu::buildRestoreBackupMenu(Gio::Menu& fileMenu)
{
    const auto backupSection = Gio::Menu::create();
    backupSection->append(LocaleManager::instance().translate("menu.file.restoreBackup"), "win.restoreBackup");
    fileMenu.append_section(backupSection);
}

void MainMenu::initSlotCountAction()
{
    std::vector<stapik::ui::RadioOption<int>> options;
    for (int slotCount = Settings::MIN_SLOTS; slotCount <= Settings::MAX_SLOTS; ++slotCount)
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
    for (int slotCount = Settings::MIN_SLOTS; slotCount <= Settings::MAX_SLOTS; ++slotCount)
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
