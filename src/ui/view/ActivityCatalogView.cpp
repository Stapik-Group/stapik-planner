#include "ActivityCatalogView.hpp"

#include "../../core/locale/LocaleManager.hpp"
#include "../dialog/ActivityDialog.hpp"
#include "../widget/catalog/ActivityCatalogItemRow.hpp"

#include <gtkmm/window.h>


ActivityCatalogView::ActivityCatalogView(PlannerModel& model) :
    Box(Gtk::Orientation::VERTICAL, 0),
    m_model(model),
    m_headerBox(Gtk::Orientation::HORIZONTAL, 0),
    m_listBox(Gtk::Orientation::VERTICAL, 0)
{
    initLayout();
    refreshList();

    m_model.signalActivitiesChanged().connect([this] { refreshList(); });
    LocaleManager::instance().signalLocaleChanged().connect([this]
    {
        const auto& loc = LocaleManager::instance();
        m_titleLabel.set_text(loc.translate("panel.activities.title"));
        m_addButton.set_label(loc.translate("panel.activities.button.add"));
        refreshList();
    });
}

void ActivityCatalogView::initLayout()
{
    const auto& loc = LocaleManager::instance();
    m_titleLabel.set_text(loc.translate("panel.activities.title"));
    m_titleLabel.set_halign(Gtk::Align::START);
    m_titleLabel.set_hexpand(true);
    m_titleLabel.add_css_class("panel-title");

    m_addButton.set_label(loc.translate("panel.activities.button.add"));
    m_addButton.signal_clicked().connect([this] { onAddRequested(); });

    m_headerBox.set_margin(8);
    m_headerBox.append(m_titleLabel);
    m_headerBox.append(m_addButton);

    m_listBox.set_margin(8);

    m_scrolledWindow.set_child(m_listBox);
    m_scrolledWindow.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    m_scrolledWindow.set_vexpand(true);
    m_scrolledWindow.add_css_class("activity-panel-area");

    append(m_headerBox);
    append(m_scrolledWindow);
}

void ActivityCatalogView::refreshList()
{
    while (auto* child = m_listBox.get_first_child())
        m_listBox.remove(*child);

    const auto& activities = m_model.activities();
    for (int i = 0; i < static_cast<int>(activities.size()); ++i)
    {
        auto* row = Gtk::make_managed<ActivityCatalogItemRow>(activities[static_cast<std::size_t>(i)]);

        row->signalEditRequested().connect([this, i] { onEditRequested(i); });
        row->signalDeleteRequested().connect([this, i] { onDeleteRequested(i); });

        m_listBox.append(*row);
    }
}

void ActivityCatalogView::onAddRequested()
{
    showActivityDialog(-1);
}

void ActivityCatalogView::onEditRequested(const int index)
{
    showActivityDialog(index);
}

void ActivityCatalogView::onDeleteRequested(const int index) const
{
    if (index < 0 || index >= static_cast<int>(m_model.activities().size()))
        return;

    m_model.removeActivity(static_cast<std::size_t>(index));
}

void ActivityCatalogView::showActivityDialog(const int editIndex)
{
    auto* window = dynamic_cast<Gtk::Window*>(get_root());
    if (window == nullptr)
        return;

    ActivityDialog* dialog = editIndex >= 0
        ? new ActivityDialog(*window, m_model.activities()[static_cast<std::size_t>(editIndex)])
        : new ActivityDialog(*window);

    dialog->signal_response().connect([this, dialog, editIndex](const int responseId)
    {
        if (responseId == Gtk::ResponseType::OK)
        {
            if (auto result = dialog->getResult(); result.has_value())
            {
                if (editIndex >= 0)
                    m_model.updateActivity(static_cast<std::size_t>(editIndex), std::move(result.value()));
                else
                    m_model.addActivity(std::move(result.value()));
            }
        }
        dialog->hide();
    });

    dialog->signal_hide().connect([dialog] { delete dialog; });
    dialog->show();
}