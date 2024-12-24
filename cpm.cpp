#include "cpm.h"

#include <queue>
#include <utility>
#include <set>
#include <vector>

namespace CPM {
    // TODO: add shortcut opt, return path to follow to closest host, generate
    // interests along the path at each timestep
    unsigned 
    criticalPathMetric(Router user, ServiceEdge interest, const Topology& topo, Workflow& work) {
        for (ServiceEdge e : iterpair(boost::edges(work))) {
            work[e].requested = false;
        }

        unsigned cpm {0};

        BranchQueue branches;
        branches.push(Branch { user, interest });

        while (!branches.empty()) {
            Branch branch = branches.top();
            branches.pop();

            ServiceEdge intr = branch.intr;
            Router rtr = branch.rtr;
            unsigned time = branch.time;

            // if already requested, skip
            if (work[intr].requested) continue;

            Service srv = boost::source(intr, work);

            // priority queue gauruntees current time is maximum
            cpm = branch.time;

            // if hosting the service we're looking for
            if (topo[rtr].hosting.count(work[srv].name)) {
                // for upstream service of srv
                for (ServiceEdge e : iterpair(boost::in_edges(srv, work))) {
                    // if already requested, skip
                    if (work[e].requested) continue;
                    branches.push(Branch { rtr, e, time });
                    work[e].requested = true;
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
                frontier.push_min(Branch { next, branch.intr, cost });
            }
        }

        throw std::runtime_error("you fucked up dumbo haha");
    }

    void BranchQueue::push_min(Branch next) {
        for (auto& n : c) {
            if (n.rtr == next.rtr) {
                if (next.time < n.time) {
                    n.time = next.time;
                    std::sort_heap(c.begin(), c.end(), std::greater<Branch>());
                }
                return;
            }
        }
        this->push(next);
    }

    //Scenario::Branch Scenario::nextHop(Scenario::Branch branch) {
    //}
}
