#include "session.hpp"
#include "app.hpp"
#include "model.hpp"
#include "screens/quiz.hpp"
#include "screens/quiz_result.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace ftxui;

static void write_metrics(const std::string& path,
                          const std::string& player_name,
                          const std::string& quiz_file,
                          int score, int total)
{
    if (path.empty()) return;
    std::ofstream out(path);
    if (!out) return;
    out << "player=" << player_name << "\n"
        << "quiz=" << quiz_file << "\n"
        << "score=" << score << "/" << total << "\n";
}

static void run_name_prompt(std::string& player_name, ScreenInteractive& screen)
{
    auto input = Input(&player_name, "Your name...");

    auto component = Container::Vertical({input});
    component |= CatchEvent([&](Event event) {
        if (event == Event::Return && !player_name.empty())
        {
            screen.Exit();
            return true;
        }
        if (event == Event::Escape)
        {
            screen.Exit();
            return true;
        }
        return false;
    });

    auto renderer = Renderer(component, [&] {
        return vbox({
            text(""),
            text(""),
            separator() | color(Color::GrayDark),
            text(""),
            text(" Enter your name:") | dim | center,
            text(""),
            input->Render() | borderRounded | size(WIDTH, LESS_THAN, 36) | center,
            text(""),
            separator() | color(Color::GrayDark),
            text(" Enter to continue ") | dim | center,
        }) | borderRounded | center;
    });

    screen.Loop(renderer);
}

static int run_quiz_picker(const std::vector<std::string>& quiz_files,
                           const std::string& player_name,
                           ScreenInteractive& screen)
{
    int selected = 0;
    int result = -1;

    auto entries = std::make_shared<std::vector<std::string>>();
    for (const auto& f : quiz_files)
        entries->push_back("  " + std::filesystem::path(f).stem().string());

    auto menu = Menu(entries.get(), &selected);
    auto component = Container::Vertical({menu});

    component |= CatchEvent([&](Event event) {
        if (event == Event::Return)
        {
            result = selected;
            screen.Exit();
            return true;
        }
        if (event == Event::Escape || event == Event::Character('q'))
        {
            screen.Exit();
            return true;
        }
        return false;
    });

    auto renderer = Renderer(component, [&, menu, entries] {
        return vbox({
            text(""),
            text(" " + player_name) | dim | center,
            text(""),
            separator() | color(Color::GrayDark),
            text(""),
            menu->Render() | vscroll_indicator | frame | flex,
            text(""),
            separator() | color(Color::GrayDark),
            text(" Enter start  q disconnect ") | dim | center,
        }) | borderRounded | center;
    });

    screen.Loop(renderer);
    return result;
}

static std::pair<int,int> run_quiz(const std::string& quiz_file,
                                   ScreenInteractive& screen)
{
    AppState state;
    try
    {
        auto quiz = load_quiz(quiz_file);
        state.questions   = std::move(quiz.questions);
        state.quiz_name   = std::move(quiz.name);
        state.quiz_author = std::move(quiz.author);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading " << quiz_file << ": " << e.what() << "\n";
        return {0, 0};
    }
    state.start_quiz();

    auto quiz_screen = make_quiz_screen(state);
    auto result_screen = make_quiz_result_screen(state);

    int screen_index = 0;
    Components screens_vec = {quiz_screen, result_screen};
    auto tab = Container::Tab(std::move(screens_vec), &screen_index);

    auto renderer = Renderer(tab, [&] {
        if (state.current_screen == AppScreen::QUIZ)
            screen_index = 0;
        else if (state.current_screen == AppScreen::QUIZ_RESULT)
            screen_index = 1;
        else
        {
            screen.Exit();
            screen_index = 0;
        }
        return tab->Render() | size(WIDTH, LESS_THAN, 100) | center;
    });

    screen.Loop(renderer);
    return {state.quiz_score, static_cast<int>(state.quiz_session.size())};
}

int session_main(const std::vector<std::string>& quiz_files,
                 const std::string& metrics_file)
{
    if (quiz_files.empty())
    {
        std::cerr << "No quiz files provided.\n";
        return 1;
    }

    auto screen = ScreenInteractive::Fullscreen();

    std::string player_name;
    run_name_prompt(player_name, screen);
    if (player_name.empty())
        return 0;

    if (quiz_files.size() == 1)
    {
        auto [score, total] = run_quiz(quiz_files[0], screen);
        write_metrics(metrics_file, player_name, quiz_files[0], score, total);
        return 0;
    }

    while (true)
    {
        int chosen = run_quiz_picker(quiz_files, player_name, screen);
        if (chosen < 0) break;

        auto [score, total] = run_quiz(quiz_files[chosen], screen);
        write_metrics(metrics_file, player_name, quiz_files[chosen], score, total);
    }

    return 0;
}
