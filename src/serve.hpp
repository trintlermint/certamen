#ifndef QUIZZER_SERVE_HPP
#define QUIZZER_SERVE_HPP

#include <string>
#include <vector>

int serve_main(int port,
               const std::string& password,
               const std::vector<std::string>& quiz_files,
               const std::string& host_key_path,
               int max_clients = 8);

#endif
