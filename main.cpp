#include "graph.h"

#include <iostream>

using namespace CPM;

int main()
{
    using CGraph = Graph<char, nullptr_t>;
    CGraph g {};
    auto* a {g.addNode('a')};
    auto* b {g.addNode('b')};
    auto* x {g.addNode('x')};
    auto* c {g.addNode('c')};
    g.addLink(a, b);
    g.addLink(b, c);
    g.addLink(c, a);
    g.addLink(a, x);

    auto dump = [&](){
        for (CGraph::Node& node : g.nodes()) {
            printf("node '%c'\n", node.data());
            for (CGraph::Link& link : node.outgoing()) {
                 printf(" %c → '%c'\n", node.data(), link.to().data());
            }
            for (CGraph::Link& link : node.incoming()) {
                 printf(" %c ← '%c'\n", node.data(), link.from().data());
            }
        }
    };

    dump();
    puts("");
    for (CGraph::Link& link : a->outgoing()) {
        delete &link;
    }
    dump();

    return 0;
}
