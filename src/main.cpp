#include <gtkmm.h>

#include "ui/style/AppStyleProvider.hpp"
#include "ui/window/MainWindow.hpp"
#include "stapik/storage/AppPaths.hpp"

int main(const int argc, char *argv[])
{
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
