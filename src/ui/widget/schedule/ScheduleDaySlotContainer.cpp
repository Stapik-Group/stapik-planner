#include "ScheduleDaySlotContainer.hpp"
#include "stapik/locale/LocaleManager.hpp"

#include <glibmm/value.h>
#include <gtkmm/label.h>

#include <format>
#include <memory>

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

    initListFactory();
    initDragAndDrop();

    set_margin(2);
    append(m_dropDown);
}

void ScheduleDaySlotContainer::initListFactory()
{
    const auto factory = Gtk::SignalListItemFactory::create();

    factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& item)
    {
        auto* label = Gtk::make_managed<Gtk::Label>();
        label->set_halign(Gtk::Align::START);
        item->set_child(*label);
    });
    factory->signal_bind().connect(sigc::mem_fun(*this, &ScheduleDaySlotContainer::bindItem));
    factory->signal_unbind().connect(sigc::mem_fun(*this, &ScheduleDaySlotContainer::unbindItem));

    m_dropDown.set_list_factory(factory);
}

void ScheduleDaySlotContainer::initDragAndDrop()
{
    m_dragSource = Gtk::DragSource::create();
    m_dragSource->set_actions(Gdk::DragAction::MOVE | Gdk::DragAction::COPY);
    m_dragSource->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    m_dragSource->signal_prepare().connect([this](double, double) { return prepareDrag(); }, false);
    add_controller(m_dragSource);

    m_dropTarget = Gtk::DropTarget::create(Glib::Value<Glib::ustring>::value_type(), Gdk::DragAction::MOVE | Gdk::DragAction::COPY);
    m_dropTarget->signal_drop().connect(sigc::mem_fun(*this, &ScheduleDaySlotContainer::onDrop), false);
    m_dropTarget->signal_enter().connect([this](double, double)
    {
        add_css_class("drag-over");
        return Gdk::DragAction::MOVE;
    }, false);
    m_dropTarget->signal_leave().connect([this] { remove_css_class("drag-over"); });
    add_controller(m_dropTarget);
}

Glib::RefPtr<Gdk::ContentProvider> ScheduleDaySlotContainer::prepareDrag() const
{
    if (!getSelected().has_value())
        return {};

    const bool copy = (m_dragSource->get_current_event_state() & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK;

    Glib::Value<Glib::ustring> value;
    value.init(Glib::Value<Glib::ustring>::value_type());
    value.set(DragPayload::forSlot(m_weekday, m_index, copy).serialize());
    return Gdk::ContentProvider::create(value);
}

bool ScheduleDaySlotContainer::onDrop(const Glib::ValueBase& value, double, double)
{
    remove_css_class("drag-over");

    if (G_VALUE_TYPE(value.gobj()) != G_TYPE_STRING)
        return false;

    const gchar* text = g_value_get_string(value.gobj());
    if (text == nullptr)
        return false;

    const auto payload = DragPayload::deserialize(text);
    if (!payload.has_value())
        return false;

    m_signalDropped.emit(*payload);
    return true;
}

void ScheduleDaySlotContainer::setPosition(const Weekday weekday, const std::size_t index)
{
    m_weekday = weekday;
    m_index = index;
}

sigc::signal<void(const DragPayload&)>& ScheduleDaySlotContainer::signalDropped()
{
    return m_signalDropped;
}

void ScheduleDaySlotContainer::bindItem(const Glib::RefPtr<Gtk::ListItem>& item)
{
    auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
    const auto text = std::dynamic_pointer_cast<Gtk::StringObject>(item->get_item());
    if (label == nullptr || text == nullptr)
        return;

    label->set_text(text->get_string());
    m_boundItems[item.get()] = item;
    applyAvailability(*item);
}

void ScheduleDaySlotContainer::unbindItem(const Glib::RefPtr<Gtk::ListItem>& item)
{
    m_boundItems.erase(item.get());
}

void ScheduleDaySlotContainer::applyAvailability(Gtk::ListItem& item) const
{
    const bool selectable = isSelectableAt(item.get_position());

    if (auto* child = item.get_child())
        child->set_sensitive(selectable);

    item.set_selectable(selectable);
    item.set_activatable(selectable);
}

bool ScheduleDaySlotContainer::isSelectableAt(const guint position) const
{
    if (position == 0 || position == m_dropDown.get_selected())
        return true;

    const auto activityIndex = static_cast<std::size_t>(position - 1);
    return activityIndex < m_activities.size() && Activity::difficultyLoad(m_activities[activityIndex].difficulty) <= m_capacity;
}

void ScheduleDaySlotContainer::setCapacity(const int capacity)
{
    m_capacity = capacity;

    for (const auto& [pointer, item] : m_boundItems)
        applyAvailability(*item);
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
        m_list_model->append(std::format("{} ({}%)", activity.name, Activity::difficultyLoad(activity.difficulty)));

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
