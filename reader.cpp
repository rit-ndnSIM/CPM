#include "reader.h"
#include "cpm.h"

#include <boost/json.hpp>

#include <string_view>
#include <iomanip>
#include <fstream>
#include <iostream>

namespace json = boost::json;

namespace CPM {

json::value parse_file(const char* filename) {
    std::ifstream f{ filename };
    json::stream_parser p;
    boost::system::error_code ec;

    if (!f) return nullptr;

    do {
        char buf[4096];
        f.read(buf, sizeof(buf));
        // why the hell is streamsize signed??
        p.write(buf, static_cast<size_t>(f.gcount()), ec);
    } while(!f.eof());

    if (ec) return nullptr;

    p.finish(ec);
    if (ec) return nullptr;

    return p.release();
}

Workflow workflow_from_file(const char* filename) {
    // error checking?
    const json::value work_json = parse_file(filename);
    Workflow work{};

    const auto& work_obj = work_json.at("dag").get_object();

    for (const auto& pair1 : work_obj) {
        for (const auto& pair2 : pair1.value().get_object()) {
            add_edge(std::string(pair1.key()), std::string(pair2.key()), work);
        }
    }

    return work;
}

Topology topology_from_files(const char* topo_file, const char* hosting_file) {
    // error checking
    const json::value topo_json = parse_file(topo_file);
    const json::value hosting_json = parse_file(hosting_file);

    Topology topo{};

    // error checking is for chumps
    for (const auto& rtr_obj : topo_json.at("router").get_array()) {
        std::string name{ rtr_obj.at("node").get_string() };
        add_vertex(name, topo);
    }

    for (const auto& link_obj : topo_json.at("link").get_array()) {
        std::string from{ link_obj.at("from").get_string() };
        std::string to{ link_obj.at("to").get_string() };
        //std::string delay{ link_obj.at("delay").get_string() };
        RouterEdge e{ add_edge(from, to, topo).first };
        // TODO: parse delay unit into value, hardcode 1 for now
        topo[e].cost = 1;
    }

    for (const auto& host_pair : hosting_json.at("routerHosting").get_object()) {
        std::string name{ host_pair.key() };
        Router rtr{ topo[boost::graph_bundle].map.at(name) };

        for (const auto& srv : host_pair.value().get_array()) {
            topo[rtr].hosting.insert(static_cast<std::string>(srv.get_string()));
        }
    }

    return topo;
}

} // namespace CPM
