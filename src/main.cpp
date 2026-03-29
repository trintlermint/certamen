#include "app.hpp"
#include "serve.hpp"
#include "session.hpp"
#include "screens/menu.hpp"
#include "screens/quiz.hpp"
#include "screens/quiz_result.hpp"
#include "screens/add_question.hpp"
#include "screens/remove_question.hpp"
#include "screens/change_answer.hpp"
#include "screens/edit_choice.hpp"
#include "screens/list_questions.hpp"
#include "screens/set_metadata.hpp"
#include "screens/save_confirm.hpp"
#include "screens/quit_confirm.hpp"
#include "screens/load_quiz.hpp"
#include "screens/quiz_setup.hpp"
#include "screens/pick_file.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace ftxui;

static void print_usage(const char* prog)
{
    std::cout << "Usage:\n"
              << "  " << prog << " [quiz.yaml ...]           Local TUI mode\n"
              << "  " << prog << " serve [options] <files>   SSH server mode\n"
              << "\nServe options:\n"
              << "  --port <N>          Listen port (default: 2222)\n"
              << "  --password <pw>     Require password (default: open)\n"
              << "  --key <path>        Host key (default: certamen_host_rsa)\n"
              << "  --max-clients <N>   Max concurrent clients (default: 8)\n"
              << "\nExamples:\n"
              << "  " << prog << " example_quiz.yaml\n"
              << "  " << prog << " serve --port 2222 --password secret quiz.yaml\n"
              << "  ssh -p 2222 quiz@localhost\n";
}

static int run_local(const std::vector<std::string>& files)
{
    AppState state;

    for (const auto& f : files)
        state.load_file(f);

    if (files.empty())
        state.status_message = "No file loaded. Use 'Load Quiz File' to open a quiz.";
    else if (files.size() > 1)
        state.status_message = "Loaded " + std::to_string(files.size()) +
                               " files (" + std::to_string(state.questions.size()) +
                               " questions total).";

    auto screen = ScreenInteractive::Fullscreen();

    auto menu_screen            = make_menu_screen(state);
    auto quiz_screen            = make_quiz_screen(state);
    auto quiz_result_screen     = make_quiz_result_screen(state);
    auto add_question_screen    = make_add_question_screen(state);
    auto remove_question_screen = make_remove_question_screen(state);
    auto change_answer_screen   = make_change_answer_screen(state);
    auto edit_choice_screen     = make_edit_choice_screen(state);
    auto list_questions_screen  = make_list_questions_screen(state);
    auto set_metadata_screen    = make_set_metadata_screen(state);
    auto save_confirm_screen    = make_save_confirm_screen(state);
    auto quit_confirm_screen    = make_quit_confirm_screen(state, screen);
    auto load_quiz_screen       = make_load_quiz_screen(state);
    auto quiz_setup_screen      = make_quiz_setup_screen(state);
    auto pick_file_screen       = make_pick_file_screen(state);

    int screen_index = 0;
    Components screens = {
        menu_screen,
        quiz_screen,
        quiz_result_screen,
        add_question_screen,
        remove_question_screen,
        change_answer_screen,
        edit_choice_screen,
        list_questions_screen,
        set_metadata_screen,
        save_confirm_screen,
        quit_confirm_screen,
        load_quiz_screen,
        quiz_setup_screen,
        pick_file_screen,
    };
    auto tab = Container::Tab(std::move(screens), &screen_index);

    auto main_renderer = Renderer(tab, [&] {
        screen_index = static_cast<int>(state.current_screen);
        return tab->Render() | size(WIDTH, LESS_THAN, 120) | center;
    });

    main_renderer |= CatchEvent([&](Event event) {
        if (state.current_screen == AppScreen::MENU)
        {
            if (event == Event::Character('r') || event == Event::Character('R'))
            {
                state.randomise = !state.randomise;
                state.status_message = std::string("Randomise is now ") +
                                       (state.randomise ? "ON" : "OFF") + ".";
                return true;
            }
            if (event == Event::Character('q'))
            {
                state.compute_diff();
                state.current_screen = AppScreen::QUIT_CONFIRM;
                return true;
            }
        }
        return false;
    });

    screen.Loop(main_renderer);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        return run_local({});
    }

    std::string first_arg = argv[1];

    if (first_arg == "--session")
    {
        std::string metrics_file;
        std::vector<std::string> files;
        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--metrics" && i + 1 < argc)
                metrics_file = argv[++i];
            else
                files.push_back(arg);
        }
        return session_main(files, metrics_file);
    }

    if (first_arg == "serve")
    {
        int port = 2222;
        int max_clients = 8;
        std::string password;
        std::string key_path = "certamen_host_rsa";
        std::vector<std::string> files;

        for (int i = 2; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--port" && i + 1 < argc)
                port = std::stoi(argv[++i]);
            else if (arg == "--password" && i + 1 < argc)
                password = argv[++i];
            else if (arg == "--key" && i + 1 < argc)
                key_path = argv[++i];
            else if (arg == "--max-clients" && i + 1 < argc)
                max_clients = std::stoi(argv[++i]);
            else if (arg == "--help" || arg == "-h")
            {
                print_usage(argv[0]);
                return 0;
            }
            else
                files.push_back(arg);
        }

        if (files.empty())
        {
            std::cerr << "Error: serve mode requires at least one quiz file.\n";
            print_usage(argv[0]);
            return 1;
        }

        return serve_main(port, password, files, key_path, max_clients);
    }

    if (first_arg == "--help" || first_arg == "-h")
    {
        print_usage(argv[0]);
        return 0;
    }

    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i)
        files.push_back(argv[i]);
    return run_local(files);
}
