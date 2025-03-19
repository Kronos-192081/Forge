#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include "parser.hpp"
#include "argparse.hpp"
#include "graph.hpp"
#include "tabulate.hpp"
#include "coderunner.hpp"
using Row_t = tabulate::Table::Row_t;

std::unordered_map<std::string, std::string> master_var_tab;
std::unordered_map<std::string, target> master_targ_tab;
std::unordered_set<std::string> processed_files;
std::unordered_set<std::string> concerned_targets;
std::unordered_set<std::string> all_import_vars;
std::unordered_map<std::string, std::string> all_import_prefixes;

void findDependencies(const std::string& target_name, const std::unordered_map<std::string, target>& targ_tab, std::set<std::string>& relevant_targets, const std::string& prefix) {
    if (relevant_targets.count(target_name)) {
        return;
    }

    relevant_targets.insert(target_name);
    const auto& target = targ_tab.at(target_name);
    for (const auto& dep : target.dependencies) {
        std::string prefixed_dep = prefix + dep;
        if (targ_tab.find(dep) != targ_tab.end()) {
            findDependencies(dep, targ_tab, relevant_targets, prefix);
        }
    }
}

void parse_and_collect_dependencies(const std::string& filename, const std::string& prefix = "") {
    if (processed_files.find(filename) != processed_files.end()) {
        return;
    }
    processed_files.insert(filename);

    Parser parser(filename);
    parser.parse();

    parser.printResults();

    auto [var_tab, targ_tab, import_tab] = parser.get_parsed_results();

    std::set<std::string> relevant_targets;

    for (const auto& target_name : concerned_targets) {
        std::string original_target_name = target_name.substr(prefix.length());
        if (targ_tab.find(original_target_name) != targ_tab.end()) {
            findDependencies(original_target_name, targ_tab, relevant_targets, prefix);
        }
    }

    for (const auto& [var_name, value] : var_tab) {
        master_var_tab[prefix + var_name] = value;
    }

    for (const auto& target_name : relevant_targets) {
        std::string new_target_name = prefix + target_name;
        all_import_prefixes[new_target_name] = prefix;
        master_targ_tab[new_target_name] = targ_tab[target_name];
    }

    for (auto& [import_var, _] : import_tab) {
      if (all_import_vars.count(import_var))
      {
        LOG(ERROR, " variable redefination error. variable " << import_var << " redefined. Please don't reuse variable names across files.")
        exit(1);
      }
      all_import_vars.insert(import_var);
    }

    if (relevant_targets.empty()) {
        return;
    }

    concerned_targets.clear();

    for (const auto& target_name : relevant_targets) {
        const auto& target = targ_tab[target_name];
        for (const auto& dep : target.dependencies) {
            for (const auto& [import_var, import_file] : import_tab) {
                if (dep.find(import_var) == 0) {
                    concerned_targets.insert(dep);
                }
            }
        }
        for (const auto& dep : target.dependencies) {
            for (const auto& [import_var, import_file] : import_tab) {
                if (dep.find(import_var) == 0) {
                    parse_and_collect_dependencies(import_file, import_var + ".");
                }
            }
        }
    }
}

struct Node {
    std::string name;
    target targ_data;
    bool operator==(const Node& other) const {
        return name == other.name;
    }
    bool operator!=(const Node& other) const {
        return !(*this == other);
    }
    bool operator<(const Node& other) const {
        return name < other.name;
    }
    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        os << node.name;
        return os;
    }
};

namespace std {
    template <>
    struct hash<Node> {
        std::size_t operator()(const Node& node) const {
            return std::hash<std::string>()(node.name);
        }
    };
}

