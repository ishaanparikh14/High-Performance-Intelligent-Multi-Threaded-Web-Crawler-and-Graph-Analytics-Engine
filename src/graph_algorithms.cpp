// ─────────────────────────────────────────────────────────────────────────────
// graph_algorithms.cpp
//
// Implements:
//   1. Tarjan's SCC           O(V + E)   — iterative (no recursion depth issues)
//   2. SCC Condensation DAG   O(V + E)
//   3. Kahn's Topological Sort O(V + E)
//   4. KMP String Matching    O(n + m)   per search
//
// All algorithms are self-contained and DAA-grade.
// ─────────────────────────────────────────────────────────────────────────────

#include "graph_algorithms.h"
#include <queue>
#include <iostream>
#include <cassert>

// ─────────────────────────────────────────────────────────────────────────────
// 1. Tarjan's SCC  —  O(V + E)
//
// Classic algorithm using discovery timestamps and low-link values.
// Uses an EXPLICIT STACK instead of recursion to avoid stack overflow on
// large graphs (web graphs can have thousands of nodes).
//
// Low-link value: the smallest disc[] reachable from the subtree of v.
// SCC root condition: disc[v] == low[v]  after fully exploring v's subtree.
// ─────────────────────────────────────────────────────────────────────────────

SCCResult GraphAlgorithms::tarjan_scc(
    const std::vector<std::vector<int>>& adj,
    int num_nodes)
{
    SCCResult result;
    if (num_nodes == 0) return result;

    result.scc_id.assign(num_nodes, -1);

    // Per-node state
    std::vector<int>  disc(num_nodes, -1);   // discovery time (-1 = unvisited)
    std::vector<int>  low(num_nodes, 0);     // low-link value
    std::vector<bool> on_stack(num_nodes, false);

    std::vector<int>  tarjan_stack;          // Tarjan's bookkeeping stack
    tarjan_stack.reserve(num_nodes);

    int timer = 0;  // monotonically increasing discovery clock

    // ── Iterative DFS frame ──────────────────────────────────────────────────
    // We simulate the recursive call stack explicitly.
    // Each frame stores: (node, index into adj[node] we're currently processing)
    struct Frame {
        int node;
        int child_idx;   // next child to visit in adj[node]
    };

    std::vector<Frame> call_stack;
    call_stack.reserve(num_nodes);

    // Process each component (handles disconnected graphs)
    for (int start = 0; start < num_nodes; ++start) {
        if (disc[start] != -1) continue;  // already visited

        call_stack.push_back({start, 0});
        disc[start]     = low[start] = timer++;
        on_stack[start] = true;
        tarjan_stack.push_back(start);

        while (!call_stack.empty()) {
            Frame& frame = call_stack.back();
            int    v     = frame.node;

            if (frame.child_idx < static_cast<int>(adj[v].size())) {
                // There are still children to process
                int w = adj[v][frame.child_idx++];

                if (disc[w] == -1) {
                    // Tree edge: push w onto the call stack
                    disc[w]     = low[w] = timer++;
                    on_stack[w] = true;
                    tarjan_stack.push_back(w);
                    call_stack.push_back({w, 0});
                } else if (on_stack[w]) {
                    // Back edge: update low-link
                    // disc[w] (not low[w]) per Tarjan's original formulation
                    low[v] = std::min(low[v], disc[w]);
                }
                // Cross/forward edges: ignored (no update needed)
            } else {
                // Finished processing all children of v
                call_stack.pop_back();

                // Propagate low-link to parent
                if (!call_stack.empty()) {
                    int parent = call_stack.back().node;
                    low[parent] = std::min(low[parent], low[v]);
                }

                // SCC root detection: disc[v] == low[v]
                if (disc[v] == low[v]) {
                    // Pop the SCC from Tarjan's stack
                    std::vector<int> scc;
                    while (true) {
                        int w = tarjan_stack.back();
                        tarjan_stack.pop_back();
                        on_stack[w]     = false;
                        result.scc_id[w] = result.num_sccs;
                        scc.push_back(w);
                        if (w == v) break;
                    }
                    result.sccs.push_back(std::move(scc));
                    result.num_sccs++;
                }
            }
        }
    }

    // ── Post-process: find largest SCC ──────────────────────────────────────
    result.max_scc_size  = 0;
    result.max_scc_index = 0;
    for (int i = 0; i < result.num_sccs; ++i) {
        if (static_cast<int>(result.sccs[i].size()) > result.max_scc_size) {
            result.max_scc_size  = static_cast<int>(result.sccs[i].size());
            result.max_scc_index = i;
        }
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. SCC Condensation  —  O(V + E)
//
// Contract each SCC into a single super-node.
// The resulting graph is a DAG (by construction, since any cycle within an
// SCC is now contained in a single node).
//
// Self-loops in the condensed graph are suppressed (intra-SCC edges).
// Parallel edges are deduplicated using a visited set per source super-node.
// ─────────────────────────────────────────────────────────────────────────────

CondensedDAG GraphAlgorithms::build_condensed_dag(
    const std::vector<std::vector<int>>& adj,
    const SCCResult& scc_result)
{
    CondensedDAG dag;
    dag.num_nodes = scc_result.num_sccs;
    dag.adj.resize(dag.num_nodes);

    // Use an unordered_set per source SCC to avoid duplicate edges
    std::vector<std::unordered_set<int>> edge_set(dag.num_nodes);

    for (int u = 0; u < static_cast<int>(adj.size()); ++u) {
        int scc_u = scc_result.scc_id[u];
        for (int v : adj[u]) {
            int scc_v = scc_result.scc_id[v];
            if (scc_u != scc_v) {   // skip intra-SCC edges (would be self-loops)
                if (edge_set[scc_u].insert(scc_v).second) {
                    dag.adj[scc_u].push_back(scc_v);
                }
            }
        }
    }

    return dag;
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Kahn's Topological Sort  —  O(V + E)
//
// BFS-based topological sort.
// We prefer Kahn's over DFS-based topological sort because:
//   a) It is iterative (no recursion depth risk).
//   b) It naturally detects cycles (output.size() < V means cycle present).
//   c) It produces a canonical ordering consistent with BFS frontier expansion.
//
// Input:  A DAG (guaranteed acyclic when used on the condensed SCC graph).
// Output: Topological ordering of super-node ids.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<int> GraphAlgorithms::topological_sort(const CondensedDAG& dag)
{
    int N = dag.num_nodes;
    std::vector<int> in_degree(N, 0);

    // Compute in-degrees  O(V + E)
    for (int u = 0; u < N; ++u) {
        for (int v : dag.adj[u]) {
            in_degree[v]++;
        }
    }

    // Enqueue all zero-in-degree nodes  O(V)
    std::queue<int> q;
    for (int u = 0; u < N; ++u) {
        if (in_degree[u] == 0) q.push(u);
    }

    std::vector<int> order;
    order.reserve(N);

    // BFS relaxation  O(V + E)
    while (!q.empty()) {
        int u = q.front(); q.pop();
        order.push_back(u);

        for (int v : dag.adj[u]) {
            if (--in_degree[v] == 0) {
                q.push(v);
            }
        }
    }

    // Cycle detection (should not trigger on a valid condensed DAG)
    if (static_cast<int>(order.size()) != N) {
        std::cerr << "[WARN] topological_sort: cycle detected in DAG "
                     "(order.size()=" << order.size()
                  << " N=" << N << ")\n";
        return {};
    }

    return order;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. KMP — Failure Function  O(m)
//
// Builds the prefix table (partial-match table) for the pattern.
// failure[i] = length of the longest proper prefix of pattern[0..i]
//              that is also a suffix.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<int> GraphAlgorithms::kmp_failure_function(const std::string& pattern)
{
    int m = static_cast<int>(pattern.size());
    std::vector<int> fail(m, 0);

    int k = 0;  // length of previous longest prefix suffix
    for (int i = 1; i < m; ++i) {
        // Fall back along the border links until we find a match or reach 0
        while (k > 0 && pattern[k] != pattern[i]) {
            k = fail[k - 1];
        }
        if (pattern[k] == pattern[i]) {
            ++k;
        }
        fail[i] = k;
    }
    return fail;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4b. KMP Search  —  O(n + m)
//
// Count all (possibly overlapping) occurrences of pattern in text.
// ─────────────────────────────────────────────────────────────────────────────

int GraphAlgorithms::kmp_search(const std::string& text, const std::string& pattern)
{
    if (pattern.empty()) return 0;

    int n    = static_cast<int>(text.size());
    int m    = static_cast<int>(pattern.size());
    int count = 0;

    std::vector<int> fail = kmp_failure_function(pattern);

    int q = 0;  // number of chars matched so far
    for (int i = 0; i < n; ++i) {
        while (q > 0 && text[i] != pattern[q]) {
            q = fail[q - 1];
        }
        if (text[i] == pattern[q]) {
            ++q;
        }
        if (q == m) {
            // Full match ending at position i
            ++count;
            // Allow overlapping matches by falling back through failure link
            q = fail[q - 1];
        }
    }
    return count;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4c. KMP Multi-Search  —  O(K × Σ(n_i + m_k))
//
// Search each keyword across all domain content entries.
// ─────────────────────────────────────────────────────────────────────────────

std::vector<KMPMatch> GraphAlgorithms::kmp_multi_search(
    const std::unordered_map<std::string, std::string>& content_map,
    const std::vector<std::string>& keywords)
{
    std::vector<KMPMatch> results;
    results.reserve(keywords.size());

    for (const auto& keyword : keywords) {
        KMPMatch match;
        match.keyword = keyword;

        for (const auto& [domain, content] : content_map) {
            int hits = kmp_search(content, keyword);
            if (hits > 0) {
                match.count += hits;
                match.found_in_domains.push_back(domain);
            }
        }
        results.push_back(std::move(match));
    }

    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. DomainSCCResult helpers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::vector<std::string>>
GraphAlgorithms::DomainSCCResult::scc_domains() const
{
    std::vector<std::vector<std::string>> result(scc.num_sccs);
    for (int i = 0; i < static_cast<int>(node_labels.size()); ++i) {
        int id = scc.scc_id[i];
        if (id >= 0 && id < scc.num_sccs) {
            result[id].push_back(node_labels[i]);
        }
    }
    return result;
}

std::vector<std::string>
GraphAlgorithms::DomainSCCResult::topo_order_labels(
    const std::vector<int>& topo_order) const
{
    // Return the representative label (first domain) for each SCC super-node
    // in topological order.
    auto domain_sccs = scc_domains();
    std::vector<std::string> labels;
    labels.reserve(topo_order.size());
    for (int id : topo_order) {
        if (id >= 0 && id < static_cast<int>(domain_sccs.size())
            && !domain_sccs[id].empty()) {
            labels.push_back(domain_sccs[id][0]);
        }
    }
    return labels;
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. run_scc_on_domain_graph  —  string-keyed wrapper
//
// Converts the domain string graph to integer ids, runs Tarjan, then
// stores the label mapping for human-readable output.
// ─────────────────────────────────────────────────────────────────────────────

GraphAlgorithms::DomainSCCResult GraphAlgorithms::run_scc_on_domain_graph(
    const std::unordered_map<std::string, std::vector<std::string>>& graph)
{
    DomainSCCResult result;

    if (graph.empty()) return result;

    // ── Build integer mapping ────────────────────────────────────────────────
    // Collect all nodes (sources + all destinations)
    for (const auto& [src, dsts] : graph) {
        if (result.node_index.find(src) == result.node_index.end()) {
            result.node_index[src] = static_cast<int>(result.node_labels.size());
            result.node_labels.push_back(src);
        }
        for (const auto& dst : dsts) {
            if (result.node_index.find(dst) == result.node_index.end()) {
                result.node_index[dst] = static_cast<int>(result.node_labels.size());
                result.node_labels.push_back(dst);
            }
        }
    }

    int N = static_cast<int>(result.node_labels.size());
    std::vector<std::vector<int>> adj(N);

    for (const auto& [src, dsts] : graph) {
        int u = result.node_index.at(src);
        for (const auto& dst : dsts) {
            int v = result.node_index.at(dst);
            adj[u].push_back(v);
        }
    }

    // ── Run Tarjan ───────────────────────────────────────────────────────────
    result.scc = tarjan_scc(adj, N);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. run_topo_on_scc  —  full pipeline: Tarjan → condense → topo sort
// ─────────────────────────────────────────────────────────────────────────────

GraphAlgorithms::TopoResult GraphAlgorithms::run_topo_on_scc(
    const DomainSCCResult& domain_scc,
    const std::unordered_map<std::string, std::vector<std::string>>& graph)
{
    TopoResult tr;

    // Rebuild integer adj for condensation (needed by build_condensed_dag)
    int N = static_cast<int>(domain_scc.node_labels.size());
    std::vector<std::vector<int>> adj(N);

    for (const auto& [src, dsts] : graph) {
        auto it_u = domain_scc.node_index.find(src);
        if (it_u == domain_scc.node_index.end()) continue;
        int u = it_u->second;
        for (const auto& dst : dsts) {
            auto it_v = domain_scc.node_index.find(dst);
            if (it_v == domain_scc.node_index.end()) continue;
            adj[u].push_back(it_v->second);
        }
    }

    tr.dag   = build_condensed_dag(adj, domain_scc.scc);
    tr.order = topological_sort(tr.dag);
    return tr;
}
