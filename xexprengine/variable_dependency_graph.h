#include "value.h"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>

namespace xexprengine
{
namespace dependencygraph
{
class DependencyCycleException : public std::runtime_error
{
  public:
    explicit DependencyCycleException(const std::vector<std::string> &cycle_path)
        : std::runtime_error(buildErrorMessage(cycle_path)), cycle_path_(cycle_path)
    {
    }

    const std::vector<std::string> &getCyclePath() const noexcept
    {
        return cycle_path_;
    }

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

class VariableDependencyGraph;

class VariableDependencyGraph
{
  public:
    class Edge
    {
      public:
        Edge(const std::string &from, const std::string &to) : from_(from), to_(to) {}
        const std::string &from() const
        {
            return from_;
        }
        const std::string &to() const
        {
            return to_;
        }

      protected:
        std::string from_;
        std::string to_;
        friend class VariableDependencyGraph;
    };

    class Node
    {
      public:
        Node(bool is_dirty = false) : is_dirty_(is_dirty) {}
        const std::unordered_set<std::string> &dependencies() const
        {
            return dependencies_;
        }
        const std::unordered_set<std::string> &dependents() const
        {
            return dependents_;
        }
        bool is_dirty() const
        {
            return is_dirty_;
        }

      private:
        std::unordered_set<std::string> dependencies_;
        std::unordered_set<std::string> dependents_;
        bool is_dirty_;
        friend class VariableDependencyGraph;
    };

    std::vector<std::string> TopologicalSort() const;

    std::unordered_set<std::string> GetNodeDependencies(const std::string &node) const;
    std::unordered_set<std::string> GetNodeDependents(const std::string &node) const;
    bool IsNodeExist(const std::string &node) const;
    bool IsNodeDirty(const std::string &node) const;

    Node GetNode(const std::string& node) const;
    size_t GetNodeCount() const;
    std::vector<std::string> GetNodeNames() const;
    
    std::vector<Edge> GetEdgesByFrom(const std::string& from) const;
    std::vector<Edge> GetEdgesByTo(const std::string& to) const;
    std::vector<Edge> GetAllEdges() const;
    size_t GetEdgeCount() const;

    void AddNode(const std::string &node, bool is_dirty = false);
    void AddNodes(const std::vector<std::string> &node_list);
    void RemoveNode(const std::string &node);
    void RemoveNodes(const std::vector<std::string> &node_list);
    void SetNodeDirty(const std::string &node, bool dirty);
    void ClearNodeDependencyEdges(const std::string &node);
    void AddEdge(const std::string &from, const std::string &to);
    void AddEdge(const Edge &edge);
    void AddEdges(const std::vector<Edge> &edge_list);
    void RemoveEdge(const Edge &edge);
    void RemoveEdges(const std::vector<Edge> &edge_list);
    void UpdateGraph(std::function<void(const std::string &)> update_callback);

  private:
    struct EdgeContainer
    {
        struct ByFrom
        {};
        struct ByTo
        {};

        struct EdgeHash
        {
            size_t operator()(const Edge &edge) const
            {
                return std::hash<std::string>()(edge.from()) ^ (std::hash<std::string>()(edge.to()) << 1);
            }
        };

        struct EdgeEqual
        {
            bool operator()(const Edge &lhs, const Edge &rhs) const
            {
                return lhs.from() == rhs.from() && lhs.to() == rhs.to();
            }
        };

        typedef boost::multi_index::multi_index_container<
            Edge, boost::multi_index::indexed_by<
                      boost::multi_index::hashed_unique<boost::multi_index::identity<Edge>, EdgeHash, EdgeEqual>,
                      boost::multi_index::hashed_non_unique<
                          boost::multi_index::tag<ByFrom>, boost::multi_index::member<Edge, std::string, &Edge::from_>>,
                      boost::multi_index::hashed_non_unique<
                          boost::multi_index::tag<ByTo>, boost::multi_index::member<Edge, std::string, &Edge::to_>>>>
            Type;
    };

    bool HasCycle() const;
    Value CheckNodeCycle(const std::string &node) const;
    bool CheckCycleDFS(
        const std::string &node, std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &recursionStack, std::vector<std::string> &cycle_path
    ) const;
    void MakeNodeDependentsDirty(const std::string &node_name, std::unordered_set<std::string> &processed_nodes);

    std::unordered_map<std::string, Node> node_map_;

    EdgeContainer::Type edges_;
};
} // namespace dependencygraph
} // namespace xexprengine