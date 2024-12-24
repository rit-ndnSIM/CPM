#ifndef CPM_H
#define CPM_H

#include <boost/graph/adjacency_list.hpp>

#include <map>
#include <string>
#include <set>
#include <queue>

namespace CPM {
    // workflow

    struct ServiceProperty {
        std::string name;
    };

    struct ServiceEdgeProperty {
        bool requested;
    };

    using Workflow = boost::adjacency_list<
        boost::vecS, boost::vecS, boost::bidirectionalS, 
        ServiceProperty, ServiceEdgeProperty>;

    using Service = Workflow::vertex_descriptor;
    using ServiceEdge = Workflow::edge_descriptor;

    // topology, each router holds hosting info
    
    struct RouterProperty {
        std::string name;
        std::set<std::string> hosting;
    };

    struct RouterEdgeProperty {
        unsigned cost;
    };

    using Topology = boost::adjacency_list<
        boost::vecS, boost::vecS, boost::undirectedS,
        RouterProperty, RouterEdgeProperty>;

    using Router = Topology::vertex_descriptor;
    using RouterEdge = Topology::edge_descriptor;

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

    int criticalPathMetric(Router* user, Service* consumer);
    Branch nearestHost(Branch branch, const Topology& topo, const Workflow& work);
}

#endif // CPM_H
