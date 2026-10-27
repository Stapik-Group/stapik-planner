#include "PlanActionHandler.hpp"

#include "../widget/PlanLabel.hpp"

#include "stapik/locale/LocaleManager.hpp"
#include "stapik/ui/dialog/ConfirmDialog.hpp"
#include "stapik/ui/dialog/DialogUtils.hpp"
#include "stapik/ui/dialog/TextInputDialog.hpp"

#include <utility>

PlanActionHandler::PlanActionHandler(Gtk::ApplicationWindow& window, PlannerModel& model) :
    m_window(window),
    m_model(model)
{}

void PlanActionHandler::registerActions()
{
    m_switchAction = Gio::SimpleAction::create_radio_string("switchPlan", Glib::ustring(m_model.activePlanId()));
    m_switchAction->signal_activate().connect(sigc::mem_fun(*this, &PlanActionHandler::onSwitchRequested));
    m_window.add_action(m_switchAction);

    m_window.add_action("newPlan", sigc::mem_fun(*this, &PlanActionHandler::onNewRequested));
    m_window.add_action("duplicatePlan", sigc::mem_fun(*this, &PlanActionHandler::onDuplicateRequested));
    m_window.add_action("renamePlan", sigc::mem_fun(*this, &PlanActionHandler::onRenameRequested));
    m_deleteAction = m_window.add_action("deletePlan", sigc::mem_fun(*this, &PlanActionHandler::onDeleteRequested));

    refresh();
}

void PlanActionHandler::refresh()
{
    m_switchAction->change_state(Glib::ustring(m_model.activePlanId()));
    m_deleteAction->set_enabled(m_model.plans().size() > 1);
}

void PlanActionHandler::onSwitchRequested(const Glib::VariantBase& parameter)
{
    const auto id = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(parameter).get();
    m_model.switchPlan(id.raw());
}

void PlanActionHandler::onNewRequested()
{
    askForName("dialog.plan.new.title", "dialog.plan.new.button", {}, [this](const std::string& name) { return m_model.createPlan(name); });
}

void PlanActionHandler::onDuplicateRequested()
{
    askForName("dialog.plan.duplicate.title", "dialog.plan.duplicate.button", {}, [this](const std::string& name) { return m_model.duplicatePlan(name); });
}

void PlanActionHandler::onRenameRequested()
{
    askForName(
        "dialog.plan.rename.title",
        "dialog.plan.rename.button",
        m_model.activePlanName(),
        [this, id = m_model.activePlanId()](const std::string& name) { return m_model.renamePlan(id, name); });
}

void PlanActionHandler::onDeleteRequested()
{
    const auto& localeManager = LocaleManager::instance();
    const LocalizationEngine::Arguments arguments = { { "name", planDisplayName(m_model.activePlanName()) } };

    showConfirmDialog(
        m_window,
        ConfirmDialogOptions{
            .title = localeManager.translate("dialog.plan.delete.title", arguments),
            .message = localeManager.translate("dialog.plan.delete.message"),
            .confirmLabel = localeManager.translate("dialog.plan.delete.button"),
            .destructive = true
        },
        [this, id = m_model.activePlanId()] { m_model.deletePlan(id); });
}

void PlanActionHandler::askForName(const char* titleKey, const char* buttonKey, const std::string& initialText, std::function<bool(const std::string&)> apply)
{
    const auto& localeManager = LocaleManager::instance();

    showTextInputDialog(
        m_window,
        TextInputDialogOptions{
            .title = localeManager.translate(titleKey),
            .label = localeManager.translate("dialog.plan.name.label"),
            .initialText = initialText,
            .placeholder = localeManager.translate("dialog.plan.name.placeholder"),
            .confirmLabel = localeManager.translate(buttonKey)
        },
        [this, apply = std::move(apply)](const std::string& name)
        {
            if (!apply(name))
                showNameTaken();
        });
}

void PlanActionHandler::showNameTaken()
{
    const auto& localeManager = LocaleManager::instance();
    showMessageDialog(m_window, localeManager.translate("dialog.plan.taken.title"), localeManager.translate("dialog.plan.taken.text"));
}
