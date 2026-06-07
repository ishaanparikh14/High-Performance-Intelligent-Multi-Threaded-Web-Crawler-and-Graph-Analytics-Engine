# 🚀 High-Performance Intelligent Multi-Threaded Web Crawler and Graph Analytics Engine

A high-performance C++ web crawling and graph analytics platform featuring multi-threaded crawling, BFS, DFS, and Priority Queue traversal strategies, PageRank computation, Tarjan SCC detection, Topological Sorting, KMP String Matching, REST API integration, and an interactive analytics dashboard for real-time visualization and performance benchmarking.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Multi-Threaded](https://img.shields.io/badge/Concurrency-MultiThreaded-green)
![Algorithms](https://img.shields.io/badge/Algorithms-DAA-orange)
![Graph Analytics](https://img.shields.io/badge/Graph-Analytics-purple)

---

## ✨ Highlights

* Multi-threaded web crawling engine
* BFS, DFS, and Priority Queue crawling strategies
* PageRank implementation
* Tarjan Strongly Connected Components (SCC)
* Topological Sort visualization
* KMP String Matching
* Interactive Graph Analytics Dashboard
* REST API backend
* Thread-local buffering optimization
* Real-time performance benchmarking
* Scalable graph construction and analysis

---

## 📸 Dashboard Preview

### Graph Analytics Dashboard

![Graph Analytics](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.03.08.jpeg)

### Algorithm Benchmark Comparison

![Benchmarks](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.03.23.jpeg)

### Tarjan SCC Community Graph

![Tarjan SCC](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.03.52.jpeg)

### SCC Communities Visualization

![SCC Communities](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.04.10.jpeg)

### PageRank Analysis Dashboard

![PageRank](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.04.30.jpeg)

### Topological Sort Visualization

![Topology](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.04.43.jpeg)

### Real-Time Queue Monitoring

![Queue Monitoring](assets/screenshots/WhatsApp%20Image%202026-06-05%20at%2000.05.00.jpeg)

---

## 🔥 Features

### Multi-Threaded Crawling

* Parallel URL fetching using worker threads
* Thread-local buffering to reduce synchronization overhead
* High throughput with near-linear scalability
* Deadlock-free and thread-safe architecture

### Intelligent Traversal Strategies

* Breadth First Search (BFS)
* Depth First Search (DFS)
* Priority Queue Based Crawling

### Graph Analytics

* Web Graph Construction
* PageRank Computation
* Tarjan Strongly Connected Components (SCC)
* Topological Sorting
* Domain Importance Ranking

### String Matching & Search

* KMP (Knuth-Morris-Pratt) String Matching
* Fast keyword detection in crawled pages
* Efficient pattern searching across datasets

### Data Processing

* Automatic URL extraction
* Duplicate URL detection
* Domain frequency analysis
* Link relationship mapping

### Visualization & Analytics

* Interactive Dashboard
* Graph Visualization
* Crawl Statistics
* Performance Metrics
* Ranking Reports
* SCC Community Analysis

### API Support

* REST API Integration
* CSV Export Support
* Real-Time Data Access

---

## 🏗️ System Architecture

The platform follows a MapReduce-inspired workflow for scalable crawling and analysis.

### Phase 1: Crawl (Map)

* Multiple worker threads fetch pages concurrently
* URLs processed using BFS, DFS, or Priority Scheduling
* Extracted links stored in thread-local buffers

### Phase 2: Merge (Shuffle)

* Thread-local results aggregated
* Unified web graph constructed
* Duplicate URLs eliminated

### Phase 3: Analyze (Reduce)

* PageRank computation
* Tarjan SCC detection
* Topological Sorting
* KMP keyword matching
* Analytics generation and visualization

---

## 🧠 Algorithms Used

| Category         | Algorithm              |
| ---------------- | ---------------------- |
| Traversal        | BFS                    |
| Traversal        | DFS                    |
| Scheduling       | Priority Queue         |
| Graph Analysis   | PageRank               |
| Graph Analysis   | Tarjan SCC             |
| Graph Analysis   | Topological Sort       |
| Pattern Matching | KMP String Matching    |
| Concurrency      | Multi-threading        |
| Optimization     | Thread-Local Buffering |

---

## 📊 Performance Benchmarks

| Strategy       | Avg Time (ms) | Throughput (pages/sec) |
| -------------- | ------------- | ---------------------- |
| BFS            | 15008         | 5.56                   |
| Priority Queue | 20988         | 3.16                   |
| DFS            | 28533         | 1.89                   |

### 🏆 Recommended Strategy

**BFS** achieved the highest throughput and lowest execution time, making it the preferred crawling strategy for large-scale web graph exploration.

---

## ⚙️ Installation & Setup

### Prerequisites

* C++17 or later
* GCC / G++
* Git
* CMake
* libcurl
* nlohmann/json

### Clone Repository

```bash
git clone https://github.com/ishaanparikh14/High-Performance-Intelligent-Multi-Threaded-Web-Crawler-and-Graph-Analytics-Engine.git

cd High-Performance-Intelligent-Multi-Threaded-Web-Crawler-and-Graph-Analytics-Engine
```

### Install Dependencies (Ubuntu)

```bash
sudo apt update

sudo apt install -y \
g++ \
cmake \
libcurl4-openssl-dev \
nlohmann-json3-dev
```

### Build Project

```bash
chmod +x build.sh

./build.sh
```

### Run Crawler

```bash
./crawler bfs
```

```bash
./crawler dfs
```

```bash
./crawler priority
```

### Launch Dashboard

```bash
cd dashboard

python3 -m http.server 3000
```

Open:

```text
http://localhost:3000
```

---

## 📁 Project Structure

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
│   └── kmp.cpp
│
├── api/
│
├── dashboard/
│
├── assets/
│   └── screenshots/
│
├── output/
│
└── README.md
```
---

## ⚡ Key Optimizations

### Thread-Local Buffering

Each worker thread maintains its own private buffer before synchronization, significantly reducing lock contention.

### Minimal Locking Strategy

Mutexes are used only for critical shared resources, minimizing synchronization overhead.

### Efficient Graph Representation

The crawler stores relationships as graph edges rather than adjacency matrices, improving scalability and memory efficiency.

---

## 📈 Sample Outputs

### Crawling Outputs

* Crawled Pages
* URL Frontier Dumps
* Thread Buffers
* Domain Statistics

### Graph Analytics

* PageRank Scores
* SCC Communities
* Topological Order
* Graph Metrics

### Benchmark Reports

* Execution Time
* Throughput
* Strategy Comparison

---

## 🔮 Future Enhancements

* Bloom Filter duplicate detection
* Distributed crawling architecture
* AI-powered URL prioritization
* Incremental PageRank
* Real-time graph streaming
* Distributed graph analytics

---

## 📜 License

This project is intended for educational and research purposes.


---


