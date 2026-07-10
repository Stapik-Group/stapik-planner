#pragma once

#include "../../../core/model/Activity.hpp"

#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/button.h>
#include <sigc++/signal.h>

class ActivityCatalogItemRow : public Gtk::Box
{
public:
    explicit ActivityCatalogItemRow(const Activity& activity);

    sigc::signal<void()>& signalEditRequested();
    sigc::signal<void()>& signalDeleteRequested();

private:
    Gtk::Label m_nameLabel;
    Gtk::Label m_difficultyLabel;
    Gtk::Button m_editButton;
    Gtk::Button m_deleteButton;

    sigc::signal<void()> m_signalEditRequested;
    sigc::signal<void()> m_signalDeleteRequested;

    void initLayout(const Activity& activity);
    static std::string difficultyLabel(Difficulty d);
};