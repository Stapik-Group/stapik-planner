#include "ActivityCatalogView.hpp"

#include "../dialog/ActivityDialog.hpp"
#include "../widget/catalog/ActivityCatalogItemRow.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

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

    for (const auto& activity : m_model.activities())
    {
        auto* row = Gtk::make_managed<ActivityCatalogItemRow>(activity);

        row->signalEditRequested().connect([this, id = activity.id] { onEditRequested(id); });
        row->signalDeleteRequested().connect([this, id = activity.id] { onDeleteRequested(id); });

        m_listBox.append(*row);
    }
}

void ActivityCatalogView::onAddRequested()
{
    showActivityDialog(std::nullopt);
}

void ActivityCatalogView::onEditRequested(const std::string& id)
{
    showActivityDialog(id);
}

void ActivityCatalogView::onDeleteRequested(const std::string& id) const
{
    if (const auto index = m_model.findActivity(id); index.has_value())
        m_model.removeActivity(*index);
}

void ActivityCatalogView::showActivityDialog(const std::optional<std::string>& editedId)
{
    auto* window = dynamic_cast<Gtk::Window*>(get_root());
    if (window == nullptr)
        return;

    const auto editedIndex = editedId.has_value() ? m_model.findActivity(*editedId) : std::nullopt;
    if (editedId.has_value() && !editedIndex.has_value())
        return;

    ActivityDialog* dialog = editedIndex.has_value()
        ? new ActivityDialog(*window, m_model.activities()[*editedIndex])
        : new ActivityDialog(*window);

    dialog->signal_response().connect([this, window, dialog, editedId](const int responseId)
    {
        if (responseId == Gtk::ResponseType::OK)
        {
            if (auto result = dialog->getResult(); result.has_value())
            {
                if (editedId.has_value())
                    saveEditedActivity(*window, *editedId, std::move(result.value()));
                else
                    m_model.addActivity(std::move(result.value()));
            }
        }
        dialog->hide();
    });

    deleteOnHide(dialog);
    dialog->show();
}

void ActivityCatalogView::saveEditedActivity(Gtk::Window& window, const std::string& id, Activity activity)
{
    const auto& localeManager = LocaleManager::instance();
    const auto index = m_model.findActivity(id);

    if (!index.has_value())
        showMessageDialog(window, localeManager.translate("dialog.activity.edit"), localeManager.translate("dialog.activity.gone"));
    else if (!m_model.updateActivity(*index, std::move(activity)))
        showMessageDialog(window, localeManager.translate("dialog.dayLimit.title"), localeManager.translate("dialog.dayLimit.activity"));
}
