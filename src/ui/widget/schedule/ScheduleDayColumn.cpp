#include "ScheduleDayColumn.hpp"

ScheduleDayColumn::ScheduleDayColumn(const PlannerModel& model) : Box(Gtk::Orientation::VERTICAL, 2),
    m_slotCount(model.settings().slots),
    m_slots(static_cast<size_t>(model.settings().slots))
{
    initLayout();
}

void ScheduleDayColumn::initLayout()
{
    set_hexpand(true);
    set_vexpand(true);
    add_css_class("todo-cell");

    for (auto& slot : m_slots)
    {
        slot.signalChanged().connect([this] { onSlotChanged(); });
        append(slot);
    }

    append(m_loadBar);
}

void ScheduleDayColumn::setActivities(const std::vector<Activity>& activities)
{
    for (auto& slot : m_slots)
        slot.setActivities(activities);
}

void ScheduleDayColumn::setDayPlan(const DayPlan& dayPlan)
{
    m_weekday = dayPlan.weekday;
    m_suppressChangeSignal = true;

    for (size_t i = 0; i < m_slots.size() && i < dayPlan.slots.size(); ++i)
        m_slots[i].setSelected(dayPlan.slots[i]);

    onSlotChanged();
    m_suppressChangeSignal = false;
}

DayPlan ScheduleDayColumn::getDayPlan() const
{
    DayPlan plan{m_weekday, m_slotCount};

    for (size_t i = 0; i < m_slots.size(); ++i)
        plan.slots[i] = m_slots[i].getSelected();

    return plan;
}

void ScheduleDayColumn::onSlotChanged()
{
    const int load = getDayPlan().totalLoad();

    m_loadBar.setLoad(std::min(load, LOAD_LIMIT));

    const bool limitReached = load >= LOAD_LIMIT;
    for (auto& slot : m_slots)
        slot.setEnabled(!limitReached);

    if (!m_suppressChangeSignal)
        m_signalChanged.emit();
}

sigc::signal<void()>& ScheduleDayColumn::signalChanged()
{
    return m_signalChanged;
}