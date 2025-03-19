#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <set>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

enum LogLevel{
    INFO,
    DEBUG,
    DEFAULT,
    ERROR
};

extern LogLevel FILTER_LEVEL;

void set_log_level(std::string loglevel);

void replace_vars(std::string &line, const std::unordered_map<std::string, std::string> &variables);

#define LOG_COLOR(LOG_LEVEL) (LOG_LEVEL == INFO ? COLOR_BLUE : (LOG_LEVEL == DEBUG ? COLOR_YELLOW : COLOR_RED))

#define LOG(LOG_LEVEL, LOG_STRING)                                          \
    if (LOG_LEVEL >= FILTER_LEVEL) {                                        \
        std::cout << LOG_COLOR(LOG_LEVEL) << "[" << #LOG_LEVEL << "] "      \
                  << LOG_STRING << COLOR_RESET << std::endl;                \
    }

struct target {
    std::set<std::string> dependencies;
    std::vector<std::string> commands;

    friend std::ostream& operator<<(std::ostream& os, const target& targ) {
        os << "Dependencies[";
        for (auto dep : targ.dependencies) {
            os << dep;
            if (dep != *targ.dependencies.rbegin()) {
                os << ", ";
            }
        }
        os << "] Commands[";
        for (size_t i = 0; i < targ.commands.size(); ++i) {
            os << targ.commands[i];
            if (i != targ.commands.size() - 1) {
                os << ", ";
            }
        }
        os << "]";
        
        return os;
    }
};

using var_table = std::unordered_map<std::string, std::string>;
using targ_table = std::unordered_map<std::string, target>;
using import_table = std::unordered_map<std::string, std::string>;

class Parser {
    public:
        Parser(const std::string& file): filename(file) {}

        void printResults() const;

        std::tuple<var_table, targ_table, import_table> get_parsed_results() {
            return std::make_tuple(std::move(variables), std::move(targets), std::move(imports));
        }

        void parse() { 
            std::ifstream file(filename);
            if (!file) {
                std::cerr << "Error opening forgefile. No such file found." << std::endl;
                exit(1);
            }
            parsefile(file);
        }

        void set_file_name(std::string& name) {
            filename = name;
        }

        std::string get_first_target() { return first_target; }

    private:
        std::string filename;
        var_table variables;
        targ_table targets;
        import_table imports;

        std::string first_target;
        std::string currentTarget;
        int line_no = 0;

        void trim(std::string &s);
        void parsefile(std::ifstream &file);
};

#endif