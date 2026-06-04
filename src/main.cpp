// ─────────────────────────────────────────────────────────────────────────────
// main.cpp — Multi-Threaded Web Crawler
//
// Flow:
//   1. Parse CLI args
//   2. Initialize StorageManager (thread-local buffers)
//   3. Start ThreadManager (worker pool + Best-First/BFS/DFS crawling)
//   4. Merge buffers into shared graph
//   5. Compute PageRank  O(k × (V + E))
//   6. Run Tarjan SCC     O(V + E)
//   7. Build condensed DAG + Kahn's Topological Sort  O(V + E)
//   8. Export CSVs: crawled_pages, pagerank_results, scc_results,
//                   topo_order, metrics
// ─────────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <vector>

#include "thread_manager.h"
#include "storage_manager.h"
#include "graph_algorithms.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void print_usage(const char* prog) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n"
              << "║       Multi-Threaded Web Crawler  (DAA Project)          ║\n"
              << "╚═══════════════════════════════════════════════════════════╝\n"
              << "\nUsage: " << prog
              << " <seed_url> <max_pages> <num_threads> [mode]\n"
              << "\nArguments:\n"
              << "  seed_url     Starting URL (http:// or https://)\n"
              << "  max_pages    Maximum pages to crawl (> 0)\n"
              << "  num_threads  Worker thread count (1 – 64)\n"
              << "  mode         bfs | dfs | priority  (default: bfs)\n"
              << "\nExamples:\n"
              << "  " << prog << " https://example.com 100 4\n"
              << "  " << prog << " https://example.com 100 4 priority\n"
              << "\nOutput files:\n"
              << "  crawled_pages.csv     domain, outgoing_links, visit_count\n"
              << "  pagerank_results.csv  domain, pagerank_score\n"
              << "  scc_results.csv       scc_id, size, domains\n"
              << "  topo_order.csv        topo_rank, scc_id, representative_domain\n"
              << "  metrics.csv           timing and throughput per run\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// export_scc_csv()
// ─────────────────────────────────────────────────────────────────────────────
static void export_scc_csv(
    const GraphAlgorithms::DomainSCCResult& dr,
    const std::string& filename)
{
    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cerr << "[ERROR] Cannot open " << filename << "\n";
        return;
    }
    f << "scc_id,size,domains\n";

    auto domain_sccs = dr.scc_domains();
    for (int i = 0; i < static_cast<int>(domain_sccs.size()); ++i) {
        const auto& members = domain_sccs[i];
        f << i << "," << members.size() << ",\"";
        for (size_t j = 0; j < members.size(); ++j) {
            if (j > 0) f << "|";
            f << members[j];
        }
        f << "\"\n";
    }
    f.close();
    std::cout << "[INFO] SCC results exported to: " << filename << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// export_topo_csv()
