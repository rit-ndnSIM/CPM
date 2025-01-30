#include "cpm.h"
#include "reader.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <chrono>

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
};

// TODO: args for supplying user/consumer names
Config argparse(int argc, char *argv[]) {
    Config cfg{};

    try {
        std::string scheme_str{};

        po::options_description desc("options");
        desc.add_options()
            ("help", "print help message")
            ("workflow,w", po::value<std::string>(&cfg.work_file)->required(), "workflow file")
            ("topology,t", po::value<std::string>(&cfg.topo_file)->required(), "topology file")
            ("hosting,h", po::value<std::string>(&cfg.host_file)->required(), "hosting file")
            ("scheme,s", po::value<std::string>(&scheme_str)->required(), "forwarding scheme")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            std::exit(0);
        }

        po::notify(vm);

        if (scheme_str == "nesco") {
            cfg.scheme = Scheme::nesco;
        } else if (scheme_str == "nescoSCOPT") {
            cfg.scheme = Scheme::nescoSCOPT;
        } else {
            std::cerr << "unknown scheme " << scheme_str << "\n";
            std::exit(2);
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        std::exit(2);
    }

    return cfg;
}

int main(int argc, char *argv[])
{
    Config cfg = argparse(argc, argv);

    Topology topo{ topology_from_files(cfg.topo_file.c_str(), cfg.host_file.c_str()) };
    Workflow work{ workflow_from_file(cfg.work_file.c_str()) };

    Router user{ topo[boost::graph_bundle].map.at("user") };
    // just grabs first interest from consumer for now
    ServiceEdge consumer_intr{ *out_edges(work[boost::graph_bundle].map.at("/consumer"), work).first };

    //print_graph(work);
    //print_graph(topo);

    bool scopt{};
    if (cfg.scheme == Scheme::nesco) {
        scopt = false;
    } else if (cfg.scheme == Scheme::nescoSCOPT) {
        scopt = true;
    } else {
        std::cerr << "good job you broke it dumbass\n";
        std::exit(1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    
    unsigned metric{ criticalPathMetric(user, consumer_intr, topo, work, scopt) };

    auto finish = std::chrono::high_resolution_clock::now();

    std::cout << "metric: " << metric << "\n";
    std::cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() << " ns\n";

    return 0;
}
