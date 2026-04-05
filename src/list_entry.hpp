#ifndef CERTAMEN_LIST_ENTRY_HPP
#define CERTAMEN_LIST_ENTRY_HPP

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>
#include <memory>
#include <vector>

using EntryBoxes = std::shared_ptr<std::vector<ftxui::Box>>;

inline EntryBoxes make_entry_boxes()
{
    return std::make_shared<std::vector<ftxui::Box>>();
}

inline ftxui::Element list_entry(ftxui::Element el, bool selected,
                                 EntryBoxes& boxes, int i)
{
    if (selected) el = el | ftxui::color(ftxui::Color::Cyan) | ftxui::focus;
    return el | ftxui::reflect((*boxes)[i]);
}

inline int mouse_click_index(ftxui::Event& event, const EntryBoxes& boxes)
{
    if (!event.is_mouse() ||
        event.mouse().button != ftxui::Mouse::Left ||
        event.mouse().motion != ftxui::Mouse::Pressed)
        return -1;
    for (int i = 0; i < static_cast<int>(boxes->size()); ++i)
        if ((*boxes)[i].Contain(event.mouse().x, event.mouse().y))
            return i;
    return -1;
}

#endif
