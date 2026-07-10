#include "ScheduleGrid.hpp"

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

void ScheduleGrid::connectCellSignals() const
{
    for (int i = 0; i < DAYS; ++i)
    {
        m_cells[i]->signalChanged().connect([this, i]
        {
            static constexpr std::array weekdays = {
                Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, Weekday::THURSDAY,
                Weekday::FRIDAY, Weekday::SATURDAY, Weekday::SUNDAY
            };

            m_model.updateDay(weekdays[static_cast<size_t>(i)], m_cells[i]->getDayPlan());
        });
    }
}
