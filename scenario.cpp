#include "scenario.h"

#include <queue>
#include <utility>
#include <set>
#include <vector>

namespace CPM {
    // no shortcut opt
    //
    // THE PLAN:
    // use a priority queue with the current timestep; do earlier things first
    // don't generate upstream interests at a branch until all ealier branches
    // are consumed
    int criticalPathMetric(Router* user, Service* consumer) {
        for (auto& link : consumer->m_graph.links()) {
            link.data().requested = false;
        }

        BranchQueue branches;
        branches.push(Branch { user, consumer });

        while (!branches.empty()) {
            Branch branch = branches.top();
            branches.pop();

            // if hosting the service we're looking for
            if (branch.rtr->data().hosts(branch.srv->data().name)) {
                // for upstream service of srv
                for (auto& uplink : branch.srv->incoming()) {
                    // if already requested, skip
                    if (uplink.data().requested) continue;
                    branches.push(Branch { branch.rtr, &uplink.from() });
                    uplink.data().requested = true;
                }
            } else {
                // traverse to nearest host, adding necessary time
                branches.push(nearestHost(branch));
            }
        }
    }

    Branch nearestHost(const Branch& branch) {
        auto* srv = branch.srv;
        BranchQueue frontier {};
        frontier.push(branch);
        std::set<Router*> expanded {};

        while (!frontier.empty()) {
            Branch close_node = frontier.top();
            frontier.pop();

            if (close_node.rtr->data().hosts(srv->data().name)) {
                return close_node;
            }

            expanded.insert(close_node.rtr);

            for (auto& link : close_node.rtr->incoming()) {
                Router* next = &link.to();

                // skip if already expanded
                if (expanded.count(next)) 
                    continue;

                unsigned cost = close_node.time + link.data().cost;
                frontier.push_min(Branch { next, srv, cost });
            }
        }

        return Branch { NULL };
    }

    void BranchQueue::push_min(const Branch& next) {
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
