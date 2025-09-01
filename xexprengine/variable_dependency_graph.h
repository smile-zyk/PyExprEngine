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
    explicit DependencyCycleException(const std::vector<std::string> &cycle_path)
        : std::runtime_error(buildErrorMessage(cycle_path)), cycle_path_(cycle_path)
    {
    }

    const std::vector<std::string> &getCyclePath() const noexcept { return cycle_path_; }

  private:
    static std::string buildErrorMessage(const std::vector<std::string> &cycle_path)
    {
        std::string msg = "Dependency cycle detected: ";
        for (size_t i = 0; i < cycle_path.size(); ++i)
        {
            if (i != 0)
                msg += " -> ";
            msg += cycle_path[i];
        }
        return msg;
    }

    std::vector<std::string> cycle_path_;
};

class VariableDependencyGraph
{
  public:
    void MakeNodeDirty(const std::string &var_name);
    void ClearNodeDirty(const std::string &var_name);

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
    bool HasCycle();
    Value CheckNodeCycle(const std::string &node);
    bool CheckCycleDFS(
        const std::string &node, std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &recursionStack, std::vector<std::string> &cycle_path
    );
    void MakeNodeDependentsDirty(const std::string &node_name, std::unordered_set<std::string> &processed_nodes);
    std::set<Edge> edge_set_;
    std::unordered_map<std::string, Node> node_map_;
    std::unordered_map<std::string, std::set<Edge>> node_dependency_edge_cache_;
    std::unordered_map<std::string, std::set<Edge>> node_dependent_edge_cache_;
};
} // namespace xexprengine