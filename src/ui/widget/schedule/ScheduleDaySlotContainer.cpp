#include "ScheduleDaySlotContainer.hpp"

ScheduleDaySlotContainer::ScheduleDaySlotContainer() :
    Box(Gtk::Orientation::HORIZONTAL, 4)
{
    initLayout();
}

void ScheduleDaySlotContainer::initLayout()
{
    m_list_model = Gtk::StringList::create({EMPTY_LABEL});
    m_dropDown.set_model(m_list_model);
    m_dropDown.set_hexpand(true);
    m_dropDown.set_selected(0);

    m_selectionConnection = m_dropDown.property_selected().signal_changed().connect([this] { m_signalChanged.emit(); });

    set_margin(2);
    append(m_dropDown);
}

void ScheduleDaySlotContainer::setActivities(const Activities& activities)
{
    m_activities = activities;
    refreshModel(activities);
}

void ScheduleDaySlotContainer::refreshModel(const std::vector<Activity>& activities)
{
    const auto selected = getSelected();
    m_selectionConnection.block();

    while (m_list_model->get_n_items() > 0)
        m_list_model->remove(0);

    m_list_model->append(EMPTY_LABEL);
    for (const auto&[name, difficulty] : activities)
        m_list_model->append(name);

    if (selected.has_value())
    {
        for (guint i = 0; i < static_cast<guint>(activities.size()); ++i)
        {
            if (activities[i].name == selected->name)
            {
                m_dropDown.set_selected(i + 1);
                m_selectionConnection.unblock();
                return;
            }
        }
    }

    m_dropDown.set_selected(0);
    m_selectionConnection.unblock();
}

void ScheduleDaySlotContainer::setSelected(const std::optional<Activity>& activity)
{
    m_selectionConnection.block();
    if (!activity.has_value())
    {
        m_dropDown.set_selected(0);
        m_selectionConnection.unblock();
        return;
    }

    for (guint i = 0; i < static_cast<guint>(m_activities.size()); ++i)
    {
        if (m_activities[i].name == activity->name)
        {
            m_dropDown.set_selected(i + 1);
            m_selectionConnection.unblock();
            return;
        }
    }

    m_dropDown.set_selected(0);
    m_selectionConnection.unblock();
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
