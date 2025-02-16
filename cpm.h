#ifndef CPM_H
#define CPM_H

#include <boost/graph/adjacency_list.hpp>

#include <map>
#include <string>
#include <set>
#include <queue>
#include <iostream>

namespace CPM {

// TODO: wrapper class for boost graph
// TODO: cleanup header file, split/simplify/move to cpp

// workflow

// forward decs
struct WorkflowProperty;
struct ServiceProperty;
struct ServiceEdgeProperty;

// TODO: workflow is modeled with data flow, should probably be interest flow
// see 'directed' branch; performance difference is negligable
using Workflow = boost::adjacency_list<
    boost::vecS, boost::vecS, boost::bidirectionalS, 
    ServiceProperty, ServiceEdgeProperty, WorkflowProperty>;

using Service = Workflow::vertex_descriptor;
using ServiceEdge = Workflow::edge_descriptor;

struct WorkflowProperty {
    // maps names to service descriptors
    std::map<std::string, Service> map{};
};

struct ServiceProperty {
    std::string name{};
};

// empty for now
struct ServiceEdgeProperty {};

// add_vertex and add_edge with string args
Service add_vertex(const std::string& name, Workflow& g);
std::pair<ServiceEdge, bool> add_edge(const std::string& name1, const std::string& name2, Workflow& g);

// topology, each router holds hosting info

struct TopologyProperty;
struct RouterProperty;
struct RouterEdgeProperty;

using Topology = boost::adjacency_list<
    boost::vecS, boost::vecS, boost::undirectedS,
    RouterProperty, RouterEdgeProperty, TopologyProperty>;

using Router = Topology::vertex_descriptor;
using RouterEdge = Topology::edge_descriptor;

struct TopologyProperty {
    std::map<std::string, Router> map{};
};

// TODO: possibly use descriptors over strings for hosting list
struct RouterProperty {
    std::string name{};
    std::set<std::string> hosting{};
};

struct RouterEdgeProperty {
    unsigned cost{ 1 };
};

// add_vertex and add_edge with string args
Router add_vertex(const std::string& name, Topology& g);
std::pair<RouterEdge, bool> add_edge(const std::string& name1, const std::string& name2, Topology& g);

// branches

struct Branch {
    Router rtr{};
    ServiceEdge intr{};
    unsigned time{ 0 };
};

inline bool operator<(const Branch& l1, const Branch& l2) {
    return l1.time < l2.time;
}

inline bool operator>(const Branch& l1, const Branch& l2) {
    return l1.time > l2.time;
}

class BranchQueue : public std::priority_queue<Branch, std::vector<Branch>, std::greater<Branch>> {
public:
    // if already present with lower priority (higher value), replaces it
    // otherwise push normally
    // returns true if pushed, false if not
    bool push_if_min(Branch next);
};


// helper class to unwrap iterator pairs, for use in for loops
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

unsigned 
criticalPathMetric(Router user, ServiceEdge interest, const Topology& topo, const Workflow& work, bool scopt);
Branch nearestHost(Branch branch, const Topology& topo, const Workflow& work);
std::vector<Branch> nearestHostPath(Branch branch, const Topology& topo, const Workflow& work);
Topology::vertex_iterator findvertex(std::string_view name, const Topology& g);
Workflow::vertex_iterator findvertex(std::string_view name, const Workflow& g);
void print_graph(const Workflow& g);
void print_graph(const Topology& g);

} // namespace CPM

#endif // CPM_H
