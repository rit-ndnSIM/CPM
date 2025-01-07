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
    // error checking
    const json::value work_json = parse_file(filename);
    Workflow work {};

    std::map<std::string, Service> desc_map{};

    const auto& work_obj = work_json.at("dag").get_object();

    for (const auto& pair1 : work_obj) {
        // if name not yet grabbed
        std::string name1 = pair1.key();
        if (!desc_map.count(name1)) {
            desc_map[name1] = boost::add_vertex(ServiceProperty{ name1 }, work);
        }

        for (const auto& pair2 : pair1.value().get_object()) {
            std::string name2 = pair2.key();
            if (!desc_map.count(name2)) {
                desc_map[name2] = boost::add_vertex(ServiceProperty{ name2 }, work);
            }

            boost::add_edge(desc_map[name1], desc_map[name2], work);
        }
    }

    return work;
}

Topology topology_from_files(const char* topo_file, const char* hosting_file) {
    // error checking
    const json::value topo_json = parse_file(topo_file);
    const json::value hosting_json = parse_file(hosting_file);

    Topology topo{};

    std::map<std::string, std::set<std::string>> hosting_map{};

    for (const auto& host_pair : hosting_json.at("routerHosting").get_object()) {
        std::string name{ host_pair.key() };
        std::set<std::string> hosting{};

        for (const auto& srv : host_pair.value().get_array()) {
            hosting.insert(static_cast<std::string>(srv.get_string()));
        }

        hosting_map[name] = hosting;
    }

    std::map<std::string, Router> desc_map{};

    // error checking is for chumps
    for (const auto& rtr_obj : topo_json.at("router").get_array()) {
        std::string name{ rtr_obj.at("node").get_string() };
        RouterProperty rtr_prop{ name, hosting_map[name] };
        desc_map[name] = boost::add_vertex(rtr_prop, topo);
    }

    for (const auto& link_obj : topo_json.at("link").get_array()) {
        std::string from{ link_obj.at("from").get_string() };
        std::string to{ link_obj.at("to").get_string() };
        // TODO: parse delay unit into value
        RouterEdgeProperty edge_prop{ 1 };
        boost::add_edge(desc_map[from], desc_map[to], edge_prop, topo);
    }

    return topo;
}

} // namespace CPM
