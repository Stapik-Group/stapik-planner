#pragma once

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/box.h>
#include <gtkmm/stringlist.h>
#include <gtkmm/dropdown.h>
#include <optional>
#include <string>
#include <vector>
#include "../../core/model/Activity.hpp"

class ActivityDialog : public Gtk::Dialog
{
public:
    explicit ActivityDialog(Window& parent, std::vector<std::string> otherNames);
    explicit ActivityDialog(Window& parent, const Activity& existing, std::vector<std::string> otherNames);

    std::optional<Activity> getResult() const;

private:
    Gtk::Box m_contentBox;
    Gtk::Label m_nameLabel;
    Gtk::Entry m_nameEntry;
    Gtk::Label m_nameErrorLabel;
    Gtk::Widget* m_okButton = nullptr;
    std::vector<std::string> m_otherNames;
    Gtk::Label m_difficultyLabel;
    Gtk::DropDown m_difficultyDropDown;
    Glib::RefPtr<Gtk::StringList> m_difficultyModel;

    void initLayout();
    void updateValidity();
    void setDifficulty(Difficulty d);
    Difficulty getSelectedDifficulty() const;
};