#include "screens/load_quiz.hpp"
#include "app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_load_quiz_screen(AppState& state)
{
    auto path_input = Input(&state.load_path_text, "path/to/quiz.yaml...");

    auto load_btn = Button(" Load ", [&] {
        if (state.load_path_text.empty())
        {
            state.status_message = "Path cannot be empty.";
            return;
        }
        if (state.load_file(state.load_path_text))
            state.current_screen = AppScreen::MENU;
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.return_to_menu();
    }, ButtonOption::Simple());

    auto action_row = Container::Horizontal({load_btn, cancel_btn});
    auto inner = Container::Vertical({path_input, action_row});

    auto component = CatchEvent(inner, [&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }
        return false;
    });

    return Renderer(component, [&, path_input, load_btn, cancel_btn] {
        Elements body;
        body.push_back(text(""));
        body.push_back(text(" Load Quiz File ") | bold | center);
        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));

        body.push_back(text(""));
        body.push_back(text(" File path") | dim);
        body.push_back(path_input->Render() | borderRounded);

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));
        body.push_back(hbox({
            filler(),
            load_btn->Render() | color(Color::Green),
            text("    "),
            cancel_btn->Render() | color(Color::RedLight),
            filler(),
        }));

        if (!state.status_message.empty())
        {
            body.push_back(text(""));
            body.push_back(text(" " + state.status_message) | color(Color::Yellow));
        }

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(" Tab switch  Enter in button loads  Esc cancel ") | dim | center);

        return vbox(std::move(body)) | vscroll_indicator | frame | flex | borderRounded;
    });
}
