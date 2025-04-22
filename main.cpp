#include <iostream>
#include <unordered_set>
#include <filesystem>
#include <unordered_map>
#include "parser.hpp"
#include "argparse.hpp"
#include "graph.hpp"
#include "tabulate.hpp"
#include "coderunner.hpp"
#include "cache.hpp"

using Row_t = tabulate::Table::Row_t;

std::unordered_map<std::string, std::string> master_var_tab; ///< table containing variable declarations across all forgefiles
std::unordered_map<std::string, target> master_targ_tab; ///< All targets
std::unordered_set<std::string> processed_files; ///< list of files already processed
std::unordered_set<std::string> concerned_targets; ///< targets which are concerned based on the target asked to be built
std::unordered_set<std::string> all_import_vars; ///< list of all import variables
std::unordered_map<std::string, std::string> all_import_prefixes; ///< list of all import variables with their prefixes
std::unordered_map<std::string, std::string> act_targs; ///< actual names of targets
std::set<std::string> all_targs; ///< list of all targets


/**
 * @brief Recursively finds and collects all dependencies for a given target.
 *
 * This function identifies all the dependencies of a specified target and adds them
 * to the set of relevant targets. It ensures that each target is processed only once
 * by checking if it is already in the set of relevant targets.
 *
 * @param target_name The name of the target whose dependencies are to be found.
 * @param targ_tab A map containing all targets and their associated data, including dependencies.
 * @param relevant_targets A set to store all relevant targets (including dependencies) for the given target.
 * @param prefix A string prefix to prepend to dependency names (if needed).
 */
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

/**
 * @brief Parses a file and collects dependencies, updating global tables and variables.
 *
 * This function processes a given file, parses its contents, and collects all relevant
 * dependencies, variables, and targets. It ensures that files are processed only once
 * and handles variable redefinitions across files. The function also recursively processes
 * imported files and their dependencies.
 *
 * @param filename The name of the file to parse and process.
 * @param prefix A string prefix to prepend to variable and target names (default is an empty string).
 *
 * @details
 * - Updates the `master_var_tab` with variables from the parsed file.
 * - Updates the `master_targ_tab` with targets from the parsed file.
 * - Handles recursive imports and ensures no variable redefinitions occur across files.
 * - Processes dependencies for concerned targets and recursively parses imported files.
 *
 * @note If a variable is redefined across files, the function logs an error and exits.
 */
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
        act_targs[new_target_name] = target_name;
        all_targs.insert(target_name);
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

/**
 * @brief Represents a node in a dependency graph with associated target data.
 *
 * This struct encapsulates a node's name and its associated target data. It provides
 * comparison operators for equality, inequality, and ordering, as well as a friend
 * function for outputting the node's name to an output stream.
 */
struct Node {
    std::string name; ///< name of the node
    target targ_data; ///< target data associated with the node

     /**
     * @brief Checks if two nodes are equal based on their names.
     * @param other The other node to compare with.
     * @return True if the names are equal, false otherwise.
     */
    bool operator==(const Node& other) const {
        return name == other.name;
    }

    /**
     * @brief Checks if two nodes are not equal based on their names.
     * @param other The other node to compare with.
     * @return True if the names are not equal, false otherwise.
     */
    bool operator!=(const Node& other) const {
        return !(*this == other);
    }

     /**
     * @brief Compares two nodes for ordering based on their names.
     * @param other The other node to compare with.
     * @return True if this node's name is lexicographically less than the other's name.
     */
    bool operator<(const Node& other) const {
        return name < other.name;
    }

    /**
     * @brief Outputs the node's name to an output stream.
     * @param os The output stream.
     * @param node The node to output.
     * @return The output stream with the node's name appended.
     */
    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
        os << node.name;
        return os;
    }
};

/**
 * @brief Specialization of the `std::hash` template for the `Node` struct.
 *
 * This specialization allows `Node` objects to be used as keys in unordered containers
 * such as `std::unordered_map` or `std::unordered_set`. It computes the hash value
 * for a `Node` based on its `name` attribute.
 */
namespace std {
    template <>
    struct hash<Node> {
        /**
         * @brief Computes the hash value for a given `Node`.
         * 
         * @param node The `Node` object for which the hash value is to be computed.
         * @return The hash value of the `Node`'s `name` attribute.
         */
        std::size_t operator()(const Node& node) const {
            return std::hash<std::string>()(node.name);
        }
    };
}

/**
 * @brief Displays a banner with information about the Forge build system.
 *
 * This function uses the `tabulate` library to create a styled table that displays
 * a banner with details about Forge, including its purpose, license, and usage instructions.
 * The banner is formatted with various font styles, colors, and alignments to enhance readability.
 *
 * @details
 * - The banner includes:
 *   - A title with the name of the build system.
 *   - A link to the Forge GitHub repository.
 *   - A brief description of Forge.
 *   - Highlights such as the required C++ version and license.
 *   - A quick-start instruction.
 * - The table is styled using the `tabulate` library with custom colors, font styles, and alignments.
 *
 * @note This function outputs the banner directly to the standard output.
 */
