#include "reader.h"

#include "cpm.h"

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

namespace CPM {

Workflow workflow_from_file(const char* filename) {
    std::ifstream f{ filename };
    const json work_json = json::parse(f);

    // TODO: error checking?
    Workflow work{};

    for (const auto& item1 : work_json.at("dag").items()) {
        for (const auto& item2 : item1.value().items()) {
            add_edge(item1.key(), item2.key(), work);
        }
    }

    return work;
}

Topology topology_from_files(const char* topo_file, const char* hosting_file) {
    // TODO: better error checking
    std::ifstream topo_f{ topo_file };
    std::ifstream host_f{ hosting_file };
    const json topo_json = json::parse(topo_f);
    const json hosting_json = json::parse(host_f);

    Topology topo{};

    for (const auto& rtr_obj : topo_json.at("router")) {
        std::string name{ rtr_obj.at("node") };
        add_vertex(name, topo);
    }

    for (const auto& link_obj : topo_json.at("link")) {
        std::string from{ link_obj.at("from") };
        std::string to{ link_obj.at("to") };
        //std::string delay{ link_obj.at("delay") };
        RouterEdge e{ add_edge(from, to, topo).first };
        // TODO: parse delay unit into value, hardcode 1 for now
        topo[e].cost = 1;
    }

    for (const auto& host_item : hosting_json.at("routerHosting").items()) {
        std::string name{ host_item.key() };
        Router rtr{ topo[boost::graph_bundle].map.at(name) };

        for (const auto& srv : host_item.value()) {
            topo[rtr].hosting.insert(static_cast<std::string>(srv));
        }
    }

    return topo;
}

} // namespace CPM
