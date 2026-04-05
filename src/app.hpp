#ifndef CERTAMEN_APP_HPP
#define CERTAMEN_APP_HPP

#include "model.hpp"
#include <array>
#include <filesystem>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

enum class AppScreen
{
    MENU,
    QUIZ,
    QUIZ_RESULT,
    ADD_QUESTION,
    REMOVE_QUESTION,
    CHANGE_ANSWER,
    EDIT_CHOICE,
    LIST_QUESTIONS,
    SET_METADATA,
    SAVE_CONFIRM,
    QUIT_CONFIRM,
    LOAD_QUIZ,
    QUIZ_SETUP,
    PICK_FILE,
    MANUAL,
};

struct LoadedFile
{
    std::string filename;
    std::string canonical_path;
    std::string name;
    std::string author;
    std::vector<Question> saved_questions;
    std::string saved_name;
    std::string saved_author;
};

struct AppState
{
    std::vector<Question> questions;
    bool randomise = false;
    std::string status_message;
    std::string quiz_name;
    std::string quiz_author;

    std::vector<LoadedFile> loaded_files;

    // load quiz screen
    std::string load_path_text;
    int load_screen_selected = 0;
    int load_screen_mode = 0; // 0 = file list, 1 = input

    // screen routing
    AppScreen current_screen = AppScreen::MENU;
    int menu_selected = 0;
    std::vector<Question> quiz_session;
    int quiz_index = 0;
    int quiz_score = 0;
    int quiz_selected = 0;
    bool quiz_answered = false;
    bool quiz_was_correct = false;
    bool quiz_quit_pending = false;
    std::string add_question_text;
    std::string add_code_text;
    std::string add_explain_text;
    std::string add_language_text;
    std::array<std::string, 10> add_choices{};
    int add_num_choices = 2;
    int add_correct_idx = 0;
    bool add_include_code = false;
    bool add_include_explain = false;

    // change answer phase, 0 state is pick question, 1 state = pick answer
    int change_answer_phase = 0;
    int select_question_idx = 0;
    int select_new_answer = 0;

    // edit choice: 0 = pick question, 1 = pick choice, 2 = edit text
    int edit_choice_phase = 0;
    int edit_choice_question_idx = 0;
    int edit_choice_choice_idx = 0;
    std::string edit_choice_text;

    std::string meta_name_text;
    std::string meta_author_text;

    // file picker
    int target_file = 0;
    int pick_file_cursor = 0;
    AppScreen pick_file_then = AppScreen::MENU;
    std::vector<int> target_indices;

    // quiz setup (file ordering)
    std::vector<bool> quiz_file_included;
    std::vector<int> quiz_file_order;
    int quiz_setup_phase = 0;   // 0 => yorue selecting the files, 1 => youre ordering them
    int quiz_setup_cursor = 0;

    int manual_topic = 0;
    int manual_scroll = 0;

    // rm question
    int remove_question_idx = 0;

    int list_selected = 0;
    bool list_show_answers = false;
    bool list_show_code = true;
    bool list_show_explain = true;

    // diff lines for quit/save
    std::vector<std::string> diff_lines;

    void return_to_menu()
    {
        current_screen = AppScreen::MENU;
        status_message.clear();
    }

    void build_target_indices()
    {
        target_indices.clear();
        for (int i = 0; i < static_cast<int>(questions.size()); ++i)
        {
            int sf = questions[i].source_file;
            if (target_file < 0 || sf == target_file || (sf < 0 && target_file == 0))
                target_indices.push_back(i);
        }
    }

    AppScreen route_to(AppScreen dest)
    {
        if (loaded_files.size() > 1)
        {
            pick_file_cursor = 0;
            pick_file_then = dest;
            return AppScreen::PICK_FILE;
        }
        target_file = loaded_files.empty() ? -1 : 0;
        build_target_indices();
        return dest;
    }

