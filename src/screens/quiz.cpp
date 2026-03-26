#include "screens/quiz.hpp"
#include "app.hpp"
#include "syntax.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <iomanip>
#include <sstream>

using namespace ftxui;

static std::string format_pct(int correct, int total)
{
    if (total == 0) return "0.0%";
    double pct = 100.0 * static_cast<double>(correct) / static_cast<double>(total);
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << pct << "%";
    return oss.str();
}

ftxui::Component make_quiz_screen(AppState& state)
{
    // Invisible placeholde needed so Container::Tab routes events here
    auto focusable = Renderer([](bool) { return text(""); });

    auto component = CatchEvent(focusable, [&](Event event) {
        if (state.quiz_session.empty()) return false;
        int idx = state.quiz_index;
        if (idx >= static_cast<int>(state.quiz_session.size())) return false;

        const auto& q = state.quiz_session[idx];
        int num_choices = static_cast<int>(q.choices.size());

        if (!state.quiz_answered)
        {
            if (event == Event::ArrowUp || event == Event::Character('k'))
            {
                if (state.quiz_selected > 0) state.quiz_selected--;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j'))
            {
                if (state.quiz_selected < num_choices - 1) state.quiz_selected++;
                return true;
            }
            if (event.is_character())
            {
                char ch = event.character()[0];
                int num = ch - '1';
                if (num >= 0 && num < num_choices)
                {
                    state.quiz_selected = num;
                    return true;
                }
            }
            if (event == Event::Return)
            {
                state.quiz_answered = true;
                state.quiz_was_correct = (state.quiz_selected == q.answer);
                if (state.quiz_was_correct) ++state.quiz_score;
                return true;
            }
        }
        else
        {
            if (event == Event::Return || event == Event::Character(' '))
            {
                state.quiz_index++;
                state.quiz_selected = 0;
                state.quiz_answered = false;
                state.quiz_was_correct = false;
                if (state.quiz_index >= static_cast<int>(state.quiz_session.size()))
                    state.current_screen = AppScreen::QUIZ_RESULT;
                return true;
            }
        }

        if (event == Event::Escape || event == Event::Character('q'))
        {
            state.current_screen = AppScreen::MENU;
            state.status_message.clear();
            return true;
        }

        return false;
    });

    return Renderer(component, [&] {
        if (state.quiz_session.empty())
            return text(" No questions present. ") | center | borderRounded;

        int idx = state.quiz_index;
        if (idx >= static_cast<int>(state.quiz_session.size()))
            return text("") | center;

        const auto& q = state.quiz_session[idx];
        int total = static_cast<int>(state.quiz_session.size());

        float progress = static_cast<float>(idx) / static_cast<float>(total);

        auto header = hbox({
            text(" Q") | dim,
            text(std::to_string(idx + 1)) | bold | color(Color::Cyan),
            text("/" + std::to_string(total)) | dim,
            filler(),
            text(std::to_string(state.quiz_score)) | bold | color(Color::Green),
            text("/" + std::to_string(total)) | dim,
            text(" (" + format_pct(state.quiz_score, total) + ") ") | dim,
        });

        Elements body;
        // progress gaugeee!!!
        if (idx > 0)
        {
            body.push_back(text(""));
            body.push_back(gauge(progress) | color(Color::Cyan) | size(HEIGHT, EQUAL, 1));
        }
        body.push_back(text(""));
        body.push_back(paragraph(" " + q.question) | bold);

        if (q.code && !q.code->empty())
        {
            body.push_back(text(""));
            body.push_back(render_code_block(*q.code, q.language));
        }

        body.push_back(text(""));

        for (int i = 0; i < static_cast<int>(q.choices.size()); ++i)
        {
            bool selected = (i == state.quiz_selected);
            bool is_correct = (i == q.answer);

            std::string marker;
            if (state.quiz_answered)
            {
                if (is_correct) marker = " + ";
                else if (selected && !state.quiz_was_correct) marker = " x ";
                else marker = "   ";
            }
            else
            {
                marker = selected ? " > " : "   ";
            }

            auto choice_el = hbox({
                text(marker),
                text(std::to_string(i + 1) + ". "),
                text(q.choices[i]),
            });

            if (state.quiz_answered && is_correct)
                choice_el = choice_el | color(Color::Green) | bold;
            else if (state.quiz_answered && selected && !state.quiz_was_correct)
                choice_el = choice_el | color(Color::RedLight);
            else if (!state.quiz_answered && selected)
                choice_el = choice_el | bold | color(Color::Cyan);
            else
                choice_el = choice_el | dim;

            body.push_back(choice_el);
        }

        if (state.quiz_answered)
        {
            body.push_back(text(""));
            if (state.quiz_was_correct)
                body.push_back(text(" Correct ") | color(Color::Green) | bold);
            else
                body.push_back(
                    text(" Incorrect  Answer: " + std::to_string(q.answer + 1) + ". " +
                         q.choices[q.answer]) | color(Color::RedLight));

            if (q.explain && !q.explain->empty())
            {
                body.push_back(text(""));
                body.push_back(paragraph(" " + *q.explain) | color(Color::Yellow));
            }

            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" Enter continue  q quit ") | dim | center);
        }
        else
        {
            body.push_back(text(""));
            body.push_back(separator() | color(Color::GrayDark));
            body.push_back(text(" j/k navigate  1-9 select  Enter submit  q quit ") | dim | center);
        }

        return vbox({
            header,
            separator() | color(Color::GrayDark),
            vbox(std::move(body)) | vscroll_indicator | frame | flex,
        }) | borderRounded;
    });
}
