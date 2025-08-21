#ifndef READER_H
#define READER_H

#include "cpm.h"
#include <nlohmann/json.hpp>

namespace CPM {
Workflow workflow_from_file(const char* filename);
Topology topology_from_files(const char* topo_file, const char* hosting_file);
Workflow workflow_from_json(const nlohmann::json& j);
Topology topology_from_json(const nlohmann::json& j);
} // namespace CPM


#endif // READER_H
