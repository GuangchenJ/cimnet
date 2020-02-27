#ifndef CIMNET_BASE_NET
#define CIMNET_BASE_NET

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

#include "_types.h"
#include "_exception.h"


struct HashPair {
    template <class T1, class T2>
        std::size_t operator() (std::pair<T1, T2> const &p) const {
            std::size_t h1 = std::hash<T1>()(p.first);
            std::size_t h2 = std::hash<T2>()(p.second);
            return h1 ^ h2;
        }
};

template <class _NId=Id, class _NData=None, class _EData=None>
class Network;
template <class _NId=Id, class _NData=None, class _EData=None>
class DirectedNetwork;

/* Base class of undirected network */
template <class _NId, class _NData, class _EData>
class Network {
    typedef std::unordered_map<_NId, _NData> _NType;
    typedef std::pair<_NId, _NId> _EPairType;
    typedef std::unordered_map<_NId, _EData *> _NeiType;
    typedef std::unordered_map<_NId, _NeiType> _AdjType;
    typedef Network<_NId, _NData, _EData> _NetType;
    typedef DirectedNetwork<_NId, _NData, _EData> _DiNetType;
    typedef std::unordered_set<std::pair<_NId, _NId>, HashPair> _ESetType;
    typedef std::unordered_set<_NId> _NeiSetType;

    friend std::ostream& operator<<(std::ostream& out, const Network& net) {
        out << "Network {#(node)=" << net.number_of_nodes()
            << ", #(edge)=" << net.number_of_edges()
            << ", #(degree)=" << net.total_degree() << "}";
        return out;
    }

    public:
    Network (): _nodes(), _adjs() {}
    Network (const _NetType &net) : _nodes(), _adjs() {
        for (auto n: net._nodes)
            add_node(n.first, n.second);
        for (auto e: net.edges())
            add_edge(e.first, e.second, net.get_edge_data(e.first, e.second));
    }
    Network (const _DiNetType &net) : _nodes(), _adjs() {
        for (auto n: net._nodes)
            add_node(n.first, n.second);
        for (auto e: net.edges())
            add_edge(e.first, e.second, net.get_edge_data(e.first, e.second));
    }

    ~Network () {
        for (auto &n : nodes())
            remove_node(n);
    }

    inline _NId add_node(const _NId &id,
            const _NData &node_data=_NData()) {
        _nodes[id] = node_data;
        _adjs[id] = _NeiType();
        return id;
    }

    inline void add_edge(const _NId &id1, const _NId &id2,
            const _EData &edge_data=_EData()) {
        if (!has_node(id1)) add_node(id1);
        if (!has_node(id2)) add_node(id2);
        if (has_edge(id1, id2)) delete _adjs[id1][id2];
        _EData *data_ptr = new _EData();
        *data_ptr = edge_data;
        _adjs[id1][id2] = data_ptr;
        _adjs[id2][id1] = data_ptr;
    }

    inline void remove_edge(const _NId &id1, const _NId &id2) {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        if (!has_edge(id1, id2)) throw NoEdgeException<_NId>(id1, id2);
        _NeiType &nei1 = _adjs.at(id1);
        _NeiType &nei2 = _adjs.at(id2);
        _EData *data_ptr = nei1.at(id2);
        delete data_ptr;
        nei1.erase(nei1.find(id2));
        nei2.erase(nei2.find(id1));
    }

    inline void remove_node(const _NId &id) {
        if (!has_node(id)) throw NoNodeException<_NId>(id);
        for (auto &n : neighbors(id))
            remove_edge(id, n);
        _adjs.erase(_adjs.find(id));
        _nodes.erase(_nodes.find(id));
    }

    inline bool has_node(const _NId &id) const {
        return _nodes.find(id) != _nodes.end();
    }

