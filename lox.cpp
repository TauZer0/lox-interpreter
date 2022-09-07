#include "lox.hpp"

#include <sysexits.h>

#include <fstream>
#include <iostream>

#include "scanner/scanner.hpp"

namespace lox {

void Lox::RunFile(const std::string& filename) {
    std::ifstream file_stream(filename);
    std::string source{std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>()};
    Run(std::move(source));
    if (had_error_) {
        std::exit(EX_DATAERR);
    }
}

void Lox::RunPrompt() {
    while (!std::cin.eof()) {
        std::cout << "> ";
        std::string line;
        std::getline(std::cin, line);
        Run(std::move(line));
        had_error_ = false;
    }
}

void Lox::Error(int line, const std::string& message) {
    Report(line, "", message);
}

void Lox::Run(std::string&& source) {
    Scanner scanner(std::move(source), *this);
    const std::vector<tokens::Token>& tokens = scanner.ScanTokens();
    for (auto token : tokens) {
        std::cout << token.ToString() << "\n";
    }
}

void Lox::Report(int line, const std::string& where, const std::string& message) {
    std::cerr << "[line " << line << "] Error " << where << ": " << message << "\n";
    had_error_ = true;
}

}  // namespace lox