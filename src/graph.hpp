#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <fstream>
#include <cstdlib>

/**
 * @brief A generic directed graph implementation with utility functions.
 *
 * This class provides a representation of a directed graph using an adjacency list.
 * It includes methods for adding and removing nodes and edges, detecting cycles,
 * performing topological sorting, and visualizing the graph.
 *
 * @tparam T The type of the nodes in the graph. Must be hashable and comparable.
 */
template <typename T>
class Graph {
public:
    /**
     * @brief Adds a node to the graph.
     * @param node The node to be added.
     */
    void addNode(const T& node);

    /**
     * @brief Removes a node from the graph.
     * @param node The node to be removed.
     */
    void removeNode(const T& node);

    /**
     * @brief Adds a directed edge between two nodes.
     * @param from The source node of the edge.
     * @param to The destination node of the edge.
     */
    void addEdge(const T& from, const T& to);

    /**
     * @brief Removes a directed edge between two nodes.
     * @param from The source node of the edge.
     * @param to The destination node of the edge.
     */
    void removeEdge(const T& from, const T& to);

    /**
     * @brief Detects if the graph contains a cycle.
     * @return An optional vector of nodes representing the cycle if one exists, or an empty optional otherwise.
     */
    std::optional<std::vector<T>> hasCycle();

    /**
     * @brief Performs a topological sort of the graph.
     * @return A vector of nodes in topologically sorted order.
     * @note The graph must be a Directed Acyclic Graph (DAG) for this to work correctly.
     */
    std::vector<T> topologicalSort();

    /**
     * @brief Computes the in-degree of a node.
     * @param node The node whose in-degree is to be calculated.
     * @return The in-degree of the node.
     */
    int inDegree(const T& node);

    /**
     * @brief Computes the out-degree of a node.
     * @param node The node whose out-degree is to be calculated.
     * @return The out-degree of the node.
     */
    int outDegree(const T& node);

    /**
     * @brief Visualizes the graph by generating a DOT file and an image.
     * @param filename The name of the DOT file to be generated.
     * @param imgfilename The name of the image file to be generated.
     * @note Requires Graphviz to generate the image from the DOT file.
     */
    void visualize(const std::string& filename, const std::string& imgfilename);

private:

    /**
     * @brief The adjacency list representation of the graph.
     * Maps each node to a set of its neighboring nodes.
     */
    std::unordered_map<T, std::unordered_set<T>> adjList;

    /**
     * @brief Helper function for cycle detection using Depth-First Search (DFS).
     * @param node The current node being visited.
     * @param visited A set of nodes that have been visited.
     * @param recStack A set of nodes in the current recursion stack.
     * @param path A vector to store the current path of nodes.
     * @param cycle A vector to store the detected cycle if one exists.
     * @return True if a cycle is detected, false otherwise.
     */
    bool dfsCycleDetection(const T& node, std::unordered_set<T>& visited, std::unordered_set<T>& recStack, std::vector<T>& path, std::vector<T>& cycle);
    
    /**
     * @brief Helper function for topological sorting using Depth-First Search (DFS).
     * @param node The current node being visited.
     * @param visited A set of nodes that have been visited.
     * @param Stack A stack to store the topologically sorted nodes.
     */
    void dfsTopologicalSort(const T& node, std::unordered_set<T>& visited, std::stack<T>& Stack);
    
    /**
     * @brief Generates a DOT file representing the graph.
     * @param filename The name of the DOT file to be generated.
     */
    void generateDotFile(const std::string& filename);
};

template <typename T>
void Graph<T>::addNode(const T& node) {
    adjList[node];
}

template <typename T>
void Graph<T>::removeNode(const T& node) {
    adjList.erase(node);
    for (auto& [key, neighbors] : adjList) {
        neighbors.erase(node);
    }
}

template <typename T>
void Graph<T>::addEdge(const T& from, const T& to) {
    adjList[from].insert(to);
}

template <typename T>
void Graph<T>::removeEdge(const T& from, const T& to) {
    adjList[from].erase(to);
}

template <typename T>
std::optional<std::vector<T>> Graph<T>::hasCycle() {
    std::unordered_set<T> visited;
    std::unordered_set<T> recStack;
    std::vector<T> path;
    std::vector<T> cycle;
    for (const auto& [node, _] : adjList) {
        if (dfsCycleDetection(node, visited, recStack, path, cycle)) {
            return cycle;
        }
    }
    return std::nullopt;
}

template <typename T>
bool Graph<T>::dfsCycleDetection(const T& node, std::unordered_set<T>& visited, std::unordered_set<T>& recStack, std::vector<T>& path, std::vector<T>& cycle) {
    if (recStack.find(node) != recStack.end()) {
        auto it = std::find(path.begin(), path.end(), node);
        cycle.assign(it, path.end());
        return true;
    }
    if (visited.find(node) != visited.end()) {
        return false;
    }
    visited.insert(node);
    recStack.insert(node);
    path.push_back(node);
    for (const auto& neighbor : adjList[node]) {
        if (dfsCycleDetection(neighbor, visited, recStack, path, cycle)) {
            return true;
        }
    }
    recStack.erase(node);
    path.pop_back();
    return false;
}

template <typename T>
std::vector<T> Graph<T>::topologicalSort() {
    std::stack<T> Stack;
    std::unordered_set<T> visited;
    for (const auto& [node, _] : adjList) {
        if (visited.find(node) == visited.end()) {
            dfsTopologicalSort(node, visited, Stack);
        }
    }
    std::vector<T> result;
    while (!Stack.empty()) {
        result.push_back(Stack.top());
        Stack.pop();
    }
    return result;
}

template <typename T>
void Graph<T>::dfsTopologicalSort(const T& node, std::unordered_set<T>& visited, std::stack<T>& Stack) {
    visited.insert(node);
    for (const auto& neighbor : adjList[node]) {
        if (visited.find(neighbor) == visited.end()) {
            dfsTopologicalSort(neighbor, visited, Stack);
        }
    }
    Stack.push(node);
}

template <typename T>
int Graph<T>::inDegree(const T& node) {
    int degree = 0;
    for (const auto& [key, neighbors] : adjList) {
        if (neighbors.find(node) != neighbors.end()) {
            degree++;
        }
    }
    return degree;
}

template <typename T>
int Graph<T>::outDegree(const T& node) {
    return adjList[node].size();
}

template <typename T>
void Graph<T>::visualize(const std::string& filename, const std::string& imgfilename) {
    generateDotFile(filename);
    std::string command = "dot -Tpng " + filename + " -o " + imgfilename + ".png";
    system(command.c_str());
    std::remove(filename.c_str());  
}

template <typename T>
void Graph<T>::generateDotFile(const std::string& filename) {
    std::ofstream file(filename);
    file << "digraph G {\n";
    for (const auto& [node, neighbors] : adjList) {
        for (const auto& neighbor : neighbors) {
            file << "    \"" << node << "\" -> \"" << neighbor << "\";\n";
        }
    }
    file << "}\n";
    file.close();
}

#endif // GRAPH_HPP