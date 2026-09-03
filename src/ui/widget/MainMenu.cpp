#include "MainMenu.hpp"

#include "../../../cmake-build-release/_deps/stapikcommon-src/src/stapik/theme/ThemeManager.hpp"
#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <string>

MainMenu::MainMenu(Gtk::ApplicationWindow &window, PlannerModel& model) :
    m_window(window),
    m_model(model),
    m_actionHandler(window, model)
{
    m_actionHandler.registerActions();
    initLanguageAction();
    initThemeAction();
    initSlotCountAction();

    buildModel();
    LocaleManager::instance().signalLocaleChanged().connect([this] { buildModel(); });
}

Gtk::PopoverMenuBar & MainMenu::getMenuBar()
{
    return m_menuBar;
}

void MainMenu::buildModel()
{
    auto const &loc = LocaleManager::instance();

    m_menuModel = Gio::Menu::create();

    const auto menuFile = Gio::Menu::create();
    menuFile->append(loc.translate("menu.file.connect"), "win.connect");
    menuFile->append(loc.translate("menu.file.sync"), "win.sync");
    menuFile->append(loc.translate("menu.file.quit"), "win.quit");
    m_menuModel->append_submenu(loc.translate("menu.file"), menuFile);

    const auto menuHelp = Gio::Menu::create();
    menuHelp->append(loc.translate("menu.help.about"), "win.about");
    m_menuModel->append_submenu(loc.translate("menu.help"), menuHelp);

    const auto menuLanguage = Gio::Menu::create();
    menuLanguage->append(loc.translate("menu.settings.language.pl"), "win.setLanguage::pl");
    menuLanguage->append(loc.translate("menu.settings.language.en"), "win.setLanguage::en");
    menuLanguage->append(loc.translate("menu.settings.language.de"), "win.setLanguage::de");

    const auto menuTheme = Gio::Menu::create();
    menuTheme->append(loc.translate("menu.settings.theme.classic"), "win.setTheme::classic");
    menuTheme->append(loc.translate("menu.settings.theme.classicPink"), "win.setTheme::classic-pink");
    menuTheme->append(loc.translate("menu.settings.theme.modern"), "win.setTheme::modern");

    const auto menuSlots = Gio::Menu::create();
    for (int count = MIN_SLOT_OPTION; count <= MAX_SLOT_OPTION; ++count)
        menuSlots->append(std::to_string(count), "win.setSlotCount::" + std::to_string(count));

    const auto menuSettings = Gio::Menu::create();
    menuSettings->append_submenu(loc.translate("menu.settings.language"), menuLanguage);
    menuSettings->append_submenu(loc.translate("menu.settings.theme"), menuTheme);
    menuSettings->append_submenu(loc.translate("menu.settings.slots"), menuSlots);

    m_menuModel->append_submenu(loc.translate("menu.settings"), menuSettings);

    m_menuBar.set_menu_model(m_menuModel);
}

void MainMenu::initLanguageAction() const
{
    const auto initialLocale = LocaleManager::instance().getLocale();
    std::string initialValue = "pl";
    if (initialLocale == Locale::EN) initialValue = "en";
    else if (initialLocale == Locale::DE) initialValue = "de";

    auto action = Gio::SimpleAction::create_radio_string("setLanguage", initialValue);

    action->signal_activate().connect([action](const Glib::VariantBase &parameter)
    {
        using enum Locale;
        const auto value = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring> >(parameter).get();

        action->change_state(value);

        if (value == "pl") LocaleManager::instance().setLocale(PL);
        else if (value == "en") LocaleManager::instance().setLocale(EN);
        else if (value == "de") LocaleManager::instance().setLocale(DE);
    });

    m_window.add_action(action);
}

void MainMenu::initThemeAction() const
{
    const auto currentTheme = ThemeManager::instance().getTheme();
    std::string initialValue = "classic";
    if (currentTheme == Theme::Modern) initialValue = "modern";
    else if (currentTheme == Theme::ClassicPink) initialValue = "classic-pink";

    auto action = Gio::SimpleAction::create_radio_string("setTheme", initialValue);
    action->signal_activate().connect([action](const Glib::VariantBase& parameter)
    {
        using enum Theme;
        const auto value = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(parameter).get();
        action->change_state(value);
        if (value == "modern") ThemeManager::instance().setTheme(Modern);
        else if (value == "classic-pink") ThemeManager::instance().setTheme(ClassicPink);
        else ThemeManager::instance().setTheme(Classic);

    });

    m_window.add_action(action);
}

void MainMenu::initSlotCountAction() const
{
    const auto initialValue = std::to_string(m_model.settings().slots);
    auto action = Gio::SimpleAction::create_radio_string("setSlotCount", initialValue);
    action->signal_activate().connect([this, action](const Glib::VariantBase& parameter)
    {
        const auto value = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(parameter).get();
        action->change_state(value);
    
        const auto newCount = std::stoi(value.raw());
        if (m_model.updateSlotCount(newCount))
            {
                const auto& loc = LocaleManager::instance();
                showMessageDialog(m_window, loc.translate("dialog.slots.dataLost.title"),
                    loc.translate("dialog.slots.dataLost.text"), Gtk::MessageType::WARNING);
            }
        });
    m_window.add_action(action);

}
