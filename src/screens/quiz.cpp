#include "screens/quiz.hpp"
#include "app.hpp"
#include "nav.hpp"
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
    auto focusable = Renderer([](bool) { return text(""); });

    auto component = CatchEvent(focusable, [&](Event event) {
        if (state.quiz_session.empty()) return false;

        if (state.quiz_quit_pending)
        {
            if (event == Event::Return || event == Event::Character('y'))
            {
                state.quiz_quit_pending = false;
                state.return_to_menu();
                return true;
            }
            if (event == Event::Escape || event == Event::Character('n'))
            {
                state.quiz_quit_pending = false;
                return true;
            }
            return true;
        }

        int idx = state.quiz_index;
        if (idx >= static_cast<int>(state.quiz_session.size())) return false;

        const auto& q = state.quiz_session[idx];
        int num_choices = static_cast<int>(q.choices.size());

        if (!state.quiz_answered)
        {
            if (nav_up_down(event, state.quiz_selected, num_choices)) return true;
            if (nav_numeric(event, state.quiz_selected, num_choices)) return true;
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
            state.quiz_quit_pending = true;
            return true;
        }

        return false;
    });

    return Renderer(component, [&] {
        if (state.quiz_quit_pending)
        {
            return vbox({
                text(""),
                text(" Quit Quiz? ") | bold | center,
                text(""),
                separator() | color(Color::GrayDark),
                text(""),
                text(" All progress will be lost.") | dim | center,
                text(""),
                filler(),
                separator() | color(Color::GrayDark),
                text(" y/Enter quit  n/Esc cancel ") | dim | center,
            }) | borderRounded;
        }

        if (state.quiz_session.empty())
            return text(" No questions present. ") | center | borderRounded;

        int idx = state.quiz_index;
        if (idx >= static_cast<int>(state.quiz_session.size()))
            return text("") | center;

        const auto& q = state.quiz_session[idx];
        int total = static_cast<int>(state.quiz_session.size());

        float progress = static_cast<float>(idx) / static_cast<float>(total);

        Elements header_lines;

        if (!state.quiz_name.empty() || !state.quiz_author.empty())
        {
            Elements meta_row;
            if (!state.quiz_name.empty())
                meta_row.push_back(text(" " + state.quiz_name + " ") | bold);
            if (!state.quiz_author.empty())
            {
                if (!state.quiz_name.empty())
                    meta_row.push_back(text(" "));
                meta_row.push_back(text("by " + state.quiz_author) | dim);
            }
            meta_row.push_back(filler());
            header_lines.push_back(hbox(std::move(meta_row)));
            header_lines.push_back(separator() | color(Color::GrayDark));
        }

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

        Elements outer;
        for (auto& hl : header_lines)
            outer.push_back(std::move(hl));
        outer.push_back(header);
        outer.push_back(separator() | color(Color::GrayDark));
        outer.push_back(vbox(std::move(body)) | vscroll_indicator | frame | flex);

        return vbox(std::move(outer)) | borderRounded;
    });
}