void print_banner() {
  tabulate::Table readme;
  readme.format().border_color(tabulate::Color::yellow);

  readme.add_row(Row_t{"Forge - an easy and user-friendly build-system"});
  readme[0].format().font_style({tabulate::FontStyle::underline, tabulate::FontStyle::bold, tabulate::FontStyle::italic}).font_align(tabulate::FontAlign::center).font_color(tabulate::Color::yellow);

  readme.add_row(Row_t{"https://github.com/Kronos-192081/forge"});
  readme[1]
      .format()
      .font_align(tabulate::FontAlign::center)
      .font_style({tabulate::FontStyle::underline, tabulate::FontStyle::italic})
      .font_color(tabulate::Color::white)
      .hide_border_top();

  readme.add_row(Row_t{"Forge is a build system built in Modern C++, inspired by Make and designed to be more intuitive and user-friendly"});
  readme[2].format().font_style({tabulate::FontStyle::italic}).font_color(tabulate::Color::magenta);

  tabulate::Table highlights;
  highlights.add_row(Row_t{"Requires C++20", "MIT License"});
  readme.add_row(Row_t{highlights});
  readme[3].format().font_align(tabulate::FontAlign::center).hide_border_top();

    readme.add_row(Row_t{"To begin with, run forge -h"});
    readme[4]
        .format()
        .font_align(tabulate::FontAlign::center)
        .font_color(tabulate::Color::white)
        .hide_border_top();

  std::cout << readme << "\n\n";
}

int main(int argc, char ** argv) {
    argparse::ArgumentParser program("forge");

    program.add_argument("--log-level")
      .default_value(std::string{"DEFAULT"})
      .help("Specify the Log-Level for Logging. Available Options: DEFAULT, INFO, DEBUG and ERROR");

    program.add_argument("file")
      .default_value(std::string{"forgefile"})
      .help("Specify the file to be considered");

    program.add_argument("target")
      .default_value(std::string{""})
      .help("Specify the target. If not given, first target will be considered");

    program.add_argument("-a", "--about")
      .action([](const auto&){ print_banner(); exit(0);})
      .help("Show about information")
      .default_value(false)
      .implicit_value(true);

    try {
      program.parse_args(argc, argv);
    } catch (const std::exception &err) {
      std::cerr << err.what() << std::endl;
      std::cerr << program;
      return 1;
   }

    std::string loglevel = program.get<std::string>("--log-level"); 
    std::string filename = program.get<std::string>("file");
    std::string target = program.get<std::string>("target");

    if(target.length() == 0){
      Parser parser(filename);
      parser.parse();
      target = parser.get_first_target();
    }

    concerned_targets.insert(target);

    set_log_level(loglevel);

    LOG(INFO, "Parsing ...\n");

    parse_and_collect_dependencies(filename);

    for (auto& [target, data] : master_targ_tab) {
        for (auto& cmd : data.commands) {
            replace_vars(cmd, master_var_tab);
        }
    }

    // std::cout << "\nMaster Variable Table:\n";
    // for (const auto& [var, value] : master_var_tab) {
    //     std::cout << var << " = " << value << "\n";
    // }

    // std::cout << "\nMaster Target Table:\n";
    // for (const auto& [target, data] : master_targ_tab) {
    //     std::cout << target << " : " << data << "\n";
    // }

    Graph<Node> targ_graph;
    // Add Nodes

    for (const auto& [target, data] : master_targ_tab) {
        Node node{target, data};
        targ_graph.addNode(node);
    }

    // Add Edges
    for (const auto& [target, data] : master_targ_tab) {
        for (const auto& dep : data.dependencies) {
            Node node1{target, data};
            if ((master_targ_tab.find(all_import_prefixes[target] + dep) == master_targ_tab.end()) && (master_targ_tab.find(dep) == master_targ_tab.end())) continue;

            std::string prefix;
            if (master_targ_tab.find(all_import_prefixes[target] + dep) == master_targ_tab.end()) {
                prefix = "";
            } else {
                prefix = all_import_prefixes[target];
            }
            Node node2{prefix + dep, master_targ_tab[prefix + dep]};
            targ_graph.addEdge(node2, node1);
        }
    }

    auto is_cycle = targ_graph.hasCycle();
    if (is_cycle.has_value()) {
        std::string cycle_message = "Cycle detected: ";
        for (const auto& node : is_cycle.value()) {
            cycle_message += node.name + " -> ";
        }
        cycle_message += is_cycle.value().front().name;
        LOG(ERROR, cycle_message);
        std::cout << "\n";
        return 1;
    }

    auto topoSort = targ_graph.topologicalSort();

    std::vector<std::string> commands;
    for (const auto& node : topoSort) {
        for (const auto& cmd : node.targ_data.commands) {
            commands.push_back(cmd);
        }
    }

    std::string output = run_command(commands);

    std::ofstream out("output2.html");
    out << output;
    out.close();
    
    targ_graph.visualize("graph.dot");

    return 0;
}