#!/usr/bin/env python

import json
import re
import copy

import sys, argparse
import time

from collections import defaultdict, deque, namedtuple

# TODO: add caching
# generally make more optimized??

class Scenario(object):
    """ All scenario information, including workflow and topology """

    def __init__(self, workflow, topology, hosting):
        self.workflow = Workflow(workflow)
        self.topology = Topology(topology)
        self.hosting = Hosting(hosting)

    def build_interest_tree(self, user, consumer, shortcutOPT=False):
        """ Builds an interest tree for the scenario """

        workflow = self.workflow.clone()
        tree = Graph([], True)
        Branch = namedtuple('Branch', ['router', 'service', 'time'])
        branches = deque()
        branches.appendleft(Branch(user, consumer, 0))

        while len(branches) > 0:
            current_branch = branches.pop()
            router, interest_service, time = current_branch

            if not self.hosting.is_hosting(router, interest_service): 
                # if not hosting, take a hop
                next_router = self.next_hop(router, interest_service)
                next_branch = Branch(next_router, interest_service, time+1)
                branches.appendleft(next_branch)
                tree.add(current_branch, next_branch)
            else:
                # if hosted, generate interests for get upstream service
                for upstream_service in workflow.get_incoming(interest_service):
                    next_branch = Branch(router, upstream_service, time)
                    # .append to handle immediately, keeps things synced (i think)
                    branches.append(next_branch)

                    # mark as accounted for by removing connection
                    workflow.remove_connection(upstream_service, interest_service)
                    tree.add(current_branch, next_branch)

            # repeat code here but it's fine
            # shortcutOPT
            if shortcutOPT:
                for hosted_service in self.hosting.get_hosting(router):
                    for upstream_service in workflow.get_incoming(hosted_service):
                        next_branch = Branch(router, upstream_service, time)
                        branches.append(next_branch)
                        workflow.remove_connection(upstream_service, hosted_service)
                        tree.add(current_branch, next_branch)

        return tree

    def critical_path_metric(self, user, consumer, shortcutOPT=False):
        """ Get critical path for distributed orchestration """

        self.check_scenario(user, consumer)

        tree = self.build_interest_tree(user, consumer, shortcutOPT)

        terminations = tree.get_sinks()

        metric = max(branch.time for branch in terminations)

        return metric

    def orch_a_critical_path_metric(self, user, consumer):
        """ Get critical path metric for orchestrator A """

        self.check_scenario(user, consumer)

        branches = deque()
        dist = {}

        roots = self.workflow.get_roots()
        for root in roots:
            dist[root] = len(self.shortest_service_path(user, root))
            branches.extendleft(self.workflow.get_outgoing(root))

        while len(branches) > 0:
            service = branches.pop()

            path = self.shortest_service_path(user, service)
            if path:
                router = path[-1]
            else:
                router = user
            # reflexive interest, interest for inputs, interest for service
            hops = len(path)

            upstream_services = self.workflow.get_incoming(service)
            # account for previous hops
            hops += max(dist[upstream] for upstream in upstream_services)
            # account for inputs to service (sent from this node)
            # assuming it does this in parallel? i see no reason why it would not
            # also, assume consumer is hosted on orchestrator and these don't need grabbed
            # probably a better way to handle this
            if service != consumer:
                hops += max(len(self.shortest_service_path(router, upstream)) for upstream in upstream_services)

            dist[service] = hops

            branches.extendleft(self.workflow.get_outgoing(service))

        return dist[consumer]

    def orch_b_critical_path_metric(self, user, consumer):
        """ Get critical path metric for orchestrator B """

        self.check_scenario(user, consumer)

        branches = deque()
        dist = {}

        roots = self.workflow.get_roots()
        for root in roots:
            dist[root] = len(self.shortest_service_path(user, root))
            branches.extendleft(self.workflow.get_outgoing(root))

        while len(branches) > 0:
            service = branches.pop()

            path = self.shortest_service_path(user, service)
            # reflexive interest, interest for inputs, interest for service
            hops = 3 * len(path)

            upstream_services = self.workflow.get_incoming(service)
            # account for previous hops
            hops += max(dist[upstream] for upstream in upstream_services)

            dist[service] = hops

            branches.extendleft(self.workflow.get_outgoing(service))

        return dist[consumer]

    def check_scenario(self, user, consumer):
        if not self.hosting.is_hosting(user, consumer):
            raise ValueError(f"Specified user host '{user}' not hosting consumer '{consumer}'")

        if consumer not in self.workflow.get_nodes():
            print(self.workflow)
            raise ValueError(f"Specified consumer '{consumer}' not in workflow")

        if user not in self.topology.get_nodes():
            raise ValueError(f"Specified user host '{user}' not in topology")

    def shortest_service_path(self, router, service):
        """ Returns an array of routers to visit to reach the nearest router hosting service """

        if self.hosting.is_hosting(router, service):
            return []

        paths = []

        for dest_router in self.hosting.get_hosts(service):
            paths.append(self.topology.shortest_path(router, dest_router))

        shortest_path = min(paths, key=len)

        return shortest_path

    def next_hop(self, router, service):
        """ Returns the next closest router to service """

        if self.hosting.is_hosting(router, service):
            return router

        return self.shortest_service_path(router, service)[0]

    def __str__(self):
        return '{}({}, {}, {})'.format(self.__class__.__name__, self.workflow, self.topology, self.hosting)
        

