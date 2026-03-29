#include "screens/load_quiz.hpp"
#include "app.hpp"
#include "nav.hpp"
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
        state.load_file(state.load_path_text);
        state.load_path_text.clear();
    }, ButtonOption::Simple());

    auto done_btn = Button(" Done ", [&] {
        state.return_to_menu();
    }, ButtonOption::Simple());

    auto action_row = Container::Horizontal({load_btn, done_btn});
    auto input_area = Container::Vertical({path_input, action_row});
    auto focusable = Renderer([](bool) { return text(""); });

    auto inner = Container::Vertical({});

    auto component = CatchEvent(inner, [&](Event event) {
        if (event == Event::Escape)
        {
            state.return_to_menu();
            return true;
        }

        if (event == Event::Tab)
        {
            if (!state.loaded_files.empty())
                state.load_screen_mode = 1 - state.load_screen_mode;
            return true;
        }

        if (state.load_screen_mode == 0 && !state.loaded_files.empty())
        {
            int count = static_cast<int>(state.loaded_files.size());
            if (nav_up_down(event, state.load_screen_selected, count)) return true;
            if (nav_numeric(event, state.load_screen_selected, count)) return true;

            if (event == Event::ArrowRight || event == Event::Return)
            {
                state.unload_file(state.load_screen_selected);
                if (state.load_screen_selected >= static_cast<int>(state.loaded_files.size()) &&
                    state.load_screen_selected > 0)
                    state.load_screen_selected--;
                if (state.loaded_files.empty())
                    state.load_screen_mode = 1;
                return true;
            }
            return true;
        }

        return false;
    });

    return Renderer(component, [&, inner, focusable, input_area,
                                   path_input, load_btn, done_btn] {
        inner->DetachAllChildren();
        if (state.load_screen_mode == 0 && !state.loaded_files.empty())
            inner->Add(focusable);
        else
            inner->Add(input_area);

        Elements body;
        body.push_back(text(""));
        body.push_back(text(" Load / Unload Quiz Files ") | bold | center);
        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));

        if (!state.loaded_files.empty())
        {
            body.push_back(text(""));
            body.push_back(text(" Loaded files:") | dim);
            body.push_back(text(""));

            for (int i = 0; i < static_cast<int>(state.loaded_files.size()); ++i)
            {
                bool sel = (state.load_screen_mode == 0 && i == state.load_screen_selected);
                const auto& lf = state.loaded_files[i];

                int qcount = 0;
                for (const auto& q : state.questions)
                    if (q.source_file == i) qcount++;

                auto entry = hbox({
                    text(sel ? " > " : "   "),
                    text(std::to_string(i + 1) + ". ") | dim,
                    text(lf.filename) | (sel ? bold : nothing),
                    text("  " + std::to_string(qcount) + " questions") | dim,
                    sel ? (text("  unload →") | color(Color::RedLight)) : text(""),
                });
                if (sel) entry = entry | color(Color::Cyan);
                body.push_back(entry);
            }

            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
        }

        body.push_back(text(""));
        body.push_back(text(" Load new file:") | dim);
        body.push_back(path_input->Render() | borderRounded);

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(""));
        body.push_back(hbox({
            filler(),
            load_btn->Render() | color(Color::Green),
            text("    "),
            done_btn->Render() | color(Color::RedLight),
            filler(),
        }));

        if (!state.status_message.empty())
        {
            body.push_back(text(""));
            body.push_back(text(" " + state.status_message) | color(Color::Yellow));
        }

        body.push_back(text(""));
        body.push_back(separator() | color(Color::GrayDark));
        body.push_back(text(" j/k files  →/Enter unload  Tab switch  Esc done ") | dim | center);

        return vbox(std::move(body)) | vscroll_indicator | frame | flex | borderRounded;
    });
}
