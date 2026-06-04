#!/usr/bin/env python3
"""
Web Crawler Metrics Analyzer
Reads output/metrics.csv and prints a summary + generates an HTML report.

Column names produced by main.cpp (v2):
  seed_url, max_pages, num_threads, mode,
  crawl_ms, merge_ms, pagerank_ms, scc_ms, topo_ms,
  pages_crawled, nodes, sccs, throughput
"""

import csv
from pathlib import Path
from statistics import mean, stdev

# ── Locate metrics file ───────────────────────────────────────────────────────
# Try output/metrics.csv first (new layout), fall back to build/metrics.csv
_base = Path(__file__).parent
for _candidate in [
    _base / "output" / "metrics.csv",
    _base / "build"  / "metrics.csv",
    _base / "metrics.csv",
]:
    if _candidate.exists():
        csv_file = _candidate
        break
else:
    print("Error: metrics.csv not found in output/, build/, or project root.")
    print("Run the crawler first to generate data.")
    raise SystemExit(1)

print(f"Reading: {csv_file}")

# ── Load data ─────────────────────────────────────────────────────────────────
data = []
with open(csv_file, "r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        # Support both old 'total_ms' column and new 'crawl_ms' column
        crawl_ms = int(row.get("crawl_ms") or row.get("total_ms") or 0)
        data.append({
            "seed_url":     row.get("seed_url", ""),
            "mode":         row.get("mode", "bfs"),
            "num_threads":  int(row.get("num_threads", 1)),
            "max_pages":    int(row.get("max_pages",   0)),
            "crawl_ms":     crawl_ms,
            "merge_ms":     int(row.get("merge_ms",    0)),
            "pagerank_ms":  int(row.get("pagerank_ms", 0)),
            "scc_ms":       int(row.get("scc_ms",      0)),
            "topo_ms":      int(row.get("topo_ms",     0)),
            "pages_crawled":int(row.get("pages_crawled", 0)),
            "nodes":        int(row.get("nodes",        0)),
            "sccs":         int(row.get("sccs",         0)),
            "throughput":   float(row.get("throughput", 0)),
        })

if not data:
    print("No data rows found in metrics.csv.")
    raise SystemExit(1)

# ── Console summary ───────────────────────────────────────────────────────────
print("=" * 80)
print("CRAWLER METRICS SUMMARY".center(80))
print("=" * 80)
print()
print(f"{'Domain':<32} {'Mode':<10} {'Threads':<8} {'Pages':<8} "
      f"{'Crawl(ms)':<12} {'T-Put(p/s)':<12} {'Nodes':<8} {'SCCs'}")
print("-" * 96)

for row in data:
    domain = row["seed_url"].split("/")[2] if "://" in row["seed_url"] else row["seed_url"]
    domain = domain[:28] + "..." if len(domain) > 28 else domain
    print(f"{domain:<32} {row['mode']:<10} {row['num_threads']:<8} "
          f"{row['pages_crawled']:<8} {row['crawl_ms']:<12} "
          f"{row['throughput']:<12.2f} {row['nodes']:<8} {row['sccs']}")

print()
print("=" * 80)
print("PER-MODE COMPARISON".center(80))
print("=" * 80)
modes: dict = {}
for row in data:
    modes.setdefault(row["mode"], []).append(row)
for m, rows in sorted(modes.items()):
    avg_tp = mean(r["throughput"] for r in rows)
    avg_ms = mean(r["crawl_ms"]   for r in rows)
    print(f"  {m.upper():<10}  runs={len(rows)}  "
          f"avg_crawl={avg_ms/1000:.2f}s  avg_throughput={avg_tp:.2f} p/s")

print()
print("=" * 80)
print("STATISTICS".center(80))
print("=" * 80)
print(f"  Total runs          : {len(data)}")
print(f"  Total pages crawled : {sum(r['pages_crawled'] for r in data)}")
print(f"  Average throughput  : {mean(r['throughput']   for r in data):.2f} p/s")
print(f"  Max throughput      : {max(r['throughput']    for r in data):.2f} p/s")
print(f"  Min throughput      : {min(r['throughput']    for r in data):.2f} p/s")
if len(data) > 1:
    print(f"  Throughput stdev    : {stdev(r['throughput'] for r in data):.2f} p/s")
print(f"  Average crawl time  : {mean(r['crawl_ms'] for r in data)/1000:.2f} s")
print()

# ── HTML report ───────────────────────────────────────────────────────────────
html = """<!DOCTYPE html>
<html>
<head>
  <title>Crawler Metrics Report</title>
  <style>
    * { font-family: 'Segoe UI', sans-serif; box-sizing: border-box; }
    body { background: linear-gradient(135deg,#667eea,#764ba2); margin:0; padding:20px; }
    .container { max-width:1200px; margin:auto; background:#fff; border-radius:10px; padding:30px; box-shadow:0 10px 30px rgba(0,0,0,.3); }
    h1 { color:#333; text-align:center; border-bottom:3px solid #667eea; padding-bottom:15px; }
    h2 { color:#667eea; margin-top:30px; }
    table { width:100%; border-collapse:collapse; margin:20px 0; }
    th { background:#667eea; color:#fff; padding:12px; text-align:left; }
    td { padding:10px 12px; border-bottom:1px solid #ddd; }
    tr:hover { background:#f5f5f5; }
    .stat-grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(160px,1fr)); gap:15px; margin:20px 0; }
    .stat-box { background:linear-gradient(135deg,#667eea,#764ba2); color:#fff; padding:20px; border-radius:8px; text-align:center; }
    .stat-value { font-size:26px; font-weight:bold; }
    .stat-label { font-size:12px; opacity:.9; margin-top:5px; }
    .bar-item { display:flex; align-items:center; margin:8px 0; }
    .bar-label { width:160px; font-weight:500; font-size:13px; }
    .bar-track { flex:1; background:#e0e0e0; height:24px; border-radius:4px; overflow:hidden; }
    .bar-fill { background:linear-gradient(90deg,#667eea,#764ba2); height:100%;
                display:flex; align-items:center; padding-left:8px;
                color:#fff; font-weight:bold; font-size:12px; }
    .charts { display:grid; grid-template-columns:1fr 1fr; gap:20px; }
    .chart { background:#f9f9f9; padding:20px; border-radius:8px; border-left:4px solid #667eea; }
    .footer { text-align:center; margin-top:30px; color:#999; font-size:12px; }
  </style>
</head>
<body><div class="container">
  <h1>🚀 Web Crawler — Performance Report</h1>

  <h2>📊 Summary Statistics</h2>
  <div class="stat-grid">
"""

stats = {
    "Total Runs":     len(data),
    "Total Pages":    sum(r["pages_crawled"] for r in data),
    "Avg Throughput": f"{mean(r['throughput'] for r in data):.2f} p/s",
    "Peak Throughput":f"{max(r['throughput']  for r in data):.2f} p/s",
    "Avg Crawl Time": f"{mean(r['crawl_ms']   for r in data)/1000:.2f} s",
    "Max Nodes":      max(r["nodes"] for r in data),
}
for label, value in stats.items():
    html += f'    <div class="stat-box"><div class="stat-value">{value}</div><div class="stat-label">{label}</div></div>\n'

html += """  </div>

  <h2>📈 Run Details</h2>
  <table>
    <tr>
      <th>Domain</th><th>Mode</th><th>Threads</th><th>Pages</th>
      <th>Crawl (ms)</th><th>PR (ms)</th><th>SCC (ms)</th>
      <th>Nodes</th><th>SCCs</th><th>Throughput</th>
    </tr>
"""
for row in data:
    domain = row["seed_url"].split("/")[2] if "://" in row["seed_url"] else row["seed_url"]
    html += (f'    <tr><td><code>{domain}</code></td><td>{row["mode"]}</td>'
             f'<td>{row["num_threads"]}</td><td>{row["pages_crawled"]}</td>'
             f'<td>{row["crawl_ms"]}</td><td>{row["pagerank_ms"]}</td>'
             f'<td>{row["scc_ms"]}</td><td>{row["nodes"]}</td><td>{row["sccs"]}</td>'
             f'<td>{row["throughput"]:.2f}</td></tr>\n')

html += """  </table>

  <h2>📊 Visual Comparison</h2>
  <div class="charts">
    <div class="chart"><h3>Throughput per Run</h3>
"""
max_tp = max(r["throughput"] for r in data) or 1
for row in data:
    domain  = row["seed_url"].split("/")[2] if "://" in row["seed_url"] else row["seed_url"]
    pct     = row["throughput"] / max_tp * 100
    html += (f'      <div class="bar-item"><div class="bar-label">{domain[:22]} '
             f'({row["num_threads"]}T)</div>'
             f'<div class="bar-track"><div class="bar-fill" style="width:{pct:.1f}%">'
             f'{row["throughput"]:.2f}</div></div></div>\n')

html += "    </div>\n    <div class=\"chart\"><h3>Crawl Time (ms)</h3>\n"
max_ms = max(r["crawl_ms"] for r in data) or 1
for row in data:
    domain = row["seed_url"].split("/")[2] if "://" in row["seed_url"] else row["seed_url"]
    pct    = row["crawl_ms"] / max_ms * 100
    html += (f'      <div class="bar-item"><div class="bar-label">{domain[:22]} '
             f'({row["num_threads"]}T)</div>'
             f'<div class="bar-track"><div class="bar-fill" style="width:{pct:.1f}%">'
             f'{row["crawl_ms"]} ms</div></div></div>\n')

html += """    </div>
  </div>
  <div class="footer">Generated from metrics.csv | Web Crawler Performance Analysis</div>
</div></body></html>
"""

out_dir = csv_file.parent
html_path = out_dir / "metrics_report.html"
with open(html_path, "w", encoding="utf-8") as f:
    f.write(html)

print(f"✅ HTML report saved to: {html_path}")
print()
