#include "cpm.h"

#include <boost/graph/adjacency_list.hpp>

#include <queue>
#include <utility>
#include <set>
#include <vector>

namespace CPM {

// TODO: add shortcut opt, return path to follow to closest host, generate
// interests along the path at each timestep
unsigned 
criticalPathMetric(Router user, ServiceEdge interest, const Topology& topo, Workflow& work) {
    unsigned cpm {0};

    // branch priority queue
    BranchQueue branches;
    branches.push(Branch { user, interest });
    // already serviced interests
    std::set<ServiceEdge> expanded{};

    while (!branches.empty()) {
        Branch branch = branches.top();
        branches.pop();

        ServiceEdge intr = branch.intr;
        Router rtr = branch.rtr;
        unsigned time = branch.time;

        // if already serviced, skip
        if (expanded.count(intr)) continue;

        Service srv = boost::source(intr, work);

        // priority queue gauruntees current time is minimum
        cpm = branch.time;

        // if hosting the service we're looking for
        if (topo[rtr].hosting.count(work[srv].name)) {
            expanded.insert(intr);
            // for upstream service of srv
            for (ServiceEdge e : iterpair(boost::in_edges(srv, work))) {
                // if already serviced, skip
                if (expanded.count(e)) continue;
                branches.push(Branch { rtr, e, time });
            }
        } else {
            // traverse to nearest host, adding necessary time
            branches.push(nearestHost(branch, topo, work));
        }
    }

    return cpm;
}

Branch nearestHost(Branch branch, const Topology& topo, const Workflow& work) {
    Service srv = boost::source(branch.intr, work);

    BranchQueue frontier {};
    frontier.push(branch);
    std::set<Router> expanded {};

    while (!frontier.empty()) {
        Branch close = frontier.top();
        frontier.pop();

        if (topo[close.rtr].hosting.count(work[srv].name)) {
            return close;
        }

        expanded.insert(close.rtr);

        for (RouterEdge e : iterpair(boost::out_edges(close.rtr, topo))) {
            Router next = target(e, topo);

            // skip if already expanded
            if (expanded.count(next)) 
                continue;

            unsigned cost = close.time + topo[e].cost;
            frontier.push_if_min(Branch { next, branch.intr, cost });
        }
    }

    throw std::runtime_error("not found lol");
}

std::vector<Branch> nearestHostPath(Branch branch, const Topology& topo, const Workflow& work) {
    Service srv = boost::source(branch.intr, work);

    BranchQueue frontier{};
    frontier.push(branch);
    std::set<Router> expanded{};
    std::map<Router, Branch> prev{};

    Branch close{};
    while (!frontier.empty()) {
        close = frontier.top();
        frontier.pop();

        if (topo[close.rtr].hosting.count(work[srv].name)) {
            // fuck you, my ass considered harmful
            // actually though i /could/ put all that code in here but it
            // interrupts the flow of the algorithm
            // imo it's better to have that code at the end
            goto found;
        }

        expanded.insert(close.rtr);

        for (RouterEdge e : iterpair(boost::out_edges(close.rtr, topo))) {
            Router next = target(e, topo);

            // skip if already expanded
            if (expanded.count(next)) 
                continue;

            unsigned cost = close.time + topo[e].cost;
            if (frontier.push_if_min(Branch { next, branch.intr, cost })) {
                prev[next] = close;
            }
        }
    }

    // TODO: lol
    throw std::runtime_error("not found lol");

found:
    std::vector<Branch> path{};
    Branch current{ close };

    do {
        path.push_back(current);
        current = prev[current.rtr];
    } while (current.rtr != branch.rtr);

    path.push_back(current);

    return std::vector(path.rbegin(), path.rend());
}

bool BranchQueue::push_if_min(Branch next) {
    for (auto& n : c) {
        if (n.rtr == next.rtr && n.intr == next.intr) {
            if (next.time < n.time) {
                n = next;
                std::sort_heap(c.begin(), c.end(), std::greater<Branch>());
                return true;
            }
            return false;
        }
    }
    this->push(next);
    return true;
}

// i looove copy pasting code it feels so good
Workflow::vertex_iterator findvertex(std::string_view name, const Workflow& g) {
    Workflow::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
        if (g[*vi].name == name) return vi;
    }
    return vi_end;
}

Topology::vertex_iterator findvertex(std::string_view name, const Topology& g) {
    Topology::vertex_iterator vi, vi_end;
    for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi) {
        if (g[*vi].name == name) return vi;
    }
    return vi_end;
}

//Scenario::Branch Scenario::nextHop(Scenario::Branch branch) {
//}

} // namespace CPM
