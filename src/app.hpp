#ifndef QUIZZER_APP_HPP
#define QUIZZER_APP_HPP

#include "model.hpp"
#include <array>
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
    LIST_QUESTIONS,
    SAVE_CONFIRM,
    QUIT_CONFIRM,
};

struct AppState
{
    std::string filename;
    std::vector<Question> questions;
    std::vector<Question> saved_questions;
    bool randomise = false;
    std::string status_message;

    // screen routing
    AppScreen current_screen = AppScreen::MENU;
    int menu_selected = 0;

    // quiz session
    std::vector<Question> quiz_session;
    int quiz_index = 0;
    int quiz_score = 0;
    int quiz_selected = 0;
    bool quiz_answered = false;
    bool quiz_was_correct = false;

    // add question form
    std::string add_question_text;
    std::string add_code_text;
    std::string add_explain_text;
    std::string add_language_text;
    std::array<std::string, 10> add_choices{};
    int add_num_choices = 2;
    int add_correct_idx = 0;
    bool add_include_code = false;
    bool add_include_explain = false;

    // change answer: 0 state = pick question, 1 sttae = pick answer
    int change_answer_phase = 0;
    int select_question_idx = 0;
    int select_new_answer = 0;

    // remov question
    int remove_question_idx = 0;

    // list questions
    int list_selected = 0;
    bool list_show_answers = false;
    bool list_show_code = true;
    bool list_show_explain = true;

    // diff lines for quit confirm
    std::vector<std::string> diff_lines;

    void reset_add_form()
    {
        add_question_text.clear(); add_code_text.clear();
        add_explain_text.clear(); add_language_text.clear();
        add_choices.fill(""); add_num_choices = 2;
        add_correct_idx = 0; add_include_code = false;
        add_include_explain = false;
    }

    void start_quiz()
    {
        if (questions.empty()) return;
        quiz_index = 0; quiz_score = 0;
        quiz_selected = 0; quiz_answered = false;
        quiz_was_correct = false;

        if (randomise)
        {
            std::random_device rd; std::mt19937 rng(rd());

            quiz_session.clear();
            quiz_session.reserve(questions.size());
            for (const auto& q : questions)
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
            quiz_session = questions;
        }
        current_screen = AppScreen::QUIZ;
    }

    void compute_diff()
    {
        diff_lines.clear();
        if (saved_questions == questions)
        {
            diff_lines.emplace_back("[0] No unsaved changes.");
            return;
        }

        for (const auto& orig : saved_questions)
        {
            bool found = false;
            for (const auto& cur : questions)
            {
                if (cur.question == orig.question) { found = true; break; }
            }
            if (!found)
                diff_lines.push_back("[-] Removed: " + orig.question);
        }

        for (const auto& cur : questions)
        {
            bool found = false;
            for (const auto& orig : saved_questions)
            {
                if (orig.question == cur.question) { found = true; break; }
            }
            if (!found)
                diff_lines.push_back("[+] Added: " + cur.question);
        }

        for (const auto& cur : questions)
        {
            for (const auto& orig : saved_questions)
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
};

#endif
