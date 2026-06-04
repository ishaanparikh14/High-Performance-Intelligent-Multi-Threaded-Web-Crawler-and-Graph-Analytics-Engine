#!/usr/bin/env python3
"""
Web Crawler REST API Server
Provides HTTP endpoints for the C++ multi-threaded web crawler.

Architecture:
  POST /api/crawl  → spawns C++ crawler subprocess
                   → crawler writes CSVs to ./output/
  GET  /api/*      → reads those CSVs and returns JSON

Live progress is tracked by monitoring the crawler's stdout in real time.
"""

from flask import Flask, jsonify, request, Response
from flask_cors import CORS
import subprocess
import csv
import os
import sys
import json
import threading
import time
from datetime import datetime

app = Flask(__name__)
CORS(app)  # Enable CORS for dashboard

# ─────────────────────────────────────────────────────────────────────────────
# Global crawl state  (single-crawl-at-a-time model)
# ─────────────────────────────────────────────────────────────────────────────
crawl_status = {
    "status":           "idle",   # idle | running | completed | error
    "pages_crawled":    0,
    "urls_discovered":  0,
    "active_threads":   0,
    "queue_size":       0,
    "throughput":       0.0,
    "progress_percent": 0,
    "started_at":       None,
    "completed_at":     None,
    "error_message":    None,
    "max_pages":        0,
}
crawl_lock = threading.Lock()

# ── Output directory ──────────────────────────────────────────────────────────
# The C++ crawler is invoked with cwd=OUTPUT_DIR so that all its CSVs land
# inside that folder.  That keeps the project root clean.
OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "output")


def ensure_output_dir():
    os.makedirs(OUTPUT_DIR, exist_ok=True)


def read_csv(filename):
    """Return rows of *filename* (relative to OUTPUT_DIR) as a list of dicts."""
    filepath = os.path.join(OUTPUT_DIR, filename)
    if not os.path.exists(filepath):
        return []
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            return list(csv.DictReader(f))
    except Exception as e:
        print(f"[WARN] Error reading {filename}: {e}")
        return []


