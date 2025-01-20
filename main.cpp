#include "cpm.h"
#include "reader.h"

#include <boost/json.hpp>
namespace json = boost::json;

#include <iostream>

using namespace CPM;

int main()
{
    Topology topo{ topology_from_files(TOPO_FILE, HOSTING_FILE) };
    Workflow work{ workflow_from_file(WORKFLOW_FILE) };

    Router user{ topo[boost::graph_bundle].map.at("user") };
    // just grabs first interest from consumer for now
    ServiceEdge interest{ *in_edges(work[boost::graph_bundle].map.at("/consumer"), work).first };

    std::cout << criticalPathMetric(user, interest, topo, work, true) << "\n";

    return 0;
}
