#ifndef SCENARIO_H
#define SCENARIO_H

#include "graph.h"

#include <map>
#include <string>
#include <set>
#include <queue>

// this is all really shitty because everything else i tried kept exploding
// i'm tired. so tired.
// ideally i'd inherit from 
namespace CPM {
    // workflow

    struct ServiceData {
        std::string name {};
    };

    struct ServiceLinkData {
        bool requested {false};
    };

    using Workflow = Graph<ServiceData, ServiceLinkData>;
    using Service = Workflow::Node;
    using ServiceLink = Workflow::Link;

    // topology, each router holds hosting info
    
    struct RouterData {
        std::string name {};
        std::set<std::string> hosting {};

        void set_hosting(std::string service) { hosting.insert(service); }
        bool hosts(std::string service) { return hosting.count(service); }
    };

    struct RouterLinkData {
        unsigned cost {0};
    };

    using Topology = Graph<RouterData, RouterLinkData>;
    using Router = Topology::Node;
    using RouterLink = Topology::Link;

    // branches

    struct Branch {
        Router* rtr {};
        Service* srv {};
        unsigned time { 0 };
    };

    bool operator<(const Branch& l1, const Branch& l2) {
        return l1.time < l2.time;
    }

    bool operator>(const Branch& l1, const Branch& l2) {
        return l1.time > l2.time;
    }

    class BranchQueue : public std::priority_queue<Branch, std::vector<Branch>, std::greater<Branch>> {
    public:
        void push_min(const Branch& next);
    };

    int criticalPathMetric(Router* user, Service* consumer);
    Branch nearestHost(const Branch& branch);
}

#endif // SCENARIO_H
