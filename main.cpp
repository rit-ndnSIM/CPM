#include "cpm.h"
#include "reader.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>

using namespace CPM;

enum class Scheme {
    nesco,
    nescoSCOPT,
    orchA,
    orchB,
};

int main(int argc, char *argv[])
{
    std::string workflow_file{};
    std::string topology_file{};
    std::string hosting_file{};
    Scheme scheme{};

    try {
        std::string scheme_str{};

        po::options_description desc("options");
        desc.add_options()
            ("help", "print help message")
            ("workflow,w", po::value<std::string>(&workflow_file)->required(), "workflow file")
            ("topology,t", po::value<std::string>(&topology_file)->required(), "topology file")
            ("hosting,h", po::value<std::string>(&hosting_file)->required(), "hosting file")
            ("scheme,s", po::value<std::string>(&scheme_str)->required(), "forwarding scheme")
        ;

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (scheme_str == "nesco") {
            scheme = Scheme::nesco;
        } else if (scheme_str == "nescoSCOPT") {
            scheme = Scheme::nescoSCOPT;
        } else {
            std::cerr << "unknown scheme " << scheme_str << "\n";
            std::exit(2);
        }

    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        std::exit(2);
    }

    Topology topo{ topology_from_files(topology_file.c_str(), hosting_file.c_str()) };
    Workflow work{ workflow_from_file(workflow_file.c_str()) };

    // TODO: args for user/consumer
    Router user{ topo[boost::graph_bundle].map.at("user") };
    // just grabs first interest from consumer for now
    ServiceEdge consumer_intr{ *in_edges(work[boost::graph_bundle].map.at("/consumer"), work).first };


    bool scopt{};
    if (scheme == Scheme::nesco) {
        scopt = false;
    } else if (scheme == Scheme::nescoSCOPT) {
        scopt = true;
    } else {
        std::cerr << "you're mom lol\n";
        std::exit(1);
    }
    
    unsigned metric{ criticalPathMetric(user, consumer_intr, topo, work, scopt) };

    std::cout << "metric: " << metric << "\n";

    return 0;
}
