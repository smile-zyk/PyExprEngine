#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>

namespace xexprengine {

// support for expression dependency tracking
// support set and reset node dependencies (e.g. SetNodeDependencies(a, {b,c}))
// support keep active dependencies (e.g. if b is remove, a's active
// dependencies will be {c}, but when b is re-added, a's active dependencies
// will include b again)
class ExprDependencyGraph {
public:
  void SetNodeDirty(const std::string &var_name, bool is_dirty, bool is_recursive = true);

  std::vector<std::string> TopologicalSort(bool is_dirty = false);
  void Traverse(const std::function<void(const std::string &) &> callback, bool is_dirty = false);
  std::unordered_set<std::string> GetNodeDependencies(const std::string &var_name) const;
  std::unordered_set<std::string> GetNodeDependents(const std::string &var_name) const;
protected:
  typedef std::pair<std::string, std::string> Edge;
  struct Node {
    std::unordered_set<std::string> active_dependencies;
    std::unordered_set<std::string> active_dependents;
    bool is_dirty;
  };
  void AddNode(const std::string &name);
  void RemoveNode(const std::string &name);
  void ClearNodeEdge(const std::string &name);
  void AddEdge(const Edge &edge);
  void RemoveEdge(const Edge &edge);
private:
  bool CheckCycle(const std::string &node);
  bool CheckCycleUtil(const std::string &node, std::unordered_set<std::string> &visited,
                      std::unordered_set<std::string> &rec_stack);
  std::set<Edge> edge_set_;
  std::unordered_map<std::string, Node> node_map_;
  std::unordered_map<std::string, std::set<Edge>> node_dependency_edge_cache_;
  std::unordered_map<std::string, std::set<Edge>> node_dependent_edge_cache_;
};
} // namespace xexprengine