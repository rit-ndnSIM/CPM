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

    Router user{ *findvertex("user", topo) };
    // for now, just grab the first interest
    // assumes "/consumer" exists and segfaults when you try to use it if it
    // doesn't
    ServiceEdge interest{ *in_edges(*findvertex("/consumer", work), work).first };

    std::cout << criticalPathMetric(user, interest, topo, work) << "\n";

    return 0;
}
