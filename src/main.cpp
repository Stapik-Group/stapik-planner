#include <gtkmm.h>

#include "app/PlannerAppInfo.hpp"
#include "ui/window/MainWindow.hpp"
#include "stapik/app/AppContext.hpp"
#include "stapik/app/SingleInstanceGuard.hpp"
#include "stapik/storage/AppPaths.hpp"
#include "stapik/theme/ThemeManager.hpp"
#include "infrastructure/network/CloudSchemaMigrationGuard.hpp"
#include "stapik/ui/style/AppStyleProvider.hpp"

int main(const int argc, char *argv[])
{
    const auto appInfo = makePlannerAppInfo();
    stapik::app::AppContext::initialize(appInfo);

    if (const auto singleInstanceGuard = stapik::app::SingleInstanceGuard::acquire(appInfo.applicationId, appInfo.displayName); singleInstanceGuard == nullptr)
        return EXIT_SUCCESS;

    CloudSchemaMigrationGuard::ensureCompatible();

    const auto app = Gtk::Application::create(appInfo.applicationId);
    AppStyleProvider styleProvider(AppPaths::resourcesDir());

    app->signal_activate().connect([&]
    {
        styleProvider.apply(ThemeManager::instance().getTheme());
        ThemeManager::instance().signalThemeChanged().connect([&styleProvider] { styleProvider.apply(ThemeManager::instance().getTheme()); });

        auto* window = new MainWindow();
        app->add_window(*window);
        window->show();
    });

    return app->run(argc, argv);
}
