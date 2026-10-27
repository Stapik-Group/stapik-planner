#include "ScheduleDayColumn.hpp"
#include "ScheduleWeekdays.hpp"

#include "stapik/locale/LocaleManager.hpp"

#include <format>

ScheduleDayColumn::ScheduleDayColumn(const PlannerModel& model) : Box(Gtk::Orientation::VERTICAL, 2),
    m_slotCount(model.settings().slots),
    m_slots(static_cast<size_t>(model.settings().slots))
{
    initLayout();
    initContextMenu();
}

ScheduleDayColumn::~ScheduleDayColumn()
{
    m_contextMenu.unparent();
}

void ScheduleDayColumn::initLayout()
{
    set_hexpand(true);
    set_vexpand(true);
    add_css_class("todo-cell");

    for (std::size_t index = 0; index < m_slots.size(); ++index)
    {
        wireSlot(index);
        append(m_slots[index]);
    }

    append(m_loadBar);
}

void ScheduleDayColumn::wireSlot(const std::size_t index)
{
    m_slots[index].signalChanged().connect([this] { onSlotChanged(); });
    m_slots[index].signalDropped().connect([this, index](const DragPayload& payload) { m_signalDropRequested.emit(payload, index); });
}

void ScheduleDayColumn::initContextMenu()
{
    m_actions = Gio::SimpleActionGroup::create();
    for (std::size_t index = 0; index < ScheduleWeekdays::ALL.size(); ++index)
    {
        m_actions->add_action(std::format("copyTo{}", index), [this, index]
        {
            m_signalCopyRequested.emit(ScheduleWeekdays::ALL[index]);
        });
    }
    m_clearAction = m_actions->add_action("clear", [this] { m_signalClearRequested.emit(); });
    insert_action_group("dayplan", m_actions);

    m_contextMenu.set_parent(*this);
    m_contextMenu.set_has_arrow(false);
    m_contextMenu.set_halign(Gtk::Align::START);

    m_rightClick = Gtk::GestureClick::create();
    m_rightClick->set_button(GDK_BUTTON_SECONDARY);
    m_rightClick->signal_pressed().connect([this](const int, const double x, const double y) { showContextMenu(x, y); });
    add_controller(m_rightClick);
}

Glib::RefPtr<Gio::Menu> ScheduleDayColumn::buildContextMenu() const
{
    const auto& localeManager = LocaleManager::instance();

    const auto targets = Gio::Menu::create();
    for (std::size_t index = 0; index < ScheduleWeekdays::ALL.size(); ++index)
    {
        if (ScheduleWeekdays::ALL[index] != m_weekday)
            targets->append(localeManager.translate(ScheduleWeekdays::FULL_KEYS[index]), std::format("dayplan.copyTo{}", index));
    }

    const auto copySection = Gio::Menu::create();
    copySection->append_submenu(localeManager.translate("schedule.menu.copyTo"), targets);

    const auto clearSection = Gio::Menu::create();
    clearSection->append(localeManager.translate("schedule.menu.clear"), "dayplan.clear");

    const auto menu = Gio::Menu::create();
    menu->append_section(copySection);
    menu->append_section(clearSection);
    return menu;
}

void ScheduleDayColumn::showContextMenu(const double x, const double y)
{
    m_clearAction->set_enabled(!getDayPlan().isEmpty());
    m_contextMenu.set_menu_model(buildContextMenu());
    m_contextMenu.set_pointing_to(Gdk::Rectangle(static_cast<int>(x), static_cast<int>(y), 1, 1));
    m_contextMenu.popup();
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
    {
        m_slots[i].setPosition(dayPlan.weekday, i);
        m_slots[i].setSelected(dayPlan.slots[i]);
    }

    onSlotChanged();
    m_suppressChangeSignal = false;
}

void ScheduleDayColumn::setSlotCount(const int newSlotCount)
{
    if (newSlotCount == static_cast<int>(m_slots.size()))
        return;

    if (newSlotCount < static_cast<int>(m_slots.size()))
    {
        for (auto i = static_cast<std::size_t>(newSlotCount); i < m_slots.size(); ++i)
            remove(m_slots[i]);
        m_slots.resize(static_cast<std::size_t>(newSlotCount));
    }
    else
    {
        remove(m_loadBar);
        const auto oldSize = m_slots.size();
        m_slots.resize(static_cast<std::size_t>(newSlotCount));
        for (std::size_t i = oldSize; i < m_slots.size(); ++i)
        {
            wireSlot(i);
            append(m_slots[i]);
        }
        append(m_loadBar);
    }

    m_slotCount = newSlotCount;
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

    m_loadBar.setLoad(load);

    const bool limitReached = load >= DayPlan::LOAD_LIMIT;
    for (auto& slot : m_slots)
    {
        const auto selected = slot.getSelected();
        const int ownLoad = selected.has_value() ? Activity::difficultyLoad(selected->difficulty) : 0;

        slot.setCapacity(DayPlan::LOAD_LIMIT - (load - ownLoad));
        slot.setEnabled(!limitReached);
    }

    if (!m_suppressChangeSignal)
        m_signalChanged.emit();
}

sigc::signal<void()>& ScheduleDayColumn::signalChanged()
{
    return m_signalChanged;
}

sigc::signal<void(Weekday)>& ScheduleDayColumn::signalCopyRequested()
{
    return m_signalCopyRequested;
}

sigc::signal<void()>& ScheduleDayColumn::signalClearRequested()
{
    return m_signalClearRequested;
}

sigc::signal<void(const DragPayload&, std::size_t)>& ScheduleDayColumn::signalDropRequested()
{
    return m_signalDropRequested;
}