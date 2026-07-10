#include "MainMenu.hpp"

#include "../../core/locale/LocaleManager.hpp"

MainMenu::MainMenu(Gtk::ApplicationWindow &window, PlannerModel& model) :
    m_window(window),
    m_actionHandler(window, model)
{
    m_actionHandler.registerActions();
    initLanguageAction();
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

    const auto menuSettings = Gio::Menu::create();
    menuSettings->append_submenu(loc.translate("menu.settings.language"), menuLanguage);
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

