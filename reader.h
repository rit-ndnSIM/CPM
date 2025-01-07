#ifndef READER_H
#define READER_H

#include "cpm.h"

#include <boost/json.hpp>

#define WORKFLOW_FILE "4dag.json"
#define HOSTING_FILE "4dag.hosting"
#define TOPO_FILE "topo-cabeee-3node.json"

namespace CPM {
Workflow workflow_from_file(const char* filename);
boost::json::value parse_file(const char* filename);
Topology topology_from_files(const char* topo_file, const char* hosting_file);
} // namespace CPM


#endif // READER_H
