#include "ScheduleGrid.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"

#include <gtkmm/window.h>

ScheduleGrid::ScheduleGrid(PlannerModel& model) :
    m_model(model),
    m_slotCount(model.settings().slots)
{
    for (int i = 0; i < DAYS; ++i)
        m_cells.push_back(std::make_unique<ScheduleDayColumn>(model));

    initLayout();
    connectCellSignals();

    m_model.signalActivitiesChanged().connect([this]
    {
        for (const auto& cell : m_cells)
            cell->setActivities(m_model.activities());
    });
    m_model.signalWeekPlanChanged().connect([this]
    {
        const auto& weekPlan = m_model.weekPlan();
        for (int i = 0; i < DAYS; ++i)
            m_cells[i]->setDayPlan(weekPlan[static_cast<std::size_t>(i)]);
    });
    m_model.signalSettingsChanged().connect([this]
    {
        const auto slotCount = m_model.settings().slots;
        const auto& weekPlan = m_model.weekPlan();
        for (int i = 0; i < DAYS; ++i)
        {
            m_cells[i]->setSlotCount(slotCount);
            m_cells[i]->setActivities(m_model.activities());
            m_cells[i]->setDayPlan(weekPlan[static_cast<std::size_t>(i)]);
        }
        m_slotCount = slotCount;
    });
}

void ScheduleGrid::initLayout()
{
    set_column_homogeneous(true);
    set_hexpand(true);
    set_vexpand(true);

    set_column_spacing(8);
    set_row_spacing(0);

    for (int i = 0; i < DAYS; ++i)
    {
        m_cells[i]->setActivities(m_model.activities());
        m_cells[i]->setDayPlan(m_model.weekPlan()[i]);
        attach(*m_cells[i], i, 0);
    }
}

void ScheduleGrid::connectCellSignals()
{
    for (int i = 0; i < DAYS; ++i)
        m_cells[i]->signalChanged().connect([this, i] { onDayChanged(i); });
}

void ScheduleGrid::onDayChanged(const int index)
{
    using enum Weekday;
    static constexpr std::array weekdays = {
        MONDAY, TUESDAY, WEDNESDAY, THURSDAY,
        FRIDAY, SATURDAY, SUNDAY
    };

    const auto cellIndex = static_cast<std::size_t>(index);
    if (m_model.updateDay(weekdays[cellIndex], m_cells[cellIndex]->getDayPlan()))
        return;

    m_cells[cellIndex]->setDayPlan(m_model.weekPlan()[cellIndex]);

    if (auto* window = dynamic_cast<Gtk::Window*>(get_root()))
    {
        const auto& localeManager = LocaleManager::instance();
        showMessageDialog(*window, localeManager.translate("dialog.dayLimit.title"), localeManager.translate("dialog.dayLimit.slot"));
    }
}
