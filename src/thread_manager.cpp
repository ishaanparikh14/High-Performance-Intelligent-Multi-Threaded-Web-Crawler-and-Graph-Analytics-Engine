#include "thread_manager.h"
#include "url_scorer.h"
#include "DumpUtils.h"
#include <iostream>
#include <chrono>
#include <thread>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: human-readable label for the current traversal mode
// ─────────────────────────────────────────────────────────────────────────────
static const char* mode_label(TraversalMode m) {
    switch (m) {
        case TraversalMode::BFS:      return "BFS  (breadth-first, FIFO)";
        case TraversalMode::DFS:      return "DFS  (depth-first,   LIFO)";
        case TraversalMode::PRIORITY: return "PRIORITY (priority queue, depth-weighted)";
    }
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// start()
// ─────────────────────────────────────────────────────────────────────────────
void ThreadManager::start(int num_threads, int max_pages,
                          const std::string& seed_url,
                          StorageManager& storage_manager) {
    max_pages_limit.store(max_pages);

    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         MULTITHREADED WEB CRAWLER                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n[CONFIG]" << std::endl;
    std::cout << "  Seed URL:     " << seed_url                        << std::endl;
    std::cout << "  Max Pages:    " << max_pages                       << std::endl;
    std::cout << "  Threads:      " << num_threads                     << std::endl;
    std::cout << "  Traversal:    " << mode_label(frontier.get_mode()) << std::endl;
    std::cout << "\n[STARTING CRAWL]" << std::endl;

    frontier.init(seed_url);

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(&ThreadManager::worker_loop, this, i,
                             std::ref(storage_manager));
    }

    // Progress reporter (detached, auto-terminates when crawl finishes)
    std::thread progress_thread([this, num_threads]() {
        while (pages_crawled.load() < max_pages_limit.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            std::cout << "[PROGRESS] Pages: " << pages_crawled.load()
                      << "/" << max_pages_limit.load()
                      << " | Queue: "   << frontier.queue_size()
                      << " | Visited: " << frontier.visited_count()
                      << " | Mode: "    << mode_label(frontier.get_mode())
                      << std::endl;

            if (frontier.queue_size() == 0 && pages_crawled.load() > 0) {
                break;
            }
        }
    });
    progress_thread.detach();
}

// ─────────────────────────────────────────────────────────────────────────────
// worker_loop()
// ─────────────────────────────────────────────────────────────────────────────
void ThreadManager::worker_loop(int thread_id, StorageManager& storage_manager) {
    Downloader downloader;
    Parser     parser;

    // Thread-local scorer — avoids shared state in PRIORITY mode
    URLScorer scorer = make_default_scorer();

    std::string url;
    int         depth      = 0;
    int         backoff_ms = 10;

    while (pages_crawled.load() < max_pages_limit.load()) {

        if (frontier.try_dequeue(url, depth)) {
            backoff_ms = 10;  // reset exponential back-off

            std::cout << "[T" << thread_id << "][depth=" << depth << "] "
                      << "Downloading: " << url << std::endl;

            // ── Download ────────────────────────────────────────────────────
            std::string html = downloader.download(url);
            if (html.empty()) {
                std::cout << "[T" << thread_id << "] ✗ Failed: " << url << std::endl;
                continue;
            }

            std::string domain = downloader.get_domain(url);
            std::cout << "[T" << thread_id << "] ✓ " << html.size()
                      << " bytes  domain=" << domain << std::endl;

            // ── Parse links ─────────────────────────────────────────────────
            std::vector<std::string> links = parser.extract_links(html, url);
            std::cout << "[T" << thread_id << "] Found " << links.size()
                      << " links" << std::endl;

            // ── Store ───────────────────────────────────────────────────────
            storage_manager.add_page(thread_id, domain, links);

            // ── Enqueue discovered URLs ──────────────────────────────────────
            // PRIORITY mode: use URLScorer heuristics (trusted domains, keywords,
            //   depth, URL length, suffix reputation, seed proximity).
            // BFS/DFS: priority is ignored by the frontier; use 1.0.
            if (frontier.get_mode() == TraversalMode::PRIORITY) {
                for (const auto& link : links) {
                    double p = scorer.score(link, depth + 1);
                    frontier.batch_enqueue({link}, depth + 1, p);
                }
            } else {
                int new_urls = frontier.batch_enqueue(links, depth + 1, 1.0);
                if (new_urls > 0) {
                    std::cout << "[T" << thread_id << "] Enqueued " << new_urls
                              << " new URLs (depth " << depth + 1 << ")" << std::endl;
                }
            }

            pages_crawled.fetch_add(1);

        } else {
            // Frontier empty – exponential back-off while waiting for other
            // threads to enqueue more URLs
            if (frontier.queue_size() == 0) {
                if (backoff_ms < 500) backoff_ms *= 2;
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
            }
        }
    }

    std::cout << "[T" << thread_id << "] Thread finished" << std::endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// wait_completion()
// ─────────────────────────────────────────────────────────────────────────────
void ThreadManager::wait_completion() {
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

#if DEBUG_DUMP
    DumpUtils::dump_frontier(frontier.copy_queue());
    DumpUtils::dump_visited(frontier.copy_visited());
#endif

    frontier.mark_done();
    std::cout << "\n[CRAWL COMPLETE]" << std::endl;
    std::cout << "Total pages crawled: " << pages_crawled.load() << std::endl;
}

void ThreadManager::dump_frontier_and_visited() {
#if DEBUG_DUMP
    auto q = frontier.get_queue_snapshot();
    auto v = frontier.get_visited_snapshot();
    DumpUtils::dump_frontier(q);
    DumpUtils::dump_visited(v);
#endif
}

int ThreadManager::get_pages_crawled() const {
    return pages_crawled.load();
}