    inline bool has_edge(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1) || !has_node(id2))
            return false;
        return _adjs.at(id1).find(id2) != _adjs.at(id1).end();
    }

    inline bool is_neighbor(const _NId &id1, const _NId &id2) const {
        return has_edge(id1, id2);
    }

    inline _NData &node(const _NId &id) {
        if (!has_node(id))
            throw NoNodeException<_NId>(id);
        return _nodes.at(id);
    }

    /* TODO: Optimize code structure of edge and get_edge_data. */
    inline _EData &edge(const _NId &id1, const _NId &id2) {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        try {
            return *(_adjs.at(id1).at(id2));
        } catch (std::exception e){
            throw NoEdgeException<_NId>(id1, id2);
        }
    }

    inline _EData get_edge_data(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        try {
            return *(_adjs.at(id1).at(id2));
        } catch (std::exception e){
            throw NoEdgeException<_NId>(id1, id2);
        }
    }

    inline int number_of_nodes() const {
        return _nodes.size();
    }

    inline int number_of_edges() const {
        return total_degree() / 2;
    }

    inline int total_degree() const {
        int degree = 0;
        for (auto &adj : _adjs)
            degree += adj.second.size();
        return degree;
    }

    inline int degree(const _NId &id) const {
        if (!has_node(id)) return 0;
        return _adjs.at(id).size();
    }

    inline const _AdjType &adjacency() const {
        return _adjs;
    }

    inline std::vector<_NId> neighbors(const _NId &id) const {
        std::vector<_NId> nei;
        if (has_node(id))
            for (auto &n : _adjs.at(id))
                nei.push_back(n.first);
        return nei;
    }

    inline std::vector<_NId> nodes() const {
        std::vector<_NId> nei;
        for (auto &n : _nodes)
            nei.push_back(n.first);
        return nei;
    }

    inline _ESetType edges() const {
        _ESetType e;
        for (auto &i : _adjs)
            for (auto &j : i.second) {
                _NId _i = i.first, _j = j.first;
                if (_i > _j) std::swap(_i, _j);
                e.insert(std::make_pair(_i, _j));
            }
        return e;
    }

    inline _NData &operator[](const _NId &id) {
        return node(id);
    }

    private:
    _NType _nodes;
    _AdjType _adjs;
};

/* Base class of directed network */
template <class _NId, class _NData, class _EData>
class DirectedNetwork {
    typedef std::unordered_map<_NId, _NData> _NType;
    typedef std::pair<_NId, _NId> _EPairType;
    typedef std::unordered_map<_NId, _EData *> _NeiType;
    typedef std::unordered_map<_NId, _NeiType> _AdjType;
    typedef Network<_NId, _NData, _EData> _NetType;
    typedef DirectedNetwork<_NId, _NData, _EData> _DiNetType;
    typedef std::unordered_set<std::pair<_NId, _NId>, HashPair> _ESetType;
    typedef std::unordered_set<_NId> _NeiSetType;

    friend std::ostream& operator<<(std::ostream& out, const DirectedNetwork& net) {
        out << "Directed network {#(node)=" << net.number_of_nodes()
            << ", #(edge)=" << net.number_of_edges()
            << ", #(degree)=" << net.total_degree() << "}";
        return out;
    }

    public:
    DirectedNetwork (): _nodes(), _pred(), _succ() {}
    DirectedNetwork (const _DiNetType &net) : _nodes(), _pred(), _succ() {
        for (auto n: net._nodes)
            add_node(n.first, n.second);
        for (auto e: net.edges())
            add_edge(e.first, e.second, net.get_edge_data(e.first, e.second));
    }
    DirectedNetwork (const _NetType &net): _nodes(), _pred(), _succ() {
        for (auto n: net._nodes)
            add_node(n.first, n.second);
        for (auto e: net.edges()) {
            add_edge(e.first, e.second, net.get_edge_data(e.first, e.second));
            add_edge(e.second, e.first, net.get_edge_data(e.first, e.second));
        }
    }

    ~DirectedNetwork () {
        for (auto &n : nodes())
            remove_node(n);
    }

    inline _NId add_node(const _NId &id,
            const _NData &node_data=_NData()) {
        _nodes[id] = node_data;
        _pred[id] = _NeiType();
        _succ[id] = _NeiType();
        return id;
    }

    inline void add_edge(const _NId &id1, const _NId &id2,
            const _EData &edge_data=_EData()) {
        if (!has_node(id1)) add_node(id1);
        if (!has_node(id2)) add_node(id2);
        if (has_edge(id1, id2)) delete _succ[id1][id2];
        _EData *data_ptr = new _EData();
        *data_ptr = edge_data;
        _succ[id1][id2] = data_ptr;
        _pred[id2][id1] = data_ptr;
    }

    inline void remove_edge(const _NId &id1, const _NId &id2) {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        if (!has_edge(id1, id2)) throw NoEdgeException<_NId>(id1, id2, true);
        _NeiType &succ = _succ.at(id1);
        _NeiType &pred = _pred.at(id2);
        _EData *data_ptr = succ.at(id2);
        delete data_ptr;
        succ.erase(succ.find(id2));
        pred.erase(pred.find(id1));
    }

    inline void remove_node(const _NId &id) {
        if (!has_node(id)) throw NoNodeException<_NId>(id);
        for (auto &n : successors(id))
            remove_edge(id, n);
        for (auto &n : predecessors(id))
            remove_edge(n, id);
        _succ.erase(_succ.find(id));
        _pred.erase(_pred.find(id));
        _nodes.erase(_nodes.find(id));
    }

    inline bool has_node(const _NId &id) const {
        return _nodes.find(id) != _nodes.end();
    }

    inline bool has_successor(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1) || !has_node(id2)) return false;
        return _succ.at(id1).find(id2) != _succ.at(id1).end();
    }

    inline bool has_predecessor(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1) || !has_node(id2)) return false;
        return _pred.at(id1).find(id2) != _pred.at(id1).end();
    }

    inline bool has_edge(const _NId &id1, const _NId &id2) const {
        return has_successor(id1, id2);
    }

    inline bool is_neighbor(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1) || !has_node(id2)) return false;
        return _succ.at(id1).find(id2) != _succ.at(id1).end() ||
               _pred.at(id1).find(id2) != _pred.at(id1).end();
    }

    inline _NData &node(const _NId &id) {
        if (!has_node(id))
            throw NoNodeException<_NId>(id);
        return _nodes.at(id);
    }

    /* TODO: Optimize code structure of edge and get_edge_data. */
    inline _EData &edge(const _NId &id1, const _NId &id2) {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        try {
            return *(_succ.at(id1).at(id2));
        } catch (std::exception e){
            throw NoEdgeException<_NId>(id1, id2);
        }
    }

    inline _EData get_edge_data(const _NId &id1, const _NId &id2) const {
        if (!has_node(id1)) throw NoNodeException<_NId>(id1);
        if (!has_node(id2)) throw NoNodeException<_NId>(id2);
        try {
            return *(_succ.at(id1).at(id2));
        } catch (std::exception e){
            throw NoEdgeException<_NId>(id1, id2);
        }
    }

    inline int number_of_nodes() const {
        return _nodes.size();
    }

    inline int number_of_edges() const {
        int num = 0;
        for (auto &adj : _succ)
            num += adj.second.size();
        return num;
    }

    inline int total_degree() const {
        return number_of_edges() * 2;
    }

    inline int in_degree(const _NId &id) const {
        if (!has_node(id)) return 0;
        return _pred.at(id).size();
    }

    inline int out_degree(const _NId &id) const {
        if (!has_node(id)) return 0;
        return _succ.at(id).size();
    }

    inline int degree(const _NId &id) const {
        if (!has_node(id)) return 0;
        return _pred.at(id).size() + _succ.at(id).size();
    }

    inline const _AdjType &succ_adjacency() const {
        return _succ;
    }

    inline const _AdjType &pred_adjacency() const {
        return _pred;
    }

    inline std::vector<_NId> successors(const _NId &id) const {
        std::vector<_NId> nei;
        if (has_node(id))
            for (auto &n : _succ.at(id))
                nei.push_back(n.first);
        return nei;
    }

    inline std::vector<_NId> predecessors(const _NId &id) const {
        std::vector<_NId> nei;
        if (has_node(id))
            for (auto &n : _pred.at(id))
                nei.push_back(n.first);
        return nei;
    }

    inline std::vector<_NId> neighbors(const _NId &id) const {
        _NeiSetType nei;
        if (has_node(id)) {
            for (auto &n : _succ.at(id))
                nei.insert(n.first);
            for (auto &n : _pred.at(id))
                nei.insert(n.first);
        }
        return std::vector<_NId>(nei);
    }

    inline std::vector<_NId> nodes() const {
        std::vector<_NId> nei;
        for (auto &n : _nodes)
            nei.push_back(n.first);
        return nei;
    }

    inline _ESetType edges() const {
        _ESetType e;
        for (auto &i : _succ)
            for (auto &j : i.second)
                e.insert(std::make_pair(i.first, j.first));
        return e;
    }

    inline _NData &operator[](const _NId &id) {
        return node(id);
    }

    private:
    _NType _nodes;
    _AdjType _pred;
    _AdjType _succ;   /* _adjs */
};

#endif /* ifndef CIMNNET_BASE_NET */
