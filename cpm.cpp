#include "cpm.h"

#include <boost/graph/adjacency_list.hpp>

#include <queue>
#include <utility>
#include <set>
#include <vector>
#include <algorithm>
#include <cassert>

namespace CPM {

unsigned
criticalPathMetric(Router user, Service consumer, const Topology& topo, const Workflow& work, bool scopt) {
    unsigned cpm{ 0 };

    // branch priority queue
    std::priority_queue<std::shared_ptr<Branch>, std::vector<std::shared_ptr<Branch>>, std::greater<std::shared_ptr<Branch>>> branches;
    branches.push(std::make_shared<Branch>(user, consumer, consumer));
    // already serviced interests
    std::set<std::pair<Router, Service>> expanded{};
    std::set<std::pair<Service, Service>> intr_expanded{};

    // pre-expand interests for scopt
    // necessary to support workflows with multiple sinks, or non-sink consumers
    if (scopt) {
        for (ServiceEdge e : iterpair(boost::edges(work))) {
            intr_expanded.insert({ boost::target(e, work), boost::source(e, work) });
        }

        std::queue<Service> srv_queue{};

        srv_queue.push(consumer);
        do {
            Service srv{ srv_queue.front() };
            srv_queue.pop();

            for (ServiceEdge e : iterpair(boost::in_edges(srv, work))) {
                srv_queue.push(source(e, work));
                intr_expanded.erase({ boost::target(e, work), boost::source(e, work) });
            }
        } while (!srv_queue.empty());
    }

    while (!branches.empty()) {
        std::shared_ptr<Branch> branch{ branches.top() };
        branches.pop();

        // skip exploratory branches if interest has been expanded
        if (intr_expanded.count({ branch->srv_prev, branch->srv })) {
            //std::cout << "Skipping interest from '" << work[branch->srv_prev].name << "' to '" << work[branch->srv].name << "' at t=" << branch->time << "\n";
            continue;
        }

        Router router = branch->rtr;
        Service service = branch->srv;
        unsigned time = branch->time;

        if (time > cpm)
            cpm = time;

        // skip branch if router has already sent interest for service
        if (expanded.count({ router, service })) {
            //std::cout << "Skipping '" << topo[router].name << "' looking for '" << work[service].name << "' requested by '" << work[branch->srv_prev].name << "' at t=" << time << "\n";
            // expand current interest, since this router satisfies it
            intr_expanded.insert({ branch->srv_prev, service });
            continue;
        }

        //std::cout << "On '" << topo[router].name << "' looking for '" << work[service].name << "' at t=" << time << "\n";

        // if hosting the service we're looking for
        if (topo[router].hosting.count(work[service].name)) {
            //std::cout << "Found '" << work[service].name << "' on '" << topo[router].name << "' at t=" << time << "\n";
            // expand router & interest
            expanded.insert({ router, service });
            intr_expanded.insert({ branch->srv_prev, branch->srv });

            // send interests for upstream services
            for (ServiceEdge e : iterpair(boost::in_edges(service, work))) {
                Service srv = boost::source(e, work);
                branches.push(std::make_shared<Branch>(router, srv, service, time));
            }

            // expand path which led here, send additional interests if scopt
            std::shared_ptr<Branch> br{ branch };
            while (br) {
                if (scopt) {
                    // dispatch interests for all services hosted on this router
                    for (const auto& srv_name : topo[br->rtr].hosting) {
                        Service srv{ work[boost::graph_bundle].map.at(srv_name) };
                        //std::cout << "Dispatching '" << work[srv].name << "' on '" << topo[br->rtr].name << "' at t=" << br->time << "\n";
                        branches.push(std::make_shared<Branch>(br->rtr, srv, srv, br->time));
                    }
                }
                expanded.insert({ br->rtr, service });
                br = br->prev;
            }
        } else {
            // take a single hop for current service in every direction
            // TODO: we backtrack here, which can generate a lot of noise
            // the noise will be killed once the interest is found, but if that
            // takes a long time it could be a performance problem
            for (RouterEdge e : iterpair(boost::out_edges(router, topo))) {
                unsigned cost = time + topo[e].cost;
                Router next = target(e, topo);
                branches.push(std::make_shared<Branch>(next, service, branch->srv_prev, cost, branch));
            }
        }

    }

    return cpm;
}

// TODO: cleanup redundant code

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
