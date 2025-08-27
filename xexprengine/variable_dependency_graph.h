#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace xexprengine 
{

// support for expression dependency tracking
// support set and reset node dependencies (e.g. SetNodeDependencies(a, {b,c}))
// support keep active dependencies (e.g. if b is remove, a's active
// dependencies will be {c}, but when b is re-added, a's active dependencies
// will include b again)
class ExprDependencyGraph {
public:
  void MakeNodeDirty(const std::string &var_name);
  void ClearNodeDirty(const std::string &var_name);

  std::vector<std::string> TopologicalSort(bool is_dirty = false);
  void Traverse(const std::function<void(const std::string &) &> callback,
                bool is_dirty = false);

  typedef std::pair<std::string, std::string> Edge;
  struct Node {
    std::unordered_set<std::string> active_dependencies;
    std::unordered_set<std::string> active_dependents;
    bool is_dirty;
  };

protected:
  void AddNode(const std::string &name);
  void RemoveNode(const std::string &name);
  void ClearNodeEdge(const std::string &name);
  void AddEdge(const Edge &edge);
  void RemoveEdge(const Edge &edge);

private:
  std::unordered_set<Edge> edge_list_;
  std::unordered_map<std::string, Node> node_map_;
  std::unordered_map<std::string, std::vector<Edge>>
      node_dependency_edge_cache_;
  std::unordered_map<std::string, std::vector<Edge>> node_dependent_edge_cache_;
};
} // namespace xexprengine