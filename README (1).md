# High-Performance-Intelligent-Multi-Threaded-Web-Crawler-and-Graph-Analytics-Engine
High-performance C++ multi-threaded web crawler with BFS, DFS, and priority-based crawling. Features PageRank, Tarjan SCC, topological sorting, KMP string matching, REST API integration, real-time analytics dashboard, graph visualization, thread-local optimization, and scalable web graph analysis.

# Intelligent Multi-Threaded Web Crawler and Graph Analytics Engine

A high-performance, scalable web crawling and graph analytics platform built in C++. The system leverages multi-threading, advanced graph algorithms, intelligent URL scheduling, and string matching techniques to efficiently crawl, analyze, and visualize web data.

## Features

### Multi-Threaded Crawling
- Parallel URL fetching using worker threads
- Thread-local buffering to reduce synchronization overhead
- High throughput with near-linear scalability
- Deadlock-free and thread-safe architecture

### Intelligent Traversal Strategies
- Breadth First Search (BFS)
- Depth First Search (DFS)
- Priority Queue Based Crawling

### Graph Analytics
- Web Graph Construction
- PageRank Computation
- Tarjan's Strongly Connected Components (SCC)
- Topological Sorting
- Domain Importance Ranking

### String Matching & Search
- KMP (Knuth-Morris-Pratt) String Matching
- Fast keyword detection in crawled pages
- Efficient pattern searching across datasets

### Data Processing
- Automatic URL extraction
- Duplicate URL detection
- Domain frequency analysis
- Link relationship mapping

### Visualization & Analytics
- Interactive Dashboard
- Graph Visualization
- Crawl Statistics
- Performance Metrics
- Ranking Reports

### API Support
- REST API Integration
- CSV Export Support
- Real-Time Data Access

---
## System Architecture

The system follows a MapReduce-inspired architecture:

### Phase 1: Crawl (Map)
- Multiple worker threads fetch web pages concurrently
- URLs are processed using BFS, DFS, or Priority Scheduling
- Extracted links are stored in thread-local buffers

### Phase 2: Merge (Shuffle)
- Thread-local results are aggregated
- Unified web graph is constructed
- Duplicate data is removed

### Phase 3: Analyze (Reduce)
- PageRank computation
- SCC detection using Tarjan's Algorithm
- Topological Sorting
- Keyword matching using KMP
- Analytics generation

---

## Algorithms Used

| Category | Algorithm |
|-----------|------------|
| Traversal | BFS |
| Traversal | DFS |
| Scheduling | Priority Queue |
| Graph Analysis | Tarjan SCC |
| Graph Analysis | PageRank |
| Graph Analysis | Topological Sort |
| Pattern Matching | KMP String Matching |
| Concurrency | Multi-threading |
| Optimization | Thread-Local Buffering |

---

## Tech Stack

- C++
- STL
- Multi-threading
- REST APIs
- CSV Processing
- Graph Data Structures
- HTML Parsing

---

## Project Structure

```text
project/
│
├── crawler/
│   ├── bfs_crawler.cpp
│   ├── dfs_crawler.cpp
│   ├── priority_crawler.cpp
│
├── graph/
│   ├── pagerank.cpp
│   ├── tarjan_scc.cpp
│   ├── topological_sort.cpp
│
├── search/
│   ├── kmp.cpp
│
├── api/
│   ├── server.cpp
│
├── dashboard/
│
├── outputs/
│   ├── crawled_pages.csv
│   ├── pagerank.csv
│
└── README.md
```

---

## Key Optimizations

### Thread-Local Buffering
Instead of writing directly to shared data structures, each worker thread maintains a private buffer. This significantly reduces lock contention and improves parallel efficiency.

### Minimal Locking Strategy
Only critical sections use mutex protection, minimizing synchronization overhead while maintaining correctness.

### Efficient Graph Representation
The crawler stores relationships as edge-based graphs rather than adjacency matrices, reducing memory consumption and improving PageRank performance.

---

## Performance Highlights

- Near-linear speedup with increasing thread count
- Reduced synchronization overhead
- Efficient large-scale graph processing
- Fast keyword search using KMP
- Scalable architecture suitable for future distributed deployment

---

## Sample Outputs

### Crawled Pages
- Domain
- URL
- Outgoing Links
- Visit Frequency

### PageRank Results
- Domain Rank
- Importance Score
- Connectivity Metrics

### Graph Analytics
- SCC Groups
- Topological Order
- Graph Statistics

---

## Future Enhancements

- Distributed Crawling
- Bloom Filter Based Duplicate Detection
- AI-Powered URL Prioritization
- Incremental PageRank
- Real-Time Streaming Analytics
- Distributed Graph Processing


## License

This project is intended for educational and research purposes.