# ─────────────────────────────────────────────────────────────────────────────
# Locate the compiled C++ binary
# ─────────────────────────────────────────────────────────────────────────────
def find_crawler_binary():
    """
    Search likely locations for the compiled crawler binary.
    Returns the absolute path if found, else None.
    """
    base = os.path.dirname(os.path.abspath(__file__))
    candidates = [
        os.path.join(base, "build", "crawler"),          # Linux / macOS cmake build
        os.path.join(base, "build", "crawler.exe"),      # Windows cmake build
        os.path.join(base, "build", "Release", "crawler.exe"),
        os.path.join(base, "build", "Debug",   "crawler.exe"),
        os.path.join(base, "crawler"),                   # hand-compiled in root
        os.path.join(base, "crawler.exe"),
    ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    return None


# ─────────────────────────────────────────────────────────────────────────────
# Background crawler thread
# ─────────────────────────────────────────────────────────────────────────────
def run_crawler(seed_url, strategy, max_pages, threads):
    """
    Spawn the C++ crawler subprocess and stream its stdout to update
    crawl_status in real time.

    The crawler prints lines like:
        [TIMING] …
        [INFO]   …
    We parse these to approximate live progress.
    """
    global crawl_status

    with crawl_lock:
        crawl_status.update({
            "status":           "running",
            "started_at":       datetime.now().isoformat(),
            "pages_crawled":    0,
            "urls_discovered":  0,
            "queue_size":       max_pages,
            "active_threads":   threads,
            "throughput":       0.0,
            "progress_percent": 0,
            "completed_at":     None,
            "error_message":    None,
            "max_pages":        max_pages,
        })

    binary = find_crawler_binary()
    if binary is None:
        with crawl_lock:
            crawl_status["status"] = "error"
            crawl_status["error_message"] = (
                "Crawler binary not found. "
                "Build it first: cd build && cmake .. && make"
            )
        print("[ERROR] Crawler binary not found.")
        return

    ensure_output_dir()

    cmd = [binary, seed_url, str(max_pages), str(threads), strategy]
    print(f"[INFO] Executing: {' '.join(cmd)}")
    print(f"[INFO] Working directory: {OUTPUT_DIR}")

    try:
        process = subprocess.Popen(
            cmd,
            cwd=OUTPUT_DIR,          # CSVs land in output/
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, # merge stderr into stdout
            text=True,
            bufsize=1,               # line-buffered
        )

        start_time = time.time()
        pages_seen = 0

        for line in process.stdout:
            line = line.rstrip()
            if line:
                print(f"[CRAWLER] {line}")

            # ── Parse progress from crawler output ────────────────────────
            # main.cpp prints: [TIMING] Crawl completed in NNN ms
            if "Crawl completed in" in line:
                try:
                    ms = int(line.split("in")[-1].split("ms")[0].strip())
                    tput = pages_seen * 1000.0 / ms if ms > 0 else 0
                    with crawl_lock:
                        crawl_status["throughput"]       = round(tput, 2)
                        crawl_status["progress_percent"] = 60  # crawl done, still analysing
                except Exception:
                    pass

            # main.cpp prints: [RESULTS]  Pages crawled:   NNN
            if "Pages crawled:" in line:
                try:
                    pages_seen = int(line.split(":")[-1].strip())
                    elapsed = time.time() - start_time
                    tput    = pages_seen / elapsed if elapsed > 0 else 0
                    with crawl_lock:
                        crawl_status["pages_crawled"]    = pages_seen
                        crawl_status["throughput"]       = round(tput, 2)
                        crawl_status["progress_percent"] = 80
                except Exception:
                    pass

            # main.cpp prints: [RESULTS]  Graph nodes:   NNN
            if "Graph nodes:" in line:
                try:
                    nodes = int(line.split(":")[-1].strip())
                    with crawl_lock:
                        crawl_status["urls_discovered"] = nodes
                except Exception:
                    pass

            # Incremental estimate while crawling
            if "pages" in line.lower() and "crawled" in line.lower():
                elapsed = time.time() - start_time
                if elapsed > 0 and pages_seen < max_pages:
                    pages_seen += 1
                    tput = pages_seen / elapsed
                    pct  = min(55, int(pages_seen / max_pages * 55))
                    with crawl_lock:
                        crawl_status["pages_crawled"]    = pages_seen
                        crawl_status["queue_size"]       = max(0, max_pages - pages_seen)
                        crawl_status["throughput"]       = round(tput, 2)
                        crawl_status["progress_percent"] = pct

        process.wait(timeout=360)
        rc = process.returncode

        if rc == 0:
            with crawl_lock:
                crawl_status["status"]           = "completed"
                crawl_status["completed_at"]     = datetime.now().isoformat()
                crawl_status["progress_percent"] = 100
            print("[INFO] Crawler completed successfully.")
        else:
            with crawl_lock:
                crawl_status["status"]        = "error"
                crawl_status["error_message"] = f"Crawler exited with code {rc}"
            print(f"[ERROR] Crawler exited with code {rc}")

    except subprocess.TimeoutExpired:
        process.kill()
        with crawl_lock:
            crawl_status["status"]        = "error"
            crawl_status["error_message"] = "Crawler timed out (> 6 min)"
        print("[ERROR] Crawler timed out.")
    except Exception as exc:
        with crawl_lock:
            crawl_status["status"]        = "error"
            crawl_status["error_message"] = str(exc)
        print(f"[ERROR] Exception during crawl: {exc}")


# ─────────────────────────────────────────────────────────────────────────────
# API endpoints
# ─────────────────────────────────────────────────────────────────────────────

@app.route("/")
def index():
    return jsonify({
        "name":    "Web Crawler API",
        "version": "2.0.0",
        "endpoints": {
            "POST /api/crawl":      "Start a new crawl",
            "GET  /api/status":     "Live crawl progress",
            "GET  /api/pagerank":   "PageRank results",
            "GET  /api/scc":        "SCC community detection",
            "GET  /api/topo":       "Topological sort order",
            "GET  /api/metrics":    "Performance metrics",
            "GET  /api/graph-data": "D3 graph data",
            "GET  /api/crawled":    "Raw crawled pages",
        }
    })


@app.route("/api/crawl", methods=["POST"])
def start_crawl():
    global crawl_status

    with crawl_lock:
        if crawl_status["status"] == "running":
            return jsonify({"status": "error", "message": "A crawl is already in progress"}), 409

    data      = request.get_json(force=True) or {}
    seed_url  = str(data.get("seed_url",    "https://example.com"))
    strategy  = str(data.get("strategy",    "bfs")).lower()
    max_pages = int(data.get("max_pages",   50))
    threads   = int(data.get("num_threads", 4))

    if not (seed_url.startswith("http://") or seed_url.startswith("https://")):
        return jsonify({"status": "error", "message": "seed_url must start with http:// or https://"}), 400
    if not 1 <= max_pages <= 1000:
        return jsonify({"status": "error", "message": "max_pages must be 1–1000"}), 400
    if not 1 <= threads <= 16:
        return jsonify({"status": "error", "message": "num_threads must be 1–16"}), 400
    if strategy not in ("bfs", "dfs", "priority"):
        return jsonify({"status": "error", "message": "strategy must be bfs, dfs, or priority"}), 400

    t = threading.Thread(
        target=run_crawler,
        args=(seed_url, strategy, max_pages, threads),
        daemon=True,
    )
    t.start()

    return jsonify({
        "status":  "started",
        "message": "Crawl started",
        "params":  {
            "seed_url":    seed_url,
            "strategy":    strategy,
            "max_pages":   max_pages,
            "num_threads": threads,
        }
    })


@app.route("/api/status")
def get_status():
    with crawl_lock:
        return jsonify(dict(crawl_status))


@app.route("/api/pagerank")
def get_pagerank():
    rows    = read_csv("pagerank_results.csv")
    crawled = {r["domain"]: r for r in read_csv("crawled_pages.csv")}

    result = []
    for row in rows:
        domain = row.get("domain", "")
        ci     = crawled.get(domain, {})
        result.append({
            "domain":         domain,
            "pagerank_score": float(row.get("pagerank_score", 0)),
            "outgoing_links": int(ci.get("outgoing_links", 0)),
            "visit_count":    int(ci.get("visit_count", 0)),
        })

    result.sort(key=lambda x: x["pagerank_score"], reverse=True)
    return jsonify(result)


@app.route("/api/scc")
def get_scc():
    rows   = read_csv("scc_results.csv")
    result = []
    for row in rows:
        raw     = row.get("domains", "")
        members = [d for d in raw.split("|") if d] if raw else []
        result.append({
            "scc_id":  int(row.get("scc_id", 0)),
            "size":    int(row.get("size",   0)),
            "domains": members,
        })
    result.sort(key=lambda x: x["size"], reverse=True)
    return jsonify(result)


@app.route("/api/topo")
def get_topo():
    return jsonify(read_csv("topo_order.csv"))


@app.route("/api/metrics")
def get_metrics():
    rows   = read_csv("metrics.csv")
    result = []
    for row in rows:
        result.append({
            "seed_url":    row.get("seed_url",    ""),
            "mode":        row.get("mode",        "bfs"),
            "num_threads": int(row.get("num_threads", 1)),
            "max_pages":   int(row.get("max_pages",   0)),
            "crawl_ms":    int(row.get("crawl_ms",    0)),
            "merge_ms":    int(row.get("merge_ms",    0)),
            "pagerank_ms": int(row.get("pagerank_ms", 0)),
            "scc_ms":      int(row.get("scc_ms",      0)),
            "topo_ms":     int(row.get("topo_ms",     0)),
            "pages_crawled": int(row.get("pages_crawled", 0)),
            "nodes":       int(row.get("nodes",       0)),
            "sccs":        int(row.get("sccs",        0)),
            "throughput":  float(row.get("throughput", 0)),
        })
    return jsonify(result)


@app.route("/api/graph-data")
def get_graph_data():
    crawled  = read_csv("crawled_pages.csv")
    pr_index = {r["domain"]: float(r["pagerank_score"])
                for r in read_csv("pagerank_results.csv")}

    top30 = sorted(crawled, key=lambda x: pr_index.get(x["domain"], 0), reverse=True)[:30]

    nodes = [{
        "id":            r["domain"],
        "pagerank":      pr_index.get(r["domain"], 0.001),
        "outgoing_links": int(r.get("outgoing_links", 0)),
    } for r in top30]

    # Build real links from crawled_pages (domain → outgoing)
    # Fallback: synthetic links when no real link data is available
    node_ids = {n["id"] for n in nodes}
    links    = []
    for r in top30:
        src = r["domain"]
        for tgt in r.get("outgoing_links_list", "").split("|"):
            if tgt and tgt in node_ids and tgt != src:
                links.append({"source": src, "target": tgt})

    # Synthetic fallback when real edge list isn't in the CSV
    if not links:
        for i, node in enumerate(nodes):
            targets = [n for j, n in enumerate(nodes) if j != i][:2]
            for t in targets:
                links.append({"source": node["id"], "target": t["id"]})

    return jsonify({"nodes": nodes, "links": links})


@app.route("/api/crawled")
def get_crawled():
    return jsonify(read_csv("crawled_pages.csv"))


# ─────────────────────────────────────────────────────────────────────────────
# SSE  (Server-Sent Events)  — optional live stream endpoint
# ─────────────────────────────────────────────────────────────────────────────
@app.route("/api/stream")
def stream():
    def generate():
        last = None
        while True:
            with crawl_lock:
                current = dict(crawl_status)
            if current != last:
                yield f"data: {json.dumps(current)}\n\n"
                last = current
            if current["status"] in ("completed", "error", "idle"):
                break
            time.sleep(0.4)
    return Response(generate(), mimetype="text/event-stream")


# ─────────────────────────────────────────────────────────────────────────────
# Startup
# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    ensure_output_dir()

    binary = find_crawler_binary()
    binary_status = f"✅ Found: {binary}" if binary else "⚠️  NOT FOUND — build first!"

    print("=" * 60)
    print("  Web Crawler REST API Server  v2.0")
    print("=" * 60)
    print(f"  Output dir : {OUTPUT_DIR}")
    print(f"  C++ binary : {binary_status}")
    print()
    print("  Endpoints:")
    print("    POST http://localhost:5000/api/crawl")
    print("    GET  http://localhost:5000/api/status")
    print("    GET  http://localhost:5000/api/pagerank")
    print("    GET  http://localhost:5000/api/scc")
    print("    GET  http://localhost:5000/api/metrics")
    print("    GET  http://localhost:5000/api/graph-data")
    print()
    print("  Press Ctrl+C to stop")
    print("=" * 60)

    app.run(host="0.0.0.0", port=5000, debug=False, threaded=True)
