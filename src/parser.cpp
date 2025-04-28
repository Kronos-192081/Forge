#include "parser.hpp"
#include <regex>

LogLevel FILTER_LEVEL = DEFAULT;

void set_log_level(std::string loglevel) {
    if (loglevel == "INFO") FILTER_LEVEL = INFO;
    else if (loglevel == "DEBUG") FILTER_LEVEL = DEBUG;
    else if (loglevel == "ERROR") FILTER_LEVEL= ERROR;
    else FILTER_LEVEL = DEFAULT;
}

void Parser::trim(std::string &s) {
        if(s[0] == '\t') return;
        size_t start = s.find_first_not_of(" \t");
        size_t end = s.find_last_not_of(" \t");
        s = (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

void replace_vars(std::string &line, const std::unordered_map<std::string, std::string> &variables) {
    std::regex var_pattern(R"(\$\(([\w\.]+)\))");
    std::smatch match;
    std::string::const_iterator search_start(line.cbegin());
    while (std::regex_search(search_start, line.cend(), match, var_pattern)) {
        std::string var_name = match[1].str();
        if (variables.find(var_name) != variables.end()) {
            line.replace(match.position(0), match.length(0), variables.at(var_name));
            search_start = line.cbegin() + match.position(0) + variables.at(var_name).length();
        } else {
            search_start = match.suffix().first;
        }
    }
}

void Parser::parsefile(std::ifstream &file) {
    bool is_first_target = true;
    std::string line;
    bool line_cont = false;
    while (std::getline(file, line)) {
        line_no++;
        trim(line);

        // print the exact characters of the line in ascii
        if (line.empty() || line[0] == '#') continue;
        replace_vars(line, variables);

        if (line[0] == '\t') { 
            if (line.length() == 1) continue;
            if (!currentTarget.empty()) {
                if (!line.empty() && line.back() == '\\') {
                    line.pop_back();
                    std::string last_cmd = "";
                    if (line_cont) {
                        last_cmd = targets[currentTarget].commands.back();
                        targets[currentTarget].commands.pop_back();
                    }
                    last_cmd += line.substr(1);
                    targets[currentTarget].commands.push_back(last_cmd);
                    line_cont = true;
                    continue;
                }

                if (line_cont){
                    std::string last_cmd = targets[currentTarget].commands.back();
                    targets[currentTarget].commands.pop_back();
                    last_cmd += line.substr(1);
                    targets[currentTarget].commands.push_back(last_cmd);
                    line_cont = false;
                } else {
                    targets[currentTarget].commands.push_back(line.substr(1));
                }
            }
        } else if (line.find(":") != std::string::npos && 
                    (line.find("=") == std::string::npos || line.find(":") < line.find("="))) {

            size_t colonPos = line.find(":");
            currentTarget = line.substr(0, colonPos);
            trim(currentTarget);
            if(is_first_target) {
                first_target = currentTarget;
                is_first_target = false;
            }
            std::istringstream depStream(line.substr(colonPos + 1));
            std::string dep;
            while (depStream >> dep) {
                trim(dep);
                targets[currentTarget].dependencies.insert(dep);
            }
        } else if (line.find("=") != std::string::npos) {
            size_t eqPos = line.find("=");
            std::string var = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);
            trim(var);
            trim(value);
            std::string after_assign = line.substr(eqPos + 1, 7);
            trim(after_assign);
            if (after_assign == "import") {
                std::string import_str = line.substr(eqPos + 8);
                trim(import_str);
                import_str = import_str.substr(1, import_str.size()- 2);
                imports[var] = import_str;
            } else {
                variables[var] = value;
            }
        } else {
            LOG(ERROR, " Error parsing forgefile in line " << line_no << ": " << line);
            exit(0);
        }
    }
}

void Parser::printResults() const {
    LOG(INFO, "Variables:\n");
    for (const auto &var : variables) {
        LOG(INFO, var.first << " = " << var.second << "\n");
    }

    LOG(INFO, "\nTargets:\n");
    for (const auto &t : targets) {
        LOG(INFO, t.first << ": ");
        for (const auto &dep : t.second.dependencies) {
            LOG(INFO, dep << " ");
        }
        LOG(INFO, "\nCommands:\n");
        for (const auto &cmd : t.second.commands) {
            LOG(INFO, cmd << "\n");
        }

        LOG(INFO, std::endl);
    }

    LOG(INFO, "\nImports:\n");
    for (const auto &imp : imports) {
        LOG(INFO, imp.first << " = import " << imp.second << "\n");
    }
}