    void save_target_file()
    {
        if (target_file < 0 || target_file >= static_cast<int>(loaded_files.size()))
            return;

        for (auto& q : questions)
            if (q.source_file < 0)
                q.source_file = target_file;

        auto& lf = loaded_files[target_file];
        QuizFile qf;
        qf.name   = (target_file == 0) ? quiz_name   : lf.name;
        qf.author = (target_file == 0) ? quiz_author : lf.author;

        for (const auto& q : questions)
            if (q.source_file == target_file)
                qf.questions.push_back(q);

        save_quiz(qf, lf.filename);
        lf.saved_questions = qf.questions;
        lf.saved_name      = qf.name;
        lf.saved_author    = qf.author;
        status_message = "Saved " + lf.filename + ".";
    }

    bool has_unsaved_changes() const
    {
        for (int i = 0; i < static_cast<int>(loaded_files.size()); ++i)
        {
            const auto& lf = loaded_files[i];

            if (i == 0 && (quiz_name != lf.saved_name || quiz_author != lf.saved_author))
                return true;

            std::vector<Question> current;
            for (const auto& q : questions)
                if (q.source_file == i)
                    current.push_back(q);

            if (current != lf.saved_questions)
                return true;
        }

        for (const auto& q : questions)
            if (q.source_file < 0)
                return true;

        return false;
    }

    bool load_file(const std::string& path)
    {
        std::string canon;
        try { canon = std::filesystem::canonical(path).string(); }
        catch (...) { canon = std::filesystem::absolute(path).string(); }

        for (const auto& lf : loaded_files)
            if (lf.canonical_path == canon)
            {
                status_message = "Already loaded: " + path;
                return false;
            }

        try
        {
            auto quiz = load_quiz(path);
            int file_idx = static_cast<int>(loaded_files.size());
            int num_loaded = static_cast<int>(quiz.questions.size());

            LoadedFile lf;
            lf.filename       = path;
            lf.canonical_path = canon;
            lf.name         = quiz.name;
            lf.author       = quiz.author;
            lf.saved_name   = quiz.name;
            lf.saved_author = quiz.author;

            for (auto& q : quiz.questions)
            {
                q.source_file = file_idx;
                questions.push_back(q);
            }
            lf.saved_questions = std::move(quiz.questions);

            loaded_files.push_back(std::move(lf));

            if (loaded_files.size() == 1)
            {
                quiz_name   = loaded_files[0].name;
                quiz_author = loaded_files[0].author;
            }

            status_message = "Loaded " + std::to_string(num_loaded) + " questions from " + path + ".";
            return true;
        }
        catch (const std::exception& e)
        {
            status_message = std::string("Load error: ") + e.what();
            return false;
        }
    }

    void unload_file(int file_idx)
    {
        if (file_idx < 0 || file_idx >= static_cast<int>(loaded_files.size()))
            return;

        questions.erase(
            std::remove_if(questions.begin(), questions.end(),
                [file_idx](const Question& q) { return q.source_file == file_idx; }),
            questions.end());

        for (auto& q : questions)
            if (q.source_file > file_idx)
                q.source_file--;

        std::string removed_name = loaded_files[file_idx].filename;
        loaded_files.erase(loaded_files.begin() + file_idx);

        if (!loaded_files.empty())
        {
            quiz_name   = loaded_files[0].name;
            quiz_author = loaded_files[0].author;
        }
        else
        {
            quiz_name.clear();
            quiz_author.clear();
        }

        status_message = "Unloaded " + removed_name + ".";
    }

    void save_all_files()
    {
        if (loaded_files.empty())
            return;

        // assign unassigned questions to first file
        for (auto& q : questions)
            if (q.source_file < 0)
                q.source_file = 0;

        for (int i = 0; i < static_cast<int>(loaded_files.size()); ++i)
        {
            auto& lf = loaded_files[i];
            QuizFile qf;
            qf.name   = (i == 0) ? quiz_name   : lf.name;
            qf.author = (i == 0) ? quiz_author : lf.author;

            for (const auto& q : questions)
                if (q.source_file == i)
                    qf.questions.push_back(q);

            save_quiz(qf, lf.filename);

            lf.saved_questions = qf.questions;
            lf.saved_name      = qf.name;
            lf.saved_author    = qf.author;
        }

        status_message = "Saved " + std::to_string(loaded_files.size()) + (loaded_files.size() == 1 ? " file." : " files.");
    }

