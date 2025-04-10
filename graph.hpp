#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <queue>
#include <fstream>
#include <cstdlib>

template <typename T>
class Graph {
public:
    void addNode(const T& node);
    void removeNode(const T& node);
    void addEdge(const T& from, const T& to);
    void removeEdge(const T& from, const T& to);
    std::optional<std::vector<T>> hasCycle();
    std::vector<T> topologicalSort();
    int inDegree(const T& node);
    int outDegree(const T& node);
    void visualize(const std::string& filename);

private:
    std::unordered_map<T, std::unordered_set<T>> adjList;
    bool dfsCycleDetection(const T& node, std::unordered_set<T>& visited, std::unordered_set<T>& recStack, std::vector<T>& path, std::vector<T>& cycle);
    void dfsTopologicalSort(const T& node, std::unordered_set<T>& visited, std::stack<T>& Stack);
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
void Graph<T>::visualize(const std::string& filename) {
    generateDotFile(filename);
    std::string command = "dot -Tpng " + filename + " -o graph.png";
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