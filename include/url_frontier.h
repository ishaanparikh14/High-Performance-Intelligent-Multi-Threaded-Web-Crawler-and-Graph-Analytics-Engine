#ifndef URL_FRONTIER_H
#define URL_FRONTIER_H

#include <string>
#include <queue>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <mutex>

// ─────────────────────────────────────────────────────────────────────────────
// Traversal mode: selects BFS (FIFO queue) or DFS (LIFO stack) or Priority
// ─────────────────────────────────────────────────────────────────────────────
enum class TraversalMode {
    BFS,       // Breadth-First Search  (default – FIFO, level-by-level)
    DFS,       // Depth-First Search    (LIFO – follows one path deep)
    PRIORITY   // Priority Queue        (highest-score URL next)
};

// Entry stored in the priority queue
struct URLEntry {
    std::string url;
    double      priority;   // higher = dequeued first
    int         depth;      // hop distance from seed

    // Max-heap: larger priority comes first
    bool operator<(const URLEntry& o) const {
        return priority < o.priority;   // intentionally reversed for max-heap
    }
};

/**
 * URL frontier supporting BFS, DFS, and Priority-Queue traversal.
 *
 * Traversal mode is selected at construction time and is immutable.
 *
 * Thread-safety: a single mutex guards all queue/stack/pq operations.
 * The visited set is also protected by the same lock.
 */
class URLFrontier {
public:
    explicit URLFrontier(TraversalMode mode = TraversalMode::BFS)
        : mode_(mode) {}

    /**
     * Initialize frontier with seed URL.
     */
    void init(const std::string& seed_url);

    /**
     * Try to dequeue next URL to crawl.
     * @param url    Output parameter for the URL.
     * @param depth  Output parameter for the URL's hop depth.
     * @return true if a URL was dequeued, false if the frontier is empty.
     */
    bool try_dequeue(std::string& url, int& depth);

    /**
     * Add URL if not yet visited.
     * @param url      URL to add.
     * @param depth    Hop depth of this URL.
     * @param priority Score used by PRIORITY mode (higher = sooner).
     * @return true if added, false if already visited.
     */
    bool add_if_not_visited(const std::string& url,
                            int    depth    = 0,
                            double priority = 1.0);

    /**
     * Batch-enqueue multiple URLs (called from the parser).
     * @param urls      Vector of URLs to enqueue.
     * @param depth     Hop depth for all URLs in this batch.
     * @param priority  Priority score (used only in PRIORITY mode).
     * @return Number of URLs actually added.
     */
    int batch_enqueue(const std::vector<std::string>& urls,
                      int    depth    = 0,
                      double priority = 1.0);

    bool   has_work()      const;
    size_t queue_size()    const;
    size_t visited_count() const;
    void   mark_done();

    TraversalMode get_mode() const { return mode_; }

    // ── Snapshot helpers (safe after crawl completes) ────────────────────────
    std::queue<std::string>          copy_queue()   const;
    std::unordered_set<std::string>  copy_visited() const;
    std::queue<std::string>          get_queue_snapshot();
    std::unordered_set<std::string>  get_visited_snapshot();

private:
    TraversalMode mode_;

    // BFS storage
    std::queue<URLEntry>                   bfs_queue_;

    // DFS storage
    std::vector<URLEntry>                  dfs_stack_;

    // Priority-queue storage (max-heap)
    std::priority_queue<URLEntry>          pq_;

    std::unordered_set<std::string> visited_;
    std::atomic<bool>               is_done_{false};
    std::atomic<size_t>             queue_size_{0};
    mutable std::mutex              queue_mutex_;

    // Internal helpers (called with lock held)
    void   push_locked(const URLEntry& entry);
    bool   pop_locked(URLEntry& entry);
    size_t internal_size() const;
};

#endif // URL_FRONTIER_H
