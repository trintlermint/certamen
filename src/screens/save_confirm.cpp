#include "screens/save_confirm.hpp"
#include "app.hpp"
#include "diff.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_save_confirm_screen(AppState& state)
{
    auto save_btn = Button(" Save ", [&] {
        try
        {
            state.save_target_file();
            state.current_screen = AppScreen::MENU;
        }
        catch (const std::exception& e)
        {
            state.status_message = std::string("Save error: ") + e.what();
        }
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.return_to_menu();
    }, ButtonOption::Simple());

    auto container = Container::Horizontal({save_btn, cancel_btn});

    container |= CatchEvent([&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(container, [&, save_btn, cancel_btn] {
        auto diff_entries = render_diff_lines(state.diff_lines);

        return vbox({
            text(""),
            text(" Save ") | bold | center,
            text(""),
            separator() | color(Color::GrayDark),
            text(""),
            text(" Changes:") | dim,
            text(""),
            vbox(std::move(diff_entries)) | vscroll_indicator | frame | flex,
            text(""),
            separator() | color(Color::GrayDark),
            text(""),
            hbox({
                filler(),
                save_btn->Render() | color(Color::Green),
                text("    "),
                cancel_btn->Render() | color(Color::RedLight),
                filler(),
            }),
            text(""),
        }) | borderRounded;
    });
}