void print_banner() {
  tabulate::Table readme;
  readme.format().border_color(tabulate::Color::yellow);

  readme.add_row(Row_t{"Forge - an easy and user-friendly build-system"});
  readme[0].format().font_style({tabulate::FontStyle::underline, tabulate::FontStyle::bold, tabulate::FontStyle::italic}).font_align(tabulate::FontAlign::center).font_color(tabulate::Color::yellow);

  readme.add_row(Row_t{"https://github.com/Kronos-192081/Forge"});
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
    argparse::ArgumentParser program("forge"); ///< argument parser object

    program.add_argument("--log-level") ///< argument for loglevel
      .default_value(std::string{"DEFAULT"})
      .help("Specify the Log-Level for Logging. Available Options: DEFAULT, INFO, DEBUG and ERROR");

    program.add_argument("file", "-f", "--file") ///< file to be considered
      .default_value(std::string{"forgefile"})
      .help("Specify the file to be considered");

    program.add_argument("target") ///< target to be built
      .default_value(std::string{""})
      .help("Specify the target. If not given, first target will be considered");

    program.add_argument("-a", "--about") ///< about forge
      .action([](const auto&){ print_banner(); exit(0);})
      .help("Show about information")
      .default_value(false)
      .implicit_value(true);

    program.add_argument("-j", "--jobs") ///< number of jobs (parallel threads)
        .default_value(1)
        .help("Specify the number of jobs to run in parallel. Default is 1")
        .scan<'i', int>();

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

    int njobs = program.get<int>("--jobs");
    bool is_par = njobs > 1;

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

    Graph<Node> targ_graph; ///< DAG of the forgefile based on the target and dependencies

    for (const auto& [target, data] : master_targ_tab) {
        Node node{target, data};
        targ_graph.addNode(node);
    }

    for (const auto& [target, data] : master_targ_tab) { ///< building the target graph
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

    auto is_cycle = targ_graph.hasCycle(); ///< return if the graph has cycle by printing a cycle as well
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

    targ_graph.visualize("graph.dot", "graph"); ///< visualises the graph as a png file

    auto topoSort = targ_graph.topologicalSort(); ///< toposort of the graph

    std::vector<Node> compilable_nodes; ///< all nodes which are to be compiled
    Cache cache; ///< a cache to store the files which are already compiled
    std::unordered_map<std::string, bool> to_compile;


    /**
     * @brief Determines which nodes in the dependency graph need to be compiled.
     *
     * This loop iterates over the topologically sorted nodes (`topoSort`) and evaluates
     * whether each node requires compilation based on its dependencies and cache status.
     *
     * @details
     * - For each node:
     *   - Retrieves its dependencies.
     *   - Initializes the `to_compile` status for the node as `false`.
     *   - Iterates over the dependencies:
     *     - Checks if the dependency is a target (`act_targs` or `all_targs`).
     *       - If the dependency is a target and marked for compilation, marks the current
     *         node for compilation and adds it to `compilable_nodes`.
     *     - If the dependency is not a target:
     *       - Constructs the full file path and checks the cache.
     *       - If the file is not in the cache, adds it to the cache, marks the current node
     *         for compilation, and adds it to `compilable_nodes`.
     *   - If the node is still not marked for compilation:
     *     - Constructs the full target path and checks if it exists in the filesystem.
     *     - If the target does not exist, marks the node for compilation and adds it to
     *       `compilable_nodes`.
     *
     * @note This logic ensures that only nodes with unmet dependencies or missing targets
     *       are marked for compilation, optimizing the build process.
     */
    for (const auto& node: topoSort) {
        auto deps = node.targ_data.dependencies;
        to_compile[node.name] = false;

        for (auto& dep: deps) {
            bool is_targ = false;
            if ((act_targs.find(dep) != act_targs.end()) || (all_targs.find(dep) != all_targs.end())) {
                is_targ = true;
            }

            if(is_targ) {
                std::string targ_name = dep;
                if (to_compile[targ_name] == true) {
                    to_compile[act_targs[node.name]] = true;
                    compilable_nodes.push_back(node);
                    break;
                }
            } else {
                std::string full_file_path = std::filesystem::current_path().string() + "/" + dep;
                if (!cache.check(full_file_path)) {
                    cache.add(full_file_path);
                    to_compile[act_targs[node.name]] = true;
                    compilable_nodes.push_back(node);
                    break;
                }
            }

        }
        if (to_compile[act_targs[node.name]] == false) {
            std::string full_target_path = std::filesystem::current_path().string() + "/" + act_targs[node.name];
            if (!std::filesystem::exists(full_target_path)) {
                to_compile[act_targs[node.name]] = true;
                compilable_nodes.push_back(node);
            }
        }
    }

    if (is_par) { ///< configure the targets that are to be compiled in parallel
        std::vector<std::vector<std::string>> parallelizable_labels;
        while (!topoSort.empty()) {
            std::vector<std::string> labels;
            std::vector<Node> nodesToRemove;
            for (const auto& node : topoSort) {
                if (targ_graph.inDegree(node) == 0) {
                    if (to_compile[act_targs[node.name]] == true) {
                        labels.push_back(node.name);
                    }
                    nodesToRemove.push_back(node);
                }
            }
            if(!labels.empty()) {
                parallelizable_labels.push_back(labels);
            }
            for (const auto& node : nodesToRemove) {
                targ_graph.removeNode(node);
            }
            topoSort = targ_graph.topologicalSort();
        }

        std::vector<std::vector<std::vector<std::string>>> parallelizable_commands;
        for (const auto& labels : parallelizable_labels) {
            std::vector<std::vector<std::string>> commands_for_labels;
            for (const auto& label : labels) {
                commands_for_labels.push_back(master_targ_tab[label].commands);
            }
            parallelizable_commands.push_back(commands_for_labels);
        }

        std::string output = run_commands_parallel(parallelizable_commands, njobs); ///< running jobs in parallel
        std::ofstream out("forge_output.html");
        out << output;
        out.close();
    } else {
        std::vector<std::string> commands;
        for (const auto& node : compilable_nodes) {
            for (const auto& cmd : node.targ_data.commands) {
                commands.push_back(cmd);
            }
        }
        std::string output = run_commands(commands); ///< running jobs in serial
        std::ofstream out("forge_output.html");
        out << output;
        out.close();
    }

    return 0;
}