# https://stackoverflow.com/questions/19472530/representing-graphs-data-structure-in-python  
class Graph(object):
    """ Graph data structure, undirected by default. """

    def __init__(self, connections=[], directed=False):
        self._graph = defaultdict(set)
        self._directed = directed
        self.add_connections(connections)

    def clone(self):
        """ Create a clone of the object """

        return __class__([(node, service) for node, services in self._graph.items() for service in services], self._directed)

    def add_connections(self, connections):
        """ Add connections (list of tuple pairs) to graph """

        for node1, node2 in connections:
            self.add(node1, node2)

    def add(self, node1, node2):
        """ Add connection between node1 and node2 """

        self._graph[node1].add(node2)
        if not self._directed:
            self._graph[node2].add(node1)

    def remove(self, node):
        """ Remove all references to node """

        delete_nodes = set()
        for n, cxns in self._graph.items():
            if node in cxns:
                cxns.remove(node)

            if not cxns:
                delete_nodes.add(n)

        for n in delete_nodes:
            del self._graph[n]

        if node in self._graph:
            del self._graph[node]

    def remove_connection(self, upstream, downstream):
        """ Remove a downstream between upstreams """

        cxns = self._graph[upstream]

        if downstream in cxns:
            cxns.remove(downstream)

        if not cxns:
            del self._graph[upstream]

    def is_connected(self, upstream=None, downstream=None):
        """
            Is upstream directly connected to downstream
            If an argument is none, check against all nodes
        """

        if upstream is None and downstream is None:
            return bool(self._graph)

        if downstream is None:
            return bool(self.get_outgoing(upstream))

        if upstream is None:
            return bool(self.get_incoming(downstream))

        return upstream in self._graph and downstream in self._graph[upstream]

    def contains(self, node):
        """ Does graph contain node """

        if node in self._graph:
            return True

        if any(node in nodes for nodes in self._graph.values()):
            return True

        return False

    def get_outgoing(self, node=None):
        """
            Get all outgoing connections of node
            Passing no argument will get outgoing connections of all nodes
        """

        if node is None:
            return set(node for cxns in self._graph.values() for node in cxns)

        if node not in self._graph:
            return set()

        return self._graph[node]

    def get_incoming(self, node=None):
        """
            Get all incoming nodes of node
            Passing no argument will get outgoing nodes of all nodes
        """

        if node is None:
            return set(self._graph.keys())

        return set(incoming for incoming, cxns in self._graph.items() if node in cxns)

    def get_roots(self):
        """ Get roots, which are any nodes without incoming connections """

        return self.get_incoming() - self.get_outgoing()

    def get_sinks(self):
        """ Get sinks, which are any nodes without outgoing connections """
        return self.get_outgoing() - self.get_incoming()

    def get_nodes(self):
        """ Get all nodes in graph """

        # if a node is in the graph but has no outgoing connections, self._graph[node] will not exist
        return self.get_outgoing() | self.get_incoming()

    # TODO: some sort of state to not re-do work each time we call this
    # may or may not be necessary for complex topologies
    # only implement caching if we have a performance issue
    # premature optimization is the root of all evil
    def shortest_path(self, source, target):
        """ Find the shortest path using Djikstra's algorithm """

        unvisited_nodes = self.get_incoming() & self.get_outgoing()
        dist = {node: float('inf') for node in unvisited_nodes}
        dist[source] = 0
        prev = dict()

        while len(unvisited_nodes) > 0:
            node = min(unvisited_nodes, key=dist.get)
            unvisited_nodes.remove(node)

            if node == target:
                break

            for neighbor in self.get_outgoing(node):
                alt = dist[node] + 1
                if alt < dist[neighbor]:
                    dist[neighbor] = alt
                    prev[neighbor] = node

        # return a list of nodes to take

        if target not in prev:
            return None

        path = []
        node = target
        while node != source:
            path.append(node)
            node = prev[node]

        path.reverse()

        return path

    def __str__(self):
        return '{}({})'.format(self.__class__.__name__, dict(self._graph))


