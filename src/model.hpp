#ifndef QUIZZER_MODEL_HPP
#define QUIZZER_MODEL_HPP

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

std::optional<std::string> validate_question(const Question& q);
std::vector<Question> load_questions(const std::string& filename);
void save_questions(const std::vector<Question>& questions, const std::string& filename);

#endif
