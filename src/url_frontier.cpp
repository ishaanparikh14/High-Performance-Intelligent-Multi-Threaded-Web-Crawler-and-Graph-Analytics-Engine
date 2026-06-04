#include "url_frontier.h"
#include <chrono>
#include <thread>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers  (always called with queue_mutex_ held)
// ─────────────────────────────────────────────────────────────────────────────

void URLFrontier::push_locked(const URLEntry& entry) {
    switch (mode_) {
        case TraversalMode::BFS:
            bfs_queue_.push(entry);
            break;
        case TraversalMode::DFS:
            dfs_stack_.push_back(entry);
            break;
        case TraversalMode::PRIORITY:
            pq_.push(entry);
            break;
    }
    queue_size_.store(internal_size());
}

bool URLFrontier::pop_locked(URLEntry& entry) {
    switch (mode_) {
        case TraversalMode::BFS:
            if (bfs_queue_.empty()) return false;
            entry = bfs_queue_.front();
            bfs_queue_.pop();
            break;
        case TraversalMode::DFS:
            if (dfs_stack_.empty()) return false;
            entry = dfs_stack_.back();
            dfs_stack_.pop_back();
            break;
        case TraversalMode::PRIORITY:
            if (pq_.empty()) return false;
            entry = pq_.top();
            pq_.pop();
            break;
    }
    queue_size_.store(internal_size());
    return true;
}

size_t URLFrontier::internal_size() const {
    switch (mode_) {
        case TraversalMode::BFS:      return bfs_queue_.size();
        case TraversalMode::DFS:      return dfs_stack_.size();
        case TraversalMode::PRIORITY: return pq_.size();
    }
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void URLFrontier::init(const std::string& seed_url) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    visited_.insert(seed_url);
    push_locked({seed_url, 1.0, 0});
    is_done_.store(false);
}

bool URLFrontier::try_dequeue(std::string& url, int& depth) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    URLEntry entry;
    if (!pop_locked(entry)) return false;
    url   = entry.url;
    depth = entry.depth;
    return true;
}

bool URLFrontier::add_if_not_visited(const std::string& url,
                                     int depth, double priority) {
    if (url.empty() || url.length() > 10000) return false;

    std::lock_guard<std::mutex> lock(queue_mutex_);

    if (visited_.count(url)) return false;

    visited_.insert(url);
    push_locked({url, priority, depth});
    return true;
}

int URLFrontier::batch_enqueue(const std::vector<std::string>& urls,
                                int depth, double priority) {
    // Acquire the lock ONCE for the entire batch to reduce contention
    std::lock_guard<std::mutex> lock(queue_mutex_);
    int added = 0;
    for (const auto& url : urls) {
        if (url.empty() || url.length() > 10000) continue;
        if (visited_.count(url)) continue;
        visited_.insert(url);
        push_locked({url, priority, depth});
        ++added;
    }
    return added;
}

bool URLFrontier::has_work() const {
    return (queue_size_.load() > 0) && !is_done_.load();
}

size_t URLFrontier::queue_size() const {
    return queue_size_.load();
}

size_t URLFrontier::visited_count() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return visited_.size();
}

void URLFrontier::mark_done() {
    is_done_.store(true);
}

// ── Snapshot helpers ──────────────────────────────────────────────────────────

std::queue<std::string> URLFrontier::copy_queue() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    std::queue<std::string> result;

    switch (mode_) {
        case TraversalMode::BFS: {
            std::queue<URLEntry> tmp = bfs_queue_;
            while (!tmp.empty()) {
                result.push(tmp.front().url);
                tmp.pop();
            }
            break;
        }
        case TraversalMode::DFS: {
            // Snapshot DFS stack (top = back of vector)
            for (auto it = dfs_stack_.rbegin(); it != dfs_stack_.rend(); ++it) {
                result.push(it->url);
            }
            break;
        }
        case TraversalMode::PRIORITY: {
            // Snapshot PQ: copy into a temp PQ then drain in priority order
            std::priority_queue<URLEntry> tmp = pq_;
            while (!tmp.empty()) {
                result.push(tmp.top().url);
                tmp.pop();
            }
            break;
        }
    }
    return result;
}

std::unordered_set<std::string> URLFrontier::copy_visited() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return visited_;
}

std::queue<std::string> URLFrontier::get_queue_snapshot() {
    return copy_queue();
}

std::unordered_set<std::string> URLFrontier::get_visited_snapshot() {
    return copy_visited();
}