class Workflow(Graph):
    """ Workflow data structure, directed graph """

    def __init__(self, connections):
        super().__init__(connections, True)

    def clone(self):
        """ Create a clone of the object """

        return __class__([(node, service) for node, services in self._graph.items() for service in services])

    def prune_downstream(self, service):
        if not self.contains(service):
            return None

        sinks = self.get_sinks()
        while (sinks):
            sink = sinks.pop()
            if sink != service:
                self.remove(sink)
                sinks.update(self.get_sinks())

        return self

    def prune_upstream(self, service):
        if not self.contains(service):
            return None

        roots = self.get_roots()
        while (roots):
            root = roots.pop()
            if root != service:
                self.remove(root)
                roots.update(self.get_roots())

        return self

    def prune_split(self, service):
        return (self.clone().prune_downstream(service), self.clone().prune_upstream(service))


class Topology(Graph):
    """ Topology data structure, undirected graph """

    def clone(self):
        """ Create a clone of the object """

        return __class__([(node, service) for node, services in self._graph.items() for service in services])

    def __init__(self, connections):
        super().__init__(connections, False)


class Hosting(Graph):
    """ Hosting data structure """

    def clone(self):
        """ Create a clone of the object """

        return __class__([(node, service) for node, services in self._graph.items() for service in services])

    def __init__(self, connections):
        super().__init__(connections, True)
        self.get_hosts = self.get_incoming
        self.get_hosting = self.get_outgoing
        self.is_hosting = self.is_connected


def workflow_connections_from_file(workflow_path):
    with open(workflow_path) as f:
        workflow_json = json.load(f)
        workflow_connections = [(parent, child) for parent, child_dict in workflow_json['dag'].items() for child in child_dict.keys()]

    return workflow_connections


def topology_connections_from_file(topology_path):
    with open(topology_path) as f:
        lines = iter(f.readlines())

        while (not next(lines).startswith("link")):
            pass
        
        # TODO: this logic could be a lot more robust, but it should work for now
        pattern = re.compile(r"^([a-zA-Z0-9_-]+)\s+([a-zA-Z0-9_-]+).*$")
        topology_connections = list()

        for line in lines:
            if line.startswith(('#', '\n')):
                continue

            matches = pattern.match(line)
            topology_connections.append((matches.group(1), matches.group(2)))

    return topology_connections
    