    void reset_add_form()
    {
        add_question_text.clear(); add_code_text.clear();
        add_explain_text.clear(); add_language_text.clear();
        add_choices.fill(""); add_num_choices = 2;
        add_correct_idx = 0; add_include_code = false;
        add_include_explain = false;
    }

    void start_quiz() { start_quiz_from(questions); }

    void start_quiz_from(const std::vector<Question>& source)
    {
        if (source.empty()) return;
        quiz_index = 0; quiz_score = 0;
        quiz_selected = 0; quiz_answered = false;
        quiz_was_correct = false; quiz_quit_pending = false;

        if (randomise)
        {
            std::random_device rd; std::mt19937 rng(rd());

            quiz_session.clear();
            quiz_session.reserve(source.size());
            for (const auto& q : source)
            {
                std::vector<std::pair<std::string, bool>> entries;
                entries.reserve(q.choices.size());
                for (std::size_t i = 0; i < q.choices.size(); ++i)
                {

                    entries.emplace_back(q.choices[i], static_cast<int>(i) == q.answer);
                }
                std::shuffle(entries.begin(), entries.end(), rng);

                Question sq;
                sq.question = q.question;
                sq.code = q.code;
                sq.explain = q.explain;
                sq.language = q.language;
                sq.choices.reserve(entries.size());
                sq.answer = 0;
                for (std::size_t i = 0; i < entries.size(); ++i)
                {
                    sq.choices.push_back(std::move(entries[i].first));
                    if (entries[i].second) sq.answer = static_cast<int>(i);
                }
                quiz_session.push_back(std::move(sq));
            }
            std::shuffle(quiz_session.begin(), quiz_session.end(), rng);
        }
        else
        {
            quiz_session = source;
        }
        current_screen = AppScreen::QUIZ;
    }

    void compute_diff(int file_idx = -1)
    {
        diff_lines.clear();

        if (!has_unsaved_changes())
        {
            diff_lines.emplace_back("[0] No unsaved changes.");
            return;
        }

        for (int i = 0; i < static_cast<int>(loaded_files.size()); ++i)
        {
            if (file_idx >= 0 && i != file_idx) continue;

            const auto& lf = loaded_files[i];

            if (loaded_files.size() > 1 && file_idx < 0)
                diff_lines.push_back("  " + lf.filename + ":");

            if (i == 0)
            {
                if (quiz_name != lf.saved_name)
                    diff_lines.push_back("[~] Name: \"" + lf.saved_name + "\" -> \"" + quiz_name + "\"");
                if (quiz_author != lf.saved_author)
                    diff_lines.push_back("[~] Author: \"" + lf.saved_author + "\" -> \"" + quiz_author + "\"");
            }

            std::vector<Question> current;
            for (const auto& q : questions)
                if (q.source_file == i)
                    current.push_back(q);

            for (const auto& orig : lf.saved_questions)
            {
                bool found = false;
                for (const auto& cur : current)
                    if (cur.question == orig.question) { found = true; break; }
                if (!found)
                    diff_lines.push_back("[-] Removed: " + orig.question);
            }

            for (const auto& cur : current)
            {
                bool found = false;
                for (const auto& orig : lf.saved_questions)
                    if (orig.question == cur.question) { found = true; break; }
                if (!found)
                    diff_lines.push_back("[+] Added: " + cur.question);
            }

            for (const auto& cur : current)
            {
                for (const auto& orig : lf.saved_questions)
                {
                    if (cur.question != orig.question) continue;
                    if (cur.answer != orig.answer)
                        diff_lines.push_back("[~] Answer changed: " + cur.question);
                    if (cur.choices != orig.choices)
                        diff_lines.push_back("[~] Choices changed: " + cur.question);
                    if (cur.code != orig.code)
                        diff_lines.push_back("[~] Code changed: " + cur.question);
                    if (cur.explain != orig.explain)
                        diff_lines.push_back("[~] Explanation changed: " + cur.question);
                    break;
                }
            }
        }

        if (file_idx < 0 || file_idx == 0)
            for (const auto& q : questions)
                if (q.source_file < 0)
                    diff_lines.push_back("[+] New: " + q.question);
    }
};

#endif
