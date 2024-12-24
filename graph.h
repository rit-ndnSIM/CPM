#ifndef GRAPH_H
#define GRAPH_H

#include <memory>
#include <stdexcept>
#include <functional>

namespace CPM {
    // helper class to interate over linked lists
    template <typename T>
    class GraphIterator {
        using NextFn = std::function<T*(T*)>;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = void;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        GraphIterator(pointer ptr, NextFn prev, NextFn next) 
            : m_ptr { ptr },
              m_prev { prev },
              m_next { next }
        {}

        GraphIterator(nullptr_t)
            : m_ptr { nullptr },
              m_prev { nullptr },
              m_next { nullptr }
        {}

        GraphIterator(const GraphIterator& a)
            : m_ptr { a.m_ptr },
              m_prev { a.m_prev },
              m_next { a.m_next }
        {}

        GraphIterator& operator=(const GraphIterator&) = delete;

        reference operator*() { return *m_ptr; }
        pointer operator->() { return m_ptr; }
        GraphIterator& operator++() { m_ptr = m_next(m_ptr); return *this; }
        GraphIterator operator++(int) { GraphIterator tmp{*this}; ++(*this); return tmp; }
        GraphIterator& operator--() { m_ptr = m_prev(m_ptr); return *this; }
        GraphIterator operator--(int) { GraphIterator tmp{*this}; --(this); return tmp; }
        friend bool operator==(const GraphIterator& a, const GraphIterator& b) { return a.m_ptr == b.m_ptr; }
        friend bool operator!=(const GraphIterator& a, const GraphIterator& b) { return a.m_ptr != b.m_ptr; }

        GraphIterator& begin() { return *this; }
        GraphIterator end() { return GraphIterator{nullptr}; }

    private:
        pointer m_ptr;
        NextFn m_prev;
        NextFn m_next;
    };

    // modified from https://github.com/6502/cppgraph
    template<typename ND, typename LD>
    class Graph {
    public:
        class Link;

        class Node {
        public:
            ND m_data;
            Graph& m_graph;
            Node *m_prev, *m_next;
            Link *m_firstIn, *m_lastIn, *m_firstOut, *m_lastOut;

        public:
            Node(ND data, Graph& graph)
                : m_data { std::move(data) },
                  m_graph { graph },
                  m_prev { graph.m_lastNode }, m_next{ nullptr },
                  m_firstIn { nullptr }, m_lastIn { nullptr },
                  m_firstOut { nullptr }, m_lastOut { nullptr }
            {
                if (m_prev) m_prev->m_next = this; else m_graph.m_firstNode = this;
                m_graph.m_lastNode = this;
            }

            Node(Graph& graph)
                : Node(ND(), graph)
            {}

            ~Node() {
                while (m_lastIn) delete m_lastIn;
                while (m_lastOut) delete m_lastOut;
                if (m_prev) m_prev->m_next = m_next; else m_graph.m_firstNode = m_next;
                if (m_next) m_next->m_prev = m_prev; else m_graph.m_lastNode = m_prev;
            }

            Node(const Node&) = delete;
            Node(Node&&) = delete;
            Node& operator=(const Node&) = delete;

            ND& data() { return m_data; }

            GraphIterator<Link> incoming() {
                return GraphIterator<Link>(
                    m_firstIn, 
                    [](Link* link){ return link->m_prevInTo; },
                    [](Link* link){ return link->m_nextInTo; }
                );
            }

            GraphIterator<Link> outgoing() {
                return GraphIterator<Link>(
                    m_firstOut,
                    [](Link* link){ return link->m_prevInFrom; },
                    [](Link* link){ return link->m_nextInFrom; }
                );
            }

            friend class Graph;
            friend class Link;
        };

        class Link {
        public:
            LD m_data;
            Node *m_from, *m_to;
            Link *m_prev, *m_next, *m_prevInFrom, *m_nextInFrom, *m_prevInTo, *m_nextInTo;

        public:
            Link(LD data, Node *from, Node *to)
                : m_data(std::move(data)),
                  m_from(from), m_to(to),
                  m_prev(from->m_graph.m_lastLink), m_next(nullptr),
                  m_prevInFrom(from->m_lastOut), m_nextInFrom(nullptr),
                  m_prevInTo(to->m_lastIn), m_nextInTo(nullptr)
            {
                if (&m_from->m_graph != &m_to->m_graph) {
                    throw std::runtime_error("Cannot link nodes from different graphs");
                }
                if (m_prev) m_prev->m_next = this; else m_from->m_graph.m_firstLink = this;
                m_from->m_graph.m_lastLink = this;
                if (m_prevInFrom) m_prevInFrom->m_nextInFrom = this; else m_from->m_firstOut = this;
                m_from->m_lastOut = this;
                if (m_prevInTo) m_prevInTo->m_nextInTo = this; else m_to->m_firstIn = this;
                m_to->m_lastIn = this;
            }

            Link(Node *from, Node *to)
                : Link(LD(), from, to)
            {}

            ~Link() {
                if (m_prevInTo) m_prevInTo->m_nextInTo = m_nextInTo; else m_to->m_firstIn = m_nextInTo;
                if (m_nextInTo) m_nextInTo->m_prevInTo = m_prevInTo; else m_to->m_lastIn = m_prevInTo;
                if (m_prevInFrom) m_prevInFrom->m_nextInFrom = m_nextInFrom; else m_from->m_firstOut = m_nextInFrom;
                if (m_nextInFrom) m_nextInFrom->m_prevInFrom = m_prevInFrom; else m_from->m_lastOut = m_prevInFrom;
                if (m_prev) m_prev->m_next = m_next; else m_from->m_graph.m_firstLink = m_next;
                if (m_next) m_next->m_prev = m_prev; else m_from->m_graph.m_lastLink = m_prev;
            }

            Link(const Link&) = delete;
            Link(Link&&) = delete;
            Link& operator=(const Link&) = delete;

            LD& data() { return m_data; }
            Node& from() { return *m_from; }
            Node& to() { return *m_to; }

            friend class Graph;
            friend class Node;
        };

    public:

        Node *m_firstNode, *m_lastNode;
        Link *m_firstLink, *m_lastLink;

    public:
        Graph()
            : m_firstNode(nullptr), m_lastNode(nullptr),
              m_firstLink(nullptr), m_lastLink(nullptr)
        {}

        ~Graph() {
            while (m_lastNode) delete m_lastNode;
        }

        Graph(const Graph&) = delete;
        Graph(Graph&&) = delete;
        Graph& operator=(const Graph&) = delete;

        Node* addNode(ND data) { return new Node(data, *this); }
        Link* addLink(LD data, Node *from, Node *to) { return new Link(data, from, to); }
        Node* addNode() { return new Node(this); }
        Link* addLink(Node *from, Node *to) { return new Link(from, to); }

        GraphIterator<Node> nodes() {
            return GraphIterator<Node>(
                m_firstNode,
                [](Node* node){ return node->m_prev; },
                [](Node* node){ return node->m_next; }
            );
        }

        GraphIterator<Link> links() {
            return GraphIterator<Link>(
                m_firstLink,
                [](Link* link){ return link->m_prev; },
                [](Link* link){ return link->m_next; }
            );
        }
    };
}

#endif // GRAPH_H