def hosting_connections_from_file(hosting_path):
    with open(hosting_path) as f:
        hosting_json = json.load(f)
        hosting_connections = [(key, service) for key, services in hosting_json['routerHosting'].items() for service in services]

    return hosting_connections
 

def scenario_from_files(workflow_path, topology_path, hosting_path):
    workflow_connections = workflow_connections_from_file(workflow_path)
    topology_connections = topology_connections_from_file(topology_path)
    hosting_connections = hosting_connections_from_file(hosting_path)

    return Scenario(workflow_connections, topology_connections, hosting_connections)


def main():
    #workflow_file = "workflows/4dag.json"
    #topology_file = "topologies/topo-cabeee-3node.txt"
    #hosting_file = "workflows/4dag.hosting"

    #workflow_file = "workflows/8dag.json"
    #topology_file = "topologies/topo-cabeee-3node.txt"
    #hosting_file = "workflows/8dag.hosting"
    
    #workflow_file = "workflows/20-parallel.json"
    #topology_file = "topologies/topo-cabeee-20node-parallel.txt"
    #hosting_file = "workflows/20-parallel.hosting"
    
    #workflow_file = "workflows/20-sensor.json"
    #topology_file = "topologies/topo-cabeee-20sensor.txt"
    #hosting_file = "workflows/20-sensor.hosting"

    #workflow_file = "workflows/20-linear.json"
    #topology_file = "topologies/topo-cabeee-3node.txt"
    #hosting_file = "workflows/20-linear-in3node.hosting"

    #workflow_file = "workflows/20-linear.json"
    #topology_file = "topologies/topo-cabeee-20node-linear.txt"
    #hosting_file = "workflows/20-linear.hosting"

    # Parse command line options
    parser = argparse.ArgumentParser()
    parser.add_argument('-type', type=str, default=None, help='Command line option to enter forwarding scheme (nescoSCOPT, orchA or orchB)')
    parser.add_argument('-workflow', type=str, default=None, help='Command line option to enter workflow file name')
    parser.add_argument('-topology', type=str, default=None, help='Command line option to enter topology file name')
    parser.add_argument('-hosting', type=str, default=None, help='Command line option to enter hosting file name')
    args = parser.parse_args()


    if args.type is None:
        print("Please enter a fowarding type (nesco, nescoSCOPT, orchA or orchB")
        sys.exit(0)
    if args.workflow is None:
        print("Please enter a workflow, topology, and hosting file using the command line options")
        sys.exit(0)
    if args.topology is None:
        print("Please enter a workflow, topology, and hosting file using the command line options")
        sys.exit(0)
    if args.hosting is None:
        print("Please enter a workflow, topology, and hosting file using the command line options")
        sys.exit(0)

    workflow_file = args.workflow
    topology_file = args.topology
    hosting_file = args.hosting

    start_time = time.time()

    scenario = scenario_from_files(workflow_file, topology_file, hosting_file)

    #tree = scenario.build_interest_tree("user", "/consumer")

    #print(tree)

    #metric = scenario.critical_path_metric("user", "/consumer")

    if (args.type == 'nesco'):
        metric = scenario.critical_path_metric("user", "/consumer", False)
    if (args.type == 'nescoSCOPT'):
        metric = scenario.critical_path_metric("user", "/consumer", True)
    if (args.type == 'orchA'):
        metric = scenario.orch_a_critical_path_metric("user", "/consumer")
    if (args.type == 'orchB'):
        metric = scenario.orch_b_critical_path_metric("user", "/consumer")


    #tree = scenario.build_interest_tree("user", "/consumer")
    end_time = time.time()

    print(f"metric is {metric}")
    print(f"time is {(end_time - start_time)}")

if __name__ == '__main__':
    main()

# vim: tw=0 shiftwidth=4 expandtab tabstop=8 softtabstop=8 smarttab autoindent
