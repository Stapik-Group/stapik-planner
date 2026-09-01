#include <gtkmm.h>

#include "ui/window/MainWindow.hpp"
#include "stapik/storage/AppPaths.hpp"
#include "stapik/theme/ThemeManager.hpp"
#include "infrastructure/network/CloudSchemaMigrationGuard.hpp"
#include "stapik/ui/style/AppStyleProvider.hpp"

namespace
{
    constexpr auto APP_NAME = "stapikplanner";
}

int main(const int argc, char *argv[])
{
    CloudSchemaMigrationGuard::ensureCompatible();

    const auto app = Gtk::Application::create("pl.stapik.planner");
    AppStyleProvider styleProvider(AppPaths::resourcesDir());

    app->signal_activate().connect([&]
    {
        styleProvider.apply(ThemeManager::instance(APP_NAME).getTheme());
        ThemeManager::instance().signalThemeChanged().connect([&styleProvider] { styleProvider.apply(ThemeManager::instance().getTheme()); });

        auto* window = new MainWindow();
        app->add_window(*window);
        window->show();
    });

    return app->run(argc, argv);
}
