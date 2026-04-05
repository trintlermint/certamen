#include "screens/quit_confirm.hpp"
#include "app.hpp"
#include "diff.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_quit_confirm_screen(AppState& state, ftxui::ScreenInteractive& screen)
{
    auto quit_btn = Button(" Quit ", [&] {
        screen.Exit();
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.return_to_menu();
    }, ButtonOption::Simple());

    auto container = Container::Horizontal({quit_btn, cancel_btn});

    container |= CatchEvent([&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(container, [&, quit_btn, cancel_btn] {
        bool has_changes = state.has_unsaved_changes();

        Elements body;
        body.push_back(text(""));

        if (has_changes)
        {
            body.push_back(text(" Quit without Saving ") | bold | center);
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(""));
            body.push_back(text(" These changes will be lost:") | dim);
            body.push_back(text(""));
            auto diff_entries = render_diff_lines(state.diff_lines);
            body.push_back(vbox(std::move(diff_entries)) | vscroll_indicator | yframe | flex);
        }
        else
        {
            body.push_back(text(" Quit? ") | bold | center);
            body.push_back(text(""));
            body.push_back(filler());
        }

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));
        body.push_back(hbox({
            filler(),
            quit_btn->Render() | color(Color::RedLight),
            text("    "),
            cancel_btn->Render(),
            filler(),
        }));
        body.push_back(text(""));

        return vbox(std::move(body)) | borderRounded;
    });
}
