#include "DialogUtils.hpp"

void showMessageDialog(Gtk::Window& parent,
                        const Glib::ustring& primaryText,
                        const Glib::ustring& secondaryText,
                        const Gtk::MessageType type)
{
    auto* dialog = new Gtk::MessageDialog(parent, primaryText, false, type, Gtk::ButtonsType::OK, true);
    dialog->set_secondary_text(secondaryText);
    dialog->signal_response().connect([dialog](int) { dialog->hide(); });
    dialog->signal_hide().connect([dialog] { delete dialog; });
    dialog->show();
}