// ─────────────────────────────────────────────────────────────────────────────
static void export_topo_csv(
    const GraphAlgorithms::DomainSCCResult& dr,
    const std::vector<int>& topo_order,
    const std::string& filename)
{
    std::ofstream f(filename);
    if (!f.is_open()) {
        std::cerr << "[ERROR] Cannot open " << filename << "\n";
        return;
    }
    f << "topo_rank,scc_id,size,representative_domain\n";

    auto domain_sccs = dr.scc_domains();
    for (int rank = 0; rank < static_cast<int>(topo_order.size()); ++rank) {
        int scc_id = topo_order[rank];
        const auto& members = (scc_id < static_cast<int>(domain_sccs.size()))
                              ? domain_sccs[scc_id]
                              : std::vector<std::string>{};
        std::string rep = members.empty() ? "(empty)" : members[0];
        f << rank << "," << scc_id << "," << members.size() << "," << rep << "\n";
    }
    f.close();
    std::cout << "[INFO] Topological order exported to: " << filename << "\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main()
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    std::string seed_url        = argv[1];
    std::string max_pages_str   = argv[2];
    std::string num_threads_str = argv[3];
    std::string mode_str        = (argc == 5) ? argv[4] : "bfs";

    // ── Validate inputs ──────────────────────────────────────────────────────
    if (seed_url.find("http://") != 0 && seed_url.find("https://") != 0) {
        std::cerr << "[ERROR] Seed URL must start with http:// or https://\n";
        return 1;
    }

    int max_pages   = 0;
    int num_threads = 0;
    try {
        max_pages   = std::stoi(max_pages_str);
        num_threads = std::stoi(num_threads_str);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Invalid arguments: " << e.what() << "\n";
        return 1;
    }

    if (max_pages <= 0) {
        std::cerr << "[ERROR] max_pages must be positive\n";
        return 1;
    }
    if (num_threads <= 0 || num_threads > 64) {
        std::cerr << "[ERROR] num_threads must be between 1 and 64\n";
        return 1;
    }

    TraversalMode mode = TraversalMode::BFS;
    if      (mode_str == "dfs")      mode = TraversalMode::DFS;
    else if (mode_str == "priority") mode = TraversalMode::PRIORITY;
    else if (mode_str != "bfs") {
        std::cerr << "[ERROR] Unknown mode '" << mode_str
                  << "'. Valid options: bfs | dfs | priority\n";
        return 1;
    }

    // ── Phase 1: Crawl ───────────────────────────────────────────────────────
    StorageManager storage;
    storage.init(num_threads);

    std::cout << "\n[TIMING] Starting crawl...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    ThreadManager crawler(mode);
    crawler.start(num_threads, max_pages, seed_url, storage);
    crawler.wait_completion();

#if DEBUG_DUMP
    crawler.dump_frontier_and_visited();
#endif

    auto t1       = std::chrono::high_resolution_clock::now();
    auto crawl_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "[TIMING] Crawl completed in " << crawl_ms << " ms\n";

    // ── Phase 2: Merge ───────────────────────────────────────────────────────
    std::cout << "\n[TIMING] Merging thread buffers...\n";
    auto tm0 = std::chrono::high_resolution_clock::now();
    storage.merge_all_buffers();
    auto tm1      = std::chrono::high_resolution_clock::now();
    auto merge_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tm1 - tm0).count();
    std::cout << "[TIMING] Merge completed in " << merge_ms << " ms\n";

    // ── Phase 3: PageRank  O(k × (V + E)) ───────────────────────────────────
    std::cout << "\n[TIMING] Computing PageRank...\n";
    auto tp0 = std::chrono::high_resolution_clock::now();
    storage.compute_pagerank(30);
    auto tp1         = std::chrono::high_resolution_clock::now();
    auto pagerank_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp1 - tp0).count();
    std::cout << "[TIMING] PageRank completed in " << pagerank_ms << " ms\n";

    // ── Phase 4: Graph algorithms (SCC + Topo Sort) ──────────────────────────
    // get_link_graph() returns the fully merged domain→[neighbours] map.
    // This is the input for Tarjan SCC  O(V + E).
    std::cout << "\n[TIMING] Running Tarjan SCC  O(V + E)...\n";
    auto ts0 = std::chrono::high_resolution_clock::now();

    // Run SCC on domain graph (uses the full merged graph via new getter)
    auto domain_scc = GraphAlgorithms::run_scc_on_domain_graph(
        storage.get_link_graph());

    auto ts1    = std::chrono::high_resolution_clock::now();
    auto scc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ts1 - ts0).count();
    std::cout << "[TIMING] Tarjan SCC completed in " << scc_ms << " ms\n";

    std::cout << "[INFO] SCC Analysis:\n"
              << "  Total nodes:   " << domain_scc.node_labels.size() << "\n"
              << "  Total SCCs:    " << domain_scc.scc.num_sccs << "\n"
              << "  Largest SCC:   " << domain_scc.scc.max_scc_size << " nodes"
              << "  (SCC #" << domain_scc.scc.max_scc_index << ")\n";

    std::cout << "\n[TIMING] Building condensed DAG + Topological Sort  O(V + E)...\n";
    auto tt0 = std::chrono::high_resolution_clock::now();

    auto topo_result = GraphAlgorithms::run_topo_on_scc(
        domain_scc, storage.get_link_graph());

    auto tt1    = std::chrono::high_resolution_clock::now();
    auto topo_ms = std::chrono::duration_cast<std::chrono::milliseconds>(tt1 - tt0).count();
    std::cout << "[TIMING] Topological Sort completed in " << topo_ms << " ms\n";
    std::cout << "[INFO] Condensed DAG: " << topo_result.dag.num_nodes
              << " super-nodes, topo order size = " << topo_result.order.size() << "\n";

    // ── Phase 5: Export ──────────────────────────────────────────────────────
    storage.export_to_csv("crawled_pages.csv", "pagerank_results.csv");
    export_scc_csv(domain_scc, "scc_results.csv");
    export_topo_csv(domain_scc, topo_result.order, "topo_order.csv");

    // ── Phase 6: Metrics ─────────────────────────────────────────────────────
    int    pages_crawled = crawler.get_pages_crawled();
    double throughput    = (crawl_ms > 0) ? pages_crawled * 1000.0 / crawl_ms : 0.0;

    // Check if metrics.csv already exists and has content (for header decision)
    bool metrics_exists = false;
    {
        std::ifstream check("metrics.csv");
        if (check.good()) {
            std::string line;
            metrics_exists = static_cast<bool>(std::getline(check, line));
        }
    }

    std::ofstream metrics("metrics.csv", std::ios::app);
    if (metrics.is_open()) {
        if (!metrics_exists) {
            metrics << "seed_url,max_pages,num_threads,mode,crawl_ms,"
                       "merge_ms,pagerank_ms,scc_ms,topo_ms,"
                       "pages_crawled,nodes,sccs,throughput\n";
        }
        metrics << seed_url << ","
                << max_pages << ","
                << num_threads << ","
                << mode_str << ","
                << crawl_ms << ","
                << merge_ms << ","
                << pagerank_ms << ","
                << scc_ms << ","
                << topo_ms << ","
                << pages_crawled << ","
                << domain_scc.node_labels.size() << ","
                << domain_scc.scc.num_sccs << ","
                << std::fixed << std::setprecision(2) << throughput << "\n";
        metrics.close();
        std::cout << "[INFO] Metrics appended to: metrics.csv\n";
    }

    // ── Summary ──────────────────────────────────────────────────────────────
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n"
              << "║                  CRAWL COMPLETE                          ║\n"
              << "╚═══════════════════════════════════════════════════════════╝\n"
              << "\n[RESULTS]\n"
              << "  Mode:            " << mode_str << "\n"
              << "  Pages crawled:   " << pages_crawled << "\n"
              << "  Graph nodes:     " << domain_scc.node_labels.size() << "\n"
              << "  SCCs found:      " << domain_scc.scc.num_sccs << "\n"
              << "  Largest SCC:     " << domain_scc.scc.max_scc_size << " nodes\n"
              << "  Topo DAG nodes:  " << topo_result.dag.num_nodes << "\n"
              << "  Crawl time:      " << crawl_ms << " ms\n"
              << "  Throughput:      " << std::fixed << std::setprecision(2)
              << throughput << " pages/s\n"
              << "\n[CSV OUTPUT]\n"
              << "  crawled_pages.csv\n"
              << "  pagerank_results.csv\n"
              << "  scc_results.csv\n"
              << "  topo_order.csv\n"
              << "  metrics.csv\n\n";

    return 0;
}
