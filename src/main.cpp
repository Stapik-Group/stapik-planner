#include "app/PlannerAppInfo.hpp"
#include "ui/window/MainWindow.hpp"

#include "stapik/app/StapikApplication.hpp"

int main(const int argc, char *argv[])
{
    const auto app = stapik::app::StapikApplication::create(makePlannerAppInfo(), { .checkForUpdates = true });

    app->signal_activate().connect([&app]
    {
        if (auto* existingWindow = app->get_active_window())
        {
            existingWindow->present();
            return;
        }

        auto* window = new MainWindow(app->themes());
        app->add_window(*window);
        window->show();
    });

    return app->run(argc, argv);
}
