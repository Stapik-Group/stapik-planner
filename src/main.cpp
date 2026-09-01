#include <gtkmm.h>

#include "ui/window/MainWindow.hpp"
#include "stapik/storage/AppPaths.hpp"

#include "stapik/ui/style/AppStyleProvider.hpp"
#include "infrastructure/network/CloudSchemaMigrationGuard.hpp"

int main(const int argc, char *argv[])
{
    CloudSchemaMigrationGuard::ensureCompatible();

    const auto app = Gtk::Application::create("pl.stapik.planner");
    app->signal_activate().connect([app]
    {
        AppStyleProvider::load((AppPaths::resourcesDir() / "style.css").string());

        auto* window = new MainWindow();
        app->add_window(*window);
        window->show();
    });

    return app->run(argc, argv);
}
