#include "value.h"

#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace xexprengine
{
class DependencyCycleException : public std::runtime_error
{
  public:
    explicit DependencyCycleException(const std::vector<std::string> &cyclePath)
        : std::runtime_error(buildErrorMessage(cyclePath)), cyclePath(cyclePath)
    {
    }

    const std::vector<std::string> &getCyclePath() const noexcept { return cyclePath; }

  private:
    static std::string buildErrorMessage(const std::vector<std::string> &cyclePath)
    {
        std::string msg = "Dependency cycle detected: ";
        for (size_t i = 0; i < cyclePath.size(); ++i)
        {
            if (i != 0)
                msg += " -> ";
            msg += cyclePath[i];
        }
        return msg;
    }

    std::vector<std::string> cyclePath;
};

// support for expression dependency tracking
// support set and reset node dependencies (e.g. SetNodeDependencies(a, {b,c}))
// support keep active dependencies (e.g. if b is remove, a's active
// dependencies will be {c}, but when b is re-added, a's active dependencies
// will include b again)
class VariableDependencyGraph
{
  public:
    void MakeNodeDirty(const std::string &var_name);

    std::vector<std::string> TopologicalSort();
    std::unordered_set<std::string> GetNodeActiveDependencies(const std::string &var_name) const;
    std::unordered_set<std::string> GetNodeActiveDependents(const std::string &var_name) const;

    typedef std::pair<std::string, std::string> Edge;
    struct Node
    {
        std::unordered_set<std::string> active_dependencies;
        std::unordered_set<std::string> active_dependents;
        bool is_dirty;
    };
    void AddNode(const std::string &name);
    void RemoveNode(const std::string &name);
    void ClearNodeEdge(const std::string &name);
    void AddEdge(const Edge &edge);
    void RemoveEdge(const Edge &edge);
    bool IsNodeExist(const std::string &name) const;
    void UpdateGraph(std::function<void(const std::string &)> update_callback);

  private:
    Value CheckNodeCycle(const std::string &node);
    bool HasCycle();
    bool CheckCycleDFS(
        const std::string &node, std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &recursionStack, std::vector<std::string> &cyclePath
    );
    void MarkDependentsDirty(const std::string &node_name, std::unordered_set<std::string> &processed_nodes);
    std::set<Edge> edge_set_;
    std::unordered_map<std::string, Node> node_map_;
    std::unordered_map<std::string, std::set<Edge>> node_dependency_edge_cache_;
    std::unordered_map<std::string, std::set<Edge>> node_dependent_edge_cache_;
};
} // namespace xexprengine