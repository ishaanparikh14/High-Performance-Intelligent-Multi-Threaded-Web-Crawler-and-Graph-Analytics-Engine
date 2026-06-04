#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// graph_algorithms.h
//
// DAA-grade graph algorithms that operate on the completed web graph.
//
//   1. Tarjan's SCC (O(V+E))        — finds strongly connected components
//   2. SCC Condensation DAG          — contracts each SCC to a super-node
//   3. Topological Sort on DAG (O(V+E)) — orders the condensed DAG
//   4. KMP String Matching (O(n+m)) — keyword search in crawled content
//
// All algorithms include detailed complexity annotations for DAA reporting.
// ─────────────────────────────────────────────────────────────────────────────

#ifndef GRAPH_ALGORITHMS_H
#define GRAPH_ALGORITHMS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>
#include <functional>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Data Structures
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Result of Tarjan SCC analysis.
 *
 * Complexity guarantees:
 *   Time:  O(V + E)   — every node/edge visited exactly once
 *   Space: O(V)       — discovery/lowlink arrays + recursion stack
 */
struct SCCResult {
    // scc_id[node_index] → which SCC this node belongs to (0-based)
    std::vector<int>              scc_id;

    // sccs[k] → list of node-indices in SCC k
    std::vector<std::vector<int>> sccs;

    // Number of SCCs found
    int  num_sccs  = 0;

    // Size of the largest SCC
    int  max_scc_size = 0;

    // Index of the largest SCC
    int  max_scc_index = 0;
};

/**
 * The condensed DAG produced by SCC contraction.
 *
 * Nodes are SCC ids (0 .. num_sccs-1).
 * Edges connect SCCs that have at least one inter-SCC original edge.
 * This graph is guaranteed acyclic, enabling topological sort.
 *
 * Complexity: O(V + E) to build (single pass over original edges)
 */
struct CondensedDAG {
    int num_nodes = 0;               // == SCCResult::num_sccs
    // adj[u] → list of SCC super-nodes reachable from super-node u
    std::vector<std::vector<int>> adj;
};

/**
 * KMP search result for a single keyword.
 */
struct KMPMatch {
    std::string keyword;
    int         count       = 0;   // total occurrences across all documents
    std::vector<std::string> found_in_domains; // domains where keyword appeared
};

// ─────────────────────────────────────────────────────────────────────────────
// GraphAlgorithms class
// ─────────────────────────────────────────────────────────────────────────────

class GraphAlgorithms {
public:
    // ── Tarjan SCC ────────────────────────────────────────────────────────────
    /**
     * Run Tarjan's SCC algorithm on an adjacency-list graph.
     *
     * @param adj         Adjacency list  adj[u] = {v1, v2, …}  (integer node ids)
     * @param num_nodes   Total number of nodes (nodes are 0..num_nodes-1)
     * @return            SCCResult with all SCC membership data
     *
     * Algorithm:
     *   - DFS with a discovery timestamp and low-link value per node.
     *   - A node is an SCC root when disc[v] == low[v] after the DFS subtree.
     *   - All nodes on the stack above the root form the SCC.
     *
     * Time Complexity:  O(V + E)
     * Space Complexity: O(V)  (disc, low, on_stack arrays + explicit DFS stack)
     *
     * Note: Uses an explicit stack to avoid recursion depth limits on large graphs.
     */
    static SCCResult tarjan_scc(
        const std::vector<std::vector<int>>& adj,
        int num_nodes);

    // ── SCC Condensation ─────────────────────────────────────────────────────
    /**
     * Build the condensed DAG from SCC results.
     *
     * @param adj        Original adjacency list
     * @param scc_result Result of tarjan_scc()
     * @return           Condensed DAG (guaranteed acyclic)
     *
     * Time Complexity: O(V + E)
     * Space Complexity: O(V + E)  (new adjacency list for super-nodes)
     */
    static CondensedDAG build_condensed_dag(
        const std::vector<std::vector<int>>& adj,
        const SCCResult& scc_result);

    // ── Topological Sort ──────────────────────────────────────────────────────
    /**
     * Kahn's algorithm (BFS-based) topological sort on the condensed DAG.
     *
     * @param dag        Condensed DAG from build_condensed_dag()
     * @return           Topological ordering (list of SCC super-node ids)
     *                   Empty if a cycle is detected (should not happen on a DAG).
     *
     * Algorithm:
     *   1. Compute in-degree for each node.
     *   2. Enqueue all zero-in-degree nodes.
     *   3. Process queue: remove node, decrement neighbours' in-degree,
     *      enqueue newly zero-in-degree neighbours.
     *
     * Why not DFS topo sort?
     *   Kahn's is iterative (no stack overflow) and produces a canonical
     *   ordering that matches natural BFS-level discovery.
     *
     * Time Complexity:  O(V + E)
     * Space Complexity: O(V)
     */
    static std::vector<int> topological_sort(const CondensedDAG& dag);

    // ── KMP String Matching ───────────────────────────────────────────────────
    /**
     * KMP failure function (prefix table).
     *
     * @param pattern   The search pattern
     * @return          Failure array of length pattern.size()
     *
     * Time Complexity:  O(m)  where m = pattern.length()
     * Space Complexity: O(m)
     */
    static std::vector<int> kmp_failure_function(const std::string& pattern);

    /**
     * Count occurrences of `pattern` in `text` using KMP.
     *
     * @param text      Text to search in
     * @param pattern   Pattern to find
     * @return          Number of (possibly overlapping) occurrences
     *
     * Time Complexity:  O(n + m)
     * Space Complexity: O(m)  (failure function table)
     */
    static int kmp_search(const std::string& text, const std::string& pattern);

    /**
     * Search multiple keywords across a map of domain → page content.
     *
     * @param content_map  { domain → html/text content }
     * @param keywords     List of keywords to search
     * @return             KMPMatch results for each keyword
     *
     * Time Complexity: O(K × Σ(n_i + m_k))
     *   where K = keywords, n_i = content length per domain, m_k = keyword length
     */
    static std::vector<KMPMatch> kmp_multi_search(
        const std::unordered_map<std::string, std::string>& content_map,
        const std::vector<std::string>& keywords);

    // ── Convenience wrapper for string-keyed graphs ───────────────────────────
    /**
     * Run full SCC pipeline on a domain graph (string-keyed adjacency list).
     *
     * Internally maps string domains to integer ids for Tarjan, then maps back.
     *
     * @param graph   { domain → [linked_domain, ...] }
     * @return        SCCResult with integer ids;
     *                use node_labels() to map back to domain strings.
     */
    struct DomainSCCResult {
        SCCResult                scc;
        std::vector<std::string> node_labels;  // index → domain string
        std::unordered_map<std::string, int> node_index; // domain → index

        /** Human-readable SCC listing: SCC id → list of domain strings */
        std::vector<std::vector<std::string>> scc_domains() const;

        /** Topological order as domain strings (per SCC super-node) */
        std::vector<std::string> topo_order_labels(
            const std::vector<int>& topo_order) const;
    };

    static DomainSCCResult run_scc_on_domain_graph(
        const std::unordered_map<std::string, std::vector<std::string>>& graph);

    /**
     * Run topological sort on the SCC condensed DAG derived from the domain graph.
     * Returns the condensed DAG and topological ordering.
     *
     * @param domain_scc   Result from run_scc_on_domain_graph()
     * @param graph        Original domain graph (needed to build condensed DAG)
     */
    struct TopoResult {
        CondensedDAG     dag;
        std::vector<int> order;          // topo order of SCC super-nodes
    };

    static TopoResult run_topo_on_scc(
        const DomainSCCResult& domain_scc,
        const std::unordered_map<std::string, std::vector<std::string>>& graph);
};

#endif // GRAPH_ALGORITHMS_H
