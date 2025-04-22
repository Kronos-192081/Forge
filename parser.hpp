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

/**
 * @brief Enum representing different log levels.
 */
enum LogLevel{
    INFO,  ///< Informational messages.
    DEBUG, ///< Debugging messages.
    DEFAULT, ///< Default log level.
    ERROR  ///< Error messages.
};

/**
 * @brief Global variable to filter log messages based on their level.
 */
extern LogLevel FILTER_LEVEL;

/**
 * @brief Sets the log level for filtering log messages.
 * 
 * @param loglevel The log level as a string (e.g., "INFO", "DEBUG").
 */
void set_log_level(std::string loglevel);

/**
 * @brief Replaces variables in a string with their corresponding values.
 * 
 * @param line The string in which variables will be replaced.
 * @param variables A map of variable names to their values.
 */
void replace_vars(std::string &line, const std::unordered_map<std::string, std::string> &variables);

/**
 * @brief Macro to determine the color for log messages based on the log level.
 *
 * This macro evaluates the log level and returns the corresponding color code:
 * - `INFO` logs are displayed in blue.
 * - `DEBUG` logs are displayed in yellow.
 * - Other log levels (e.g., `ERROR`) are displayed in red.
 *
 * @param LOG_LEVEL The log level (e.g., INFO, DEBUG, ERROR).
 * @return The color code as a string.
 */
#define LOG_COLOR(LOG_LEVEL) (LOG_LEVEL == INFO ? COLOR_BLUE : (LOG_LEVEL == DEBUG ? COLOR_YELLOW : COLOR_RED))

/**
 * @brief Macro to log messages to the console with color and log level filtering.
 *
 * This macro checks if the provided log level is greater than or equal to the global
 * `FILTER_LEVEL`. If so, it prints the log message to the console with the appropriate
 * color and log level tag.
 *
 * @param LOG_LEVEL The log level of the message (e.g., INFO, DEBUG, ERROR).
 * @param LOG_STRING The log message to be printed.
 *
 * @details
 * - The log message is prefixed with the log level (e.g., `[INFO]`, `[DEBUG]`).
 * - The message is displayed in a color corresponding to the log level.
 * - The color is reset after the message is printed.
 */
#define LOG(LOG_LEVEL, LOG_STRING)                                          \
    if (LOG_LEVEL >= FILTER_LEVEL) {                                        \
        std::cout << LOG_COLOR(LOG_LEVEL) << "[" << #LOG_LEVEL << "] "      \
                  << LOG_STRING << COLOR_RESET << std::endl;                \
    }

/**
 * @brief Struct representing a build target with its dependencies and commands.
 */
struct target {
    std::set<std::string> dependencies;
    std::vector<std::string> commands;

    /**
     * @brief Overloads the stream insertion operator to print the target's details.
     * 
     * @param os The output stream.
     * @param targ The target to be printed.
     * @return The output stream with the target's details.
     */
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

using var_table = std::unordered_map<std::string, std::string>; ///< Alias for a variable table.
using targ_table = std::unordered_map<std::string, target>; ///< Alias for a target table.
using import_table = std::unordered_map<std::string, std::string>; ///< Alias for an import table.

/**
 * @brief Class for parsing a build configuration file.
 */
class Parser {
    public:

        /**
         * @brief Constructs a Parser object with the given file name.
         * 
         * @param file The name of the file to be parsed.
         */
        Parser(const std::string& file): filename(file) {}

        /**
         * @brief Prints the parsed results (variables, targets, and imports).
         */
        void printResults() const;

        /**
         * @brief Retrieves the parsed results as a tuple.
         * 
         * @return A tuple containing the variable table, target table, and import table.
         */
        std::tuple<var_table, targ_table, import_table> get_parsed_results() {
            return std::make_tuple(std::move(variables), std::move(targets), std::move(imports));
        }

        /**
         * @brief Parses the file and populates the variables, targets, and imports.
         * 
         * @note Exits the program if the file cannot be opened.
         */
        void parse() { 
            std::ifstream file(filename);
            if (!file) {
                std::cerr << "Error opening forgefile. No such file found." << std::endl;
                exit(1);
            }
            parsefile(file);
        }

        /**
         * @brief Sets the name of the file to be parsed.
         * 
         * @param name The new file name.
         */
        void set_file_name(std::string& name) {
            filename = name;
        }

        /**
         * @brief Retrieves the name of the first target in the file.
         * 
         * @return The name of the first target.
         */
        std::string get_first_target() { return first_target; }

    private:
        std::string filename; ///< The name of the file to be parsed.
        var_table variables; ///< Table of variables defined in the file.
        targ_table targets; ///< Table of targets defined in the file.
        import_table imports; ///< Table of imported files.

        std::string first_target; ///< The name of the first target in the file.
        std::string currentTarget; ///< The name of the current target being parsed.
        int line_no = 0; ///< The current line number being processed.

        /**
         * @brief Trims leading and trailing whitespace from a string.
         * 
         * @param s The string to be trimmed.
         */
        void trim(std::string &s);

        /**
         * @brief Parses the contents of the file and populates the tables.
         * 
         * @param file The input file stream to be parsed.
         */
        void parsefile(std::ifstream &file);
};

#endif