#include "cpm.h"

#include <boost/graph/adjacency_list.hpp>

#include <queue>
#include <utility>
#include <set>
#include <vector>
#include <algorithm>

namespace CPM {

// TODO: optimize scopt code
// TODO: split scopt and non-scopt algos?
unsigned 
criticalPathMetric(Router user, ServiceEdge init_intr, const Topology& topo, const Workflow& work, bool scopt) {
    unsigned cpm {0};

    // branch priority queue
    BranchQueue branches;
    branches.push(Branch { user, init_intr });
    // already serviced interests
    std::set<ServiceEdge> intr_expanded{};
    std::set<Router> rtr_expanded{};

    // we need to pre-expand all interests irrelevant to finding the initial
    // interest or the algorithm will exhaust the entire workflow
    // this is unecessary for no scopt, since the algorithm won't ever see
    // those edges anyway
    // this only matters for non-sink initial interests
    if (scopt) {
        std::queue<Service> edge_queue{};
        std::set<ServiceEdge> edges{ init_intr };

        // traverse all upstream of init_intr
        edge_queue.push(source(init_intr, work));
        do {
            Service srv{ edge_queue.front() };
            edge_queue.pop();

            for (ServiceEdge e : iterpair(boost::in_edges(srv, work))) {
                if (edges.insert(e).second)
                    edge_queue.push(source(e, work));
            }
        } while (!edge_queue.empty());

        // put into set to gauruntee sort
        // not sure if necessary
        auto all_iter{ boost::edges(work) };
        std::set<ServiceEdge> all_edges(all_iter.first, all_iter.second);

        std::set_difference(all_edges.begin(), all_edges.end(), 
                edges.begin(), edges.end(), 
                std::inserter(intr_expanded, intr_expanded.end()));
    }

    while (!branches.empty()) {
        Branch branch = branches.top();
        branches.pop();

        ServiceEdge intr = branch.intr;
        // skip if already expanded
        if (intr_expanded.count(intr)) continue;

        Router rtr = branch.rtr;
        unsigned time = branch.time;
        Service service = boost::source(intr, work);

        // priority queue gauruntees current time is minimum
        cpm = time;

        // if hosting the service we're looking for
        if (topo[rtr].hosting.count(work[service].name)) {
            intr_expanded.insert(intr);
            // for upstream service of service
            for (ServiceEdge e : iterpair(boost::in_edges(service, work))) {
                // skip if already expanded
                if (intr_expanded.count(e)) continue;
                branches.push(Branch { rtr, e, time });
            }
        } else {
            // TODO: cleaup nested code?
            if (scopt) {
                std::vector<Branch> path{ nearestHostPath(branch, topo, work) };
                // for each step (router) on the path
                for (const auto& br : path) {
                    // skip already expanded routers
                    if (rtr_expanded.count(br.rtr)) continue;
                    rtr_expanded.insert(br.rtr);
                    // for each hosted service
                    for (const auto& srv_name : topo[br.rtr].hosting) {
                        const auto& desc_map{ work[boost::graph_bundle].map };
                        // if service in workflow
                        if (desc_map.count(srv_name)) {
                            Service srv { desc_map.at(srv_name) };
                            // for each upstream service
                            for (ServiceEdge up : iterpair(boost::in_edges(srv, work))) {
                                // skip if already expanded
                                if (intr_expanded.count(up)) continue;
                                if (up == intr) continue;
                                branches.push(Branch{ br.rtr, up, br.time });
                            }
                        }
                    }
                }
                branches.push(path.back());
            } else {
                // traverse to nearest host, adding necessary time
                branches.push(nearestHost(branch, topo, work));
            }
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

    // TODO: real error handling
    throw std::runtime_error("no host found");
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

        // if found
        if (topo[close.rtr].hosting.count(work[srv].name)) {
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

    // TODO: real error handling
    throw std::runtime_error("no host found");

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

// push to priority queue
// if already present, push only if higher priority
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

// TODO: cleanup repeat code

Service add_vertex(const std::string& name, Workflow& g) {
    auto& name_map{ g[boost::graph_bundle].map };
    if (!name_map.count(name)) {
        name_map[name] = boost::add_vertex(ServiceProperty{ name }, g);
    }
    return name_map[name];
}

std::pair<ServiceEdge, bool> add_edge(const std::string& name1, const std::string& name2, Workflow& g) {
    Service u = add_vertex(name1, g);
    Service v = add_vertex(name2, g);
    return add_edge(u, v, g);
}

Router add_vertex(const std::string& name, Topology& g) {
    auto& name_map{ g[boost::graph_bundle].map };
    if (!name_map.count(name)) {
        name_map[name] = boost::add_vertex(RouterProperty{ name }, g);
    }
    return name_map[name];
}

std::pair<RouterEdge, bool> add_edge(const std::string& name1, const std::string& name2, Topology& g) {
    Router u = add_vertex(name1, g);
    Router v = add_vertex(name2, g);
    return add_edge(u, v, g);
}

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

void print_graph(const Workflow& g) {
    std::cout << "Edges:\n";
    for (auto e : iterpair(boost::edges(g))) {
        auto src = source(e, g);
        auto tgt = target(e, g);
        std::cout << g[src].name << " (" << src << ") -> " 
            << g[tgt].name << " (" << tgt << ")\n";
    }
    std::cout << "Descriptor Map:\n";
    for (const auto& [key, val] : g[boost::graph_bundle].map) {
        std::cout << key << ": " << val << "\n";
    }
}

void print_graph(const Topology& g) {
    std::cout << "Edges:\n";
    for (auto e : iterpair(boost::edges(g))) {
        auto src = source(e, g);
        auto tgt = target(e, g);
        std::cout << g[src].name << " (" << src << ") -> " 
            << g[tgt].name << " (" << tgt << ")\n";
    }
    std::cout << "Hosting:\n";
    for (auto v : iterpair(boost::vertices(g))) {
        std::cout << g[v].name << ": ";
        for (const auto& srv_name : g[v].hosting) {
            std::cout << srv_name << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "Descriptor Map:\n";
    for (const auto& [key, val] : g[boost::graph_bundle].map) {
        std::cout << key << ": " << val << "\n";
    }
}

} // namespace CPM
