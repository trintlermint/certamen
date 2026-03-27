#include "screens/set_metadata.hpp"
#include "app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

ftxui::Component make_set_metadata_screen(AppState& state)
{
    auto name_input   = Input(&state.meta_name_text, "Quiz name...");
    auto author_input = Input(&state.meta_author_text, "Author...");

    auto save_btn = Button(" Save ", [&] {
        state.quiz_name   = state.meta_name_text;
        state.quiz_author = state.meta_author_text;
        state.status_message = "Metadata updated.";
        state.current_screen = AppScreen::MENU;
    }, ButtonOption::Simple());

    auto cancel_btn = Button(" Cancel ", [&] {
        state.current_screen = AppScreen::MENU;
        state.status_message.clear();
    }, ButtonOption::Simple());

    auto action_row = Container::Horizontal({save_btn, cancel_btn});

    auto inner = Container::Vertical({name_input, author_input, action_row});

    auto component = CatchEvent(inner, [&](Event event) {
        if (event == Event::Escape)
        {
            state.current_screen = AppScreen::MENU;
            state.status_message.clear();
            return true;
        }
        return false;
    });

    return Renderer(component, [&, name_input, author_input, save_btn, cancel_btn] {
        Elements body;
        body.push_back(text(""));
        body.push_back(text(" Set Author and Name ") | bold | center);
        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));

        body.push_back(text(""));
        body.push_back(text(" Quiz Name") | dim);
        body.push_back(name_input->Render() | borderRounded);

        body.push_back(text(""));
        body.push_back(text(" Author") | dim);
        body.push_back(author_input->Render() | borderRounded);

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));
        body.push_back(hbox({
            filler(),
            save_btn->Render() | color(Color::Green),
            text("    "),
            cancel_btn->Render() | color(Color::RedLight),
            filler(),
        }));

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(" Tab switch fields  Enter in button saves  Esc cancel ") | dim | center);

        return vbox(std::move(body)) | vscroll_indicator | frame | flex | borderRounded;
    });
}
