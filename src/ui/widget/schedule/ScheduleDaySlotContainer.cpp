#include "ScheduleDaySlotContainer.hpp"
#include "stapik/locale/LocaleManager.hpp"

ScheduleDaySlotContainer::ScheduleDaySlotContainer() :
    Box(Gtk::Orientation::HORIZONTAL, 4)
{
    initLayout();
}

void ScheduleDaySlotContainer::initLayout()
{
    m_list_model = Gtk::StringList::create({LocaleManager::instance().translate("schedule.slot.empty")});

    m_dropDown.set_model(m_list_model);
    m_dropDown.set_hexpand(true);
    m_dropDown.set_selected(0);

    m_selectionConnection = m_dropDown.property_selected().signal_changed().connect([this] { m_signalChanged.emit(); });
    m_localeConnection = LocaleManager::instance().signalLocaleChanged().connect([this] { refreshModel(getSelected()); });

    set_margin(2);
    append(m_dropDown);
}

void ScheduleDaySlotContainer::setActivities(const Activities& activities)
{
    // The selection is read against the previous list, before it is replaced.
    const auto selected = getSelected();
    m_activities = activities;
    refreshModel(selected);
}

void ScheduleDaySlotContainer::refreshModel(const std::optional<Activity>& selected)
{
    m_selectionConnection.block();

    while (m_list_model->get_n_items() > 0)
        m_list_model->remove(0);

    m_list_model->append(LocaleManager::instance().translate("schedule.slot.empty"));
    for (const auto& activity : m_activities)
        m_list_model->append(activity.name);

    selectActivity(selected);
    m_selectionConnection.unblock();
}

void ScheduleDaySlotContainer::setSelected(const std::optional<Activity>& activity)
{
    m_selectionConnection.block();
    selectActivity(activity);
    m_selectionConnection.unblock();
}

// Selects the entry of the activity with the same id, or the empty entry when it is not in the list.
void ScheduleDaySlotContainer::selectActivity(const std::optional<Activity>& activity)
{
    if (activity.has_value())
    {
        for (guint position = 0; position < static_cast<guint>(m_activities.size()); ++position)
        {
            if (m_activities[position].id == activity->id)
            {
                m_dropDown.set_selected(position + 1);
                return;
            }
        }
    }

    m_dropDown.set_selected(0);
}

std::optional<Activity> ScheduleDaySlotContainer::getSelected() const
{
    const auto index = m_dropDown.get_selected();
    if (index == 0 || index == GTK_INVALID_LIST_POSITION)
        return std::nullopt;

    const auto activityIndex = static_cast<size_t>(index - 1);
    if (activityIndex >= m_activities.size())
        return std::nullopt;

    return m_activities[activityIndex];
}

sigc::signal<void()>& ScheduleDaySlotContainer::signalChanged()
{
    return m_signalChanged;
}

void ScheduleDaySlotContainer::setEnabled(const bool enabled)
{
    m_dropDown.set_sensitive(enabled || getSelected().has_value());
}
