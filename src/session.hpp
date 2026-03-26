#ifndef QUIZZER_SESSION_HPP
#define QUIZZER_SESSION_HPP

#include <string>
#include <vector>

int session_main(const std::vector<std::string>& quiz_files,
                 const std::string& metrics_file = "");

#endif
