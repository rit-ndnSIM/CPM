#ifndef CPM_H
#define CPM_H

#include <boost/graph/adjacency_list.hpp>

#include <map>
#include <string>
#include <set>
#include <queue>
#include <iostream>

namespace CPM {

// workflow

struct ServiceProperty {
    std::string name{};
};

struct ServiceEdgeProperty {
    bool requested{false};
};

using Workflow = boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS, 
    ServiceProperty, ServiceEdgeProperty>;

using Service = Workflow::vertex_descriptor;
using ServiceEdge = Workflow::edge_descriptor;

// topology, each router holds hosting info

struct RouterProperty {
    std::string name{};
    std::set<std::string> hosting{};
};

struct RouterEdgeProperty {
    unsigned cost{0};
};

using Topology = boost::adjacency_list<
    boost::vecS, boost::vecS, boost::undirectedS,
    RouterProperty, RouterEdgeProperty>;

using Router = Topology::vertex_descriptor;
using RouterEdge = Topology::edge_descriptor;

// branches

struct Branch {
    Router rtr;
    ServiceEdge intr;
    unsigned time { 0 };
};

inline bool operator<(const Branch& l1, const Branch& l2) {
    return l1.time < l2.time;
}

inline bool operator>(const Branch& l1, const Branch& l2) {
    return l1.time > l2.time;
}

class BranchQueue : public std::priority_queue<Branch, std::vector<Branch>, std::greater<Branch>> {
public:
    void push_min(Branch next);
};

unsigned 
criticalPathMetric(Router user, ServiceEdge interest, const Topology& topo, Workflow& work);
Branch nearestHost(Branch branch, const Topology& topo, const Workflow& work);
Topology::vertex_iterator findvertex(std::string_view name, const Topology& g);
Workflow::vertex_iterator findvertex(std::string_view name, const Workflow& g);

// helper to unwrap iterator pairs
template <typename Iter>
class iterpair {
    std::pair<Iter, Iter> m_pair;

public:
    iterpair(std::pair<Iter, Iter> pair)
        : m_pair { pair }
    {}

    Iter begin() { return m_pair.first; }
    Iter end() { return m_pair.second; }
};

template <typename Graph>
void print_graph(const Graph& g) {
    for (auto e : iterpair(boost::edges(g))) {
        auto src = source(e, g);
        auto tgt = target(e, g);
        std::cout << g[src].name << " (" << src << ") -> " 
            << g[tgt].name << " (" << tgt << ")\n";
    }
}

} // namespace CPM

#endif // CPM_H
