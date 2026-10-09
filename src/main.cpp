#include <gtkmm.h>

#include "app/PlannerAppInfo.hpp"
#include "ui/window/MainWindow.hpp"
#include "stapik/app/AppContext.hpp"
#include "stapik/app/SingleInstanceGuard.hpp"
#include "stapik/storage/AppPaths.hpp"
#include "stapik/theme/ThemeManager.hpp"
#include "stapik/ui/menu/StandardMenu.hpp"
#include "stapik/ui/style/AppStyleProvider.hpp"

int main(const int argc, char *argv[])
{
    const auto appInfo = makePlannerAppInfo();
    stapik::app::AppContext::initialize(appInfo);

    if (const auto singleInstanceGuard = stapik::app::SingleInstanceGuard::acquire(appInfo.applicationId, appInfo.displayName); singleInstanceGuard == nullptr)
        return EXIT_SUCCESS;

    const auto app = Gtk::Application::create(appInfo.applicationId);
    auto styleProvider = AppStyleProvider::withCommonThemes(AppPaths::resourcesDir());

    app->signal_activate().connect([&]
    {
        StandardMenu::installShortcuts(*app);

        styleProvider.apply(ThemeManager::instance().themeId());
        ThemeManager::instance().signalThemeChanged().connect([&styleProvider] { styleProvider.apply(ThemeManager::instance().themeId()); });

        auto* window = new MainWindow(styleProvider.themes());
        app->add_window(*window);
        window->show();
    });

    return app->run(argc, argv);
}
