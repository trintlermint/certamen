#ifndef CERTAMEN_MODEL_HPP
#define CERTAMEN_MODEL_HPP

#include <string>
#include <vector>
#include <optional>

struct Question
{
    std::string question;
    std::vector<std::string> choices;
    int answer; // 0-based index into 'choices'.
    std::optional<std::string> code;
    std::optional<std::string> explain;
    std::optional<std::string> language; // code highlight language (e.g. "java", "haskell")
    int source_file = -1; // -1 = unassigned

    bool operator==(const Question& other) const
    {
        return question == other.question
            && choices  == other.choices
            && answer   == other.answer
            && code     == other.code
            && explain  == other.explain
            && language == other.language;
    }
};

struct QuizFile
{
    std::string name;
    std::string author;
    std::vector<Question> questions;
};

std::optional<std::string> validate_question(const Question& q);
QuizFile load_quiz(const std::string& filename);
void save_quiz(const QuizFile& quiz, const std::string& filename);

#endif
