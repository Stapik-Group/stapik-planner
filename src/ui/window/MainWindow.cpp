#include "MainWindow.hpp"

#include "../../../cmake-build-release/_deps/stapikcommon-src/src/stapik/storage/CloudStorageConfigStorage.hpp"

MainWindow::MainWindow() :
    m_mainBox(Gtk::Orientation::VERTICAL, 0),
    m_contentBox(Gtk::Orientation::VERTICAL, 0),
    m_mainMenu(*this, m_model),
    m_activityCatalogView(m_model),
    m_scheduleView(m_model)
{
    init();
    initLayout();
    initCloud();
}

void MainWindow::init()
{
    set_title(WINDOW_TITLE);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    set_child(m_mainBox);
}

void MainWindow::initLayout()
{
    m_scheduleView.set_vexpand(true);
    m_activityCatalogView.set_size_request(-1, 200);

    m_contentBox.append(m_scheduleView);
    m_contentBox.append(m_separator);
    m_contentBox.append(m_activityCatalogView);

    m_mainBox.append(m_mainMenu.getMenuBar());
    m_mainBox.append(m_contentBox);
}

void MainWindow::initCloud()
{
    const auto config = CloudStorageConfigStorage::load("stapikplanner");
    if (!config.has_value())
        return;
    m_model.setCloudClient(std::make_unique<CloudStorageClient>(config.value(), PLANNER_FILENAME));
}