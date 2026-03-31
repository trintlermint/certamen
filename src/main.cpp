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

static void print_help(const char* prog)
{
    std::cout
        << "CERTAMEN; Terminal quiz game engine\n"
        << "\n"
        << "USAGE\n"
        << "  " << prog << "                           Start empty (load files from TUI or create one!)\n"
        << "  " << prog << " <a.yaml b.yaml ...>           Load >1 quiz files\n"
        << "  " << prog << " serve [options] <files>   Host an SSH quiz server\n"
        << "  " << prog << " -h, --help                Show this help menu!\n"
        << "\n"
        << "LOCAL MODE\n"
        << "  Opens the executable regularly. Pass zero or more .yaml quiz files as stdin arguments.\n"
        << "  With no arguments, the program starts empty and load files from the Load Quiz File menu.\n"
        << "  With multiple files, each file's questions are tracked separately, letting you choose.\n"
        << "\n"
        << "TUI KEYBINDINGS\n"
        << "  Main menu:\n"
        << "    1-9           Jump to menu entry\n"
        << "    j/k, arrows   Navigate menu\n"
        << "    Enter         Confirm selection\n"
        << "    r             Toggle randomised question/answer order\n"
        << "    q             Quit (asks for confirmation)\n"
        << "\n"
        << "  Quiz:\n"
        << "    j/k, arrows   Navigate answers\n"
        << "    1-9           Jump to answer\n"
        << "    Enter         Submit answer / next question\n"
        << "    q, Esc        Quit quiz (asks for confirmation)\n"
        << "\n"
        << "  Editing screens (Add, Remove, Change Answer, Edit Choice):\n"
        << "    j/k, arrows   Navigate items\n"
        << "    Enter         Select / confirm\n"
        << "    Esc, b        Go back one step\n"
        << "\n"
        << "  Load Quiz File:\n"
        << "    Tab           Switch between file list and input\n"
        << "    j/k           Navigate loaded files\n"
        << "    Right, Enter  Unload selected file\n"
        << "    Esc           Return to menu\n"
        << "\n"
        << "MENU OPTIONS\n"
        << "  1. Take Quiz        Answer questions, get scored. With multiple files\n"
        << "                      loaded, pick which files to include and their order.\n"
        << "  2. Add Question     Compose a question with choices, optional code\n"
        << "                      snippet (with syntax highlighting), and explanation.\n"
        << "  3. Remove Question  Delete a question from the quiz.\n"
        << "  4. Change Answer    Update the correct answer for a question.\n"
        << "  5. Edit Choice      Edit the text of a choice.\n"
        << "  6. List Questions   Browse questions with toggleable answers, code,\n"
        << "                      and explanations.\n"
        << "  7. Set Author/Name  Set metadata (per-file with multiple files).\n"
        << "  8. Save             Save changes to disk, view diff before saving.\n"
        << "  9. Load Quiz File   Load additional .yaml files or unload existing ones.\n"
        << " 10. Manual           Built-in reference for all features.\n"
        << "\n"
        << "  With multiple files loaded, editing operations (2-8) ask which file\n"
        << "  to target before proceeding.\n"
        << "\n"
        << "QUIZ FORMAT (.yaml)\n"
        << "  name: My Quiz\n"
        << "  author: yourname\n"
        << "  questions:\n"
        << "    - question: What is 2+2?\n"
        << "      choices: [3, 4, 5, 6]\n"
        << "      answer: 1              # 0-based index (displayed as 1-based)\n"
        << "      code: |                # optional code block\n"
        << "        print(2+2)\n"
        << "      language: python       # optional code highlighter\n"
        << "      explain: |             # optional explanation shown after answering\n"
        << "        Basic arithmetic.\n"
        << "\n"
        << "  Required per question: question, choices (>1), answer\n"
        << "  Optional per question: code, language, explain\n"
        << "  Optional top-level: name, author\n"
        << "\n"
        << "SERVE MODE (SSH)\n"
        << "  Host quizzes over SSH. Clients connect with any SSH client (as long as they use a compatible term emulator).\n"
        << "  The SSH username becomes the player's display name.\n"
        << "\n"
        << "  " << prog << " serve [options] <quiz.yaml ...>\n"
        << "\n"
        << "  Options:\n"
        << "    --port <N>          Listen port (default: 2222)\n"
        << "    --password <pw>     Require password (default: open access)\n"
        << "    --key <path>        RSA host key (default: certamen_host_rsa)\n"
        << "    --max-clients <N>   Max concurrent connections (default: 8)\n"
        << "\n"
        << "  Client connects:  ssh -p 2222 playername@server-ip\n"
        << "  Server logs scores per player to stdout.\n"
        << "  Ctrl+C stops the server. See SERVING.md for full documentation.\n"
        << "\n"
        << "EXAMPLES\n"
        << "  " << prog << " quiz.yaml\n"
        << "  " << prog << " algebra.yaml history.yaml\n"
        << "  " << prog << " serve --port 3000 --password secret quiz.yaml\n"
        << "  ssh -p 3000 alice@192.168.1.10\n";
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
                print_help(argv[0]);
                return 0;
            }
            else
                files.push_back(arg);
        }

        if (files.empty())
        {
            std::cerr << "Error: serve mode requires at least one quiz file.\n";
            print_help(argv[0]);
            return 1;
        }

        return serve_main(port, password, files, key_path, max_clients);
    }

    if (first_arg == "--help" || first_arg == "-h")
    {
        print_help(argv[0]);
        return 0;
    }

    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i)
        files.push_back(argv[i]);
    return run_local(files);
}
