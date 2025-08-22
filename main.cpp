#include "cpm.h"
#include "reader.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <chrono>

#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

using namespace CPM;

enum class Scheme {
    nesco,
    nescoSCOPT,
    orchA,
    orchB,
};

struct Config {
    std::string work_file{};
    std::string topo_file{};
    std::string host_file{};
    Scheme scheme{};
    std::string json_file{};
};

// TODO: args for supplying user/consumer names
Config argparse(int argc, char *argv[]) {
    Config cfg{};

    try {
        std::string scheme_str{};

        po::options_description desc("options");
        desc.add_options()
            ("help", "print help message")
            ("workflow,w", po::value<std::string>(&cfg.work_file), "workflow file")
            ("topology,t", po::value<std::string>(&cfg.topo_file), "topology file")
            ("hosting,h", po::value<std::string>(&cfg.host_file), "hosting file")
            ("scheme,s", po::value<std::string>(&scheme_str), "forwarding scheme")
            ("scenarioJSON,j", po::value<std::string>(&cfg.json_file), "scenario JSON file containing workflow, topology, hosting, etc.")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            std::exit(1);
        }

        po::notify(vm);

        // We can either use the generic scenario JSON file that contains all the workflow/topology/hosting information
        // or specify it in separate files
        if (vm.count("scenarioJSON")) { //if json option used
            std::cout << "JSON file: " << cfg.json_file << "\n";
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        std::exit(2);
    }

    return cfg;
}

std::string select_dag_for_consumer(const nlohmann::json& j) {
    for (const auto& host_obj : j.at("routerHosting")) {
        std::string service_name = host_obj.at("service");
        if (service_name == "/consumer") {
            if (host_obj.contains("dag")) {
                return host_obj.at("dag").get<std::string>();
            } else {
                std::cerr << "Error: /consumer service does not specify a DAG\n";
                std::exit(1);
            }
        }
    }
    std::cerr << "Error: /consumer service not found in routerHosting\n";
    std::exit(1);
}

int main(int argc, char *argv[])
{
    Config cfg = argparse(argc, argv);

    Topology topo;
    Workflow work;

    if (!cfg.json_file.empty()) {
        // Scenario JSON mode
        std::ifstream f{ cfg.json_file };
        if (!f) {
            std::cerr << "Error: could not open scenario JSON file " << cfg.json_file << "\n";
            std::exit(1);
        }

        nlohmann::json j = nlohmann::json::parse(f);

        // scheme
        std::string scheme_str = j.value("prefix", "");
        if (scheme_str == "nesco") {
            cfg.scheme = Scheme::nesco;
        } else if (scheme_str == "nescoSCOPT") {
            cfg.scheme = Scheme::nescoSCOPT;
        } else {
            std::cerr << "unknown scheme " << scheme_str << "\n";
            std::exit(2);
        }

        // Parse directly from JSON subtrees
        topo = topology_from_json(j);

        // Select DAG for /consumer
        std::string dag_name = select_dag_for_consumer(j);
        work = workflow_from_json(j, dag_name);

    } else {
        // Old file-based mode
        topo = Topology{ topology_from_files(cfg.topo_file.c_str(), cfg.host_file.c_str()) };
        work = Workflow{ workflow_from_file(cfg.work_file.c_str()) };
    }


    Router user{ topo[boost::graph_bundle].map.at("user") };
    // grabs first interest from consumer, consumer should not have more than
    // one interest
    ServiceEdge consumer_intr{ *in_edges(work[boost::graph_bundle].map.at("/consumer"), work).first };

    //print_graph(work);
    //print_graph(topo);

    bool scopt{};
    if (cfg.scheme == Scheme::nesco) {
        scopt = false;
    } else if (cfg.scheme == Scheme::nescoSCOPT) {
        scopt = true;
    } else {
        std::cerr << "impossible scheme value\n";
        std::exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    unsigned metric{ criticalPathMetric(user, consumer_intr, topo, work, scopt) };

    auto finish = std::chrono::high_resolution_clock::now();

    std::cout << "metric: " << metric << "\n";
    std::cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() << " ns\n";

    return 0;
}
