# Multi-Threaded Web Crawler with Graph Analytics

Production-grade web crawler featuring advanced graph algorithms, real-time monitoring, and interactive visualization dashboard.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C.svg)
![Python](https://img.shields.io/badge/Python-3.8%2B-3776AB.svg)

## Features

### Core Algorithms (All Optimized)
- **BFS Traversal** - O(V+E) breadth-first crawling
- **Priority Queue Crawling** - O(E log V) best-first search with heuristic scoring
- **PageRank** - O(k·E) power iteration (30 iterations, damping=0.85)
- **Tarjan SCC** - O(V+E) strongly connected components detection
- **Topological Sort** - O(V+E) Kahn's algorithm on condensed DAG

### Architecture
- **Multi-threaded C++ backend** with thread-local buffers
- **REST API** (Flask) for seamless frontend integration
- **Interactive dashboard** with D3.js force-directed graphs
- **Real-time progress monitoring** during crawls
- **Automatic CSV export** (crawled_pages, pagerank_results, scc_results, topo_order, metrics)

## Quick Start

### Prerequisites

**Linux/macOS:**
```bash
sudo apt install cmake build-essential libcurl4-openssl-dev python3 python3-pip
```

**Windows:**
- Install CMake, Visual Studio (C++ tools), and Python 3
- Or use WSL/Linux VM (recommended)

### Build & Run

```bash
# 1. Clone and build
git clone <your-repo-url>
cd wub
mkdir build && cd build
cmake ..
make  # or: cmake --build .

# 2. Start API server (Terminal 1)
cd ..
pip install -r requirements.txt
python api_server.py

# 3. Start dashboard (Terminal 2)
cd dashboard
python -m http.server 8080

# 4. Open browser
# Visit: http://localhost:8080
```

### Direct CLI Usage

```bash
./build/crawler <seed_url> <max_pages> <num_threads> [mode]

# Examples:
./build/crawler https://github.com 100 4 bfs
./build/crawler https://example.com 50 8 priority
```

**Arguments:**
- `seed_url` - Starting URL (http:// or https://)
- `max_pages` - Maximum pages to crawl (1-1000)
- `num_threads` - Worker threads (1-16)
- `mode` - Crawl strategy: `bfs` | `dfs` | `priority` (default: bfs)

## Architecture

```
User enters URL → Dashboard (Tailwind + D3.js)
                      ↓
                  REST API (Flask :5000)
                      ↓
            C++ Multi-Threaded Crawler
                      ↓
         CSV Files (output/ directory)
                      ↓
         Auto-loaded by API → Dashboard
```

### Data Flow
1. User submits crawl config via dashboard
2. Frontend sends `POST /api/crawl` to Flask API
3. API spawns C++ crawler subprocess
4. Crawler generates 5 CSV files in `output/`
5. API reads CSVs and serves JSON via endpoints
6. Dashboard polls `/api/status` for live progress
7. Results auto-load when crawl completes

## API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/crawl` | Start new crawl |
| `GET` | `/api/status` | Live crawl progress |
| `GET` | `/api/pagerank` | PageRank results |
| `GET` | `/api/scc` | SCC community detection |
| `GET` | `/api/topo` | Topological sort order |
| `GET` | `/api/metrics` | Performance metrics |
| `GET` | `/api/graph-data` | D3 visualization data |

## Output Files

All CSVs are written to `output/` directory:

- **crawled_pages.csv** - Domain, outgoing_links, visit_count
- **pagerank_results.csv** - Domain, pagerank_score
- **scc_results.csv** - SCC ID, size, member domains
- **topo_order.csv** - Topological rank, SCC ID, representative domain
- **metrics.csv** - Timing, throughput, algorithm performance

## Performance

Tested on 8-core machine with 50-page crawls:

| Threads | Crawl Time | Throughput | Speedup |
|---------|------------|------------|---------|
| 1       | 4820 ms    | 10.4 p/s   | 1.0x    |
| 2       | 2910 ms    | 17.2 p/s   | 1.66x   |
| 4       | 1730 ms    | 28.9 p/s   | 2.79x   |
| 8       | 1120 ms    | 44.6 p/s   | 4.30x   |

## Testing

```bash
# Test all API endpoints
python test_api.py

# Generate metrics report
python metrics_analyzer.py
```

## Project Structure

```
wub/
├── src/                    # C++ source files
│   ├── main.cpp           # Entry point
│   ├── downloader.cpp     # HTTP client
│   ├── parser.cpp         # HTML parsing
│   ├── url_frontier.cpp   # Queue management
│   ├── storage_manager.cpp # Graph storage
│   ├── thread_manager.cpp # Worker pool
│   ├── graph_algorithms.cpp # PageRank, SCC, Topo
│   └── url_scorer.cpp     # Priority scoring
├── include/               # Header files
├── dashboard/
│   └── index.html        # Interactive UI
├── output/               # Generated CSV files
├── api_server.py         # REST API
├── test_api.py           # API tests
├── metrics_analyzer.py   # Performance analysis
├── requirements.txt      # Python dependencies
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

## Algorithm Details

### PageRank Power Iteration
```
PR(v) = (1-d)/N + d × Σ(PR(u)/L(u))
```
- Damping factor: 0.85
- Iterations: 30
- Convergence: Stable after ~20 iterations

### Tarjan's SCC
- Single-pass DFS with discovery/low-link times
- Stack-based component extraction
- Identifies web communities (mutual link cycles)

### URL Scoring (Priority Queue)
Multi-factor heuristic:
- Trusted domains (GitHub, StackOverflow, etc.)
- Keywords (docs, api, tutorial, etc.)
- URL depth penalty
- Seed proximity bonus

## Demo Day Checklist

- [ ] Build crawler binary
- [ ] Start API server on port 5000
- [ ] Start dashboard on port 8080
- [ ] Run test crawl: `https://github.com`
- [ ] Show live progress display
- [ ] Navigate to Analytics → show interactive graph
- [ ] Navigate to Benchmarks → show performance metrics
- [ ] Explain algorithm complexity (all O(V+E) except Priority=O(E log V))

## Troubleshooting

**Crawler binary not found:**
```bash
cd build
cmake ..
make
```

**API connection failed:**
- Check `python api_server.py` is running
- Verify port 5000 is not in use
- Check firewall settings

**Empty dashboard:**
- API must be running first
- Run at least one crawl to generate CSV data
- Check browser console (F12) for errors

**Build errors on Windows:**
- Install CMake from cmake.org
- Install Visual Studio with C++ support
- Or use WSL/Linux VM (easier)

## License

MIT License - Feel free to use for academic projects

## Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing`)
5. Open Pull Request

## Acknowledgments

Built as part of Design & Analysis of Algorithms course project.

**Tech Stack:**
- C++17 (STL, threading)
- Python 3 (Flask, Flask-CORS)
- JavaScript (D3.js, Chart.js, Tailwind CSS)
- libcurl (HTTP client)
- CMake (build system)

---

**For detailed algorithm explanations and viva questions, see course documentation.**
