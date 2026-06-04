// ─────────────────────────────────────────────────────────────────────────────
// url_scorer.cpp
//
// Heuristic priority scorer for Best-First (Priority Queue) crawling.
//
// During an active crawl the full web graph is unknown, so PageRank cannot
// be applied.  This scorer combines cheap, stateless, per-URL signals to
// produce a priority in [0.0, 1.0] that drives the max-heap frontier.
//
// Signal breakdown (weights defined in url_scorer.h):
//   +0.40  Trusted domain bonus
//   -0.10  Depth penalty per hop (capped at DEPTH_MAX)
//   +0.20  Keyword presence in URL path / host
//   -0.10  URL length penalty (URLs > 80 chars are penalised)
//   +0.10  Reputable domain suffix (.edu, .gov, .org)
//   +0.05  Seed-proximity bonus (currently: depth == 0 or depth == 1)
//
// Score is clamped to [0.0, 1.0] before return.
//
// Time Complexity: O(|url| + K)  where K = number of configured keywords
// ─────────────────────────────────────────────────────────────────────────────

#include "url_scorer.h"
#include <algorithm>
#include <cctype>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string URLScorer::to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string URLScorer::extract_domain(const std::string& url) {
    // Skip protocol
    std::string rest = url;
    std::string lc   = to_lower(url);
    size_t proto = lc.find("://");
    if (proto != std::string::npos) {
        rest = rest.substr(proto + 3);
    }
    // Strip path
    size_t slash = rest.find('/');
    if (slash != std::string::npos) {
        rest = rest.substr(0, slash);
    }
    // Strip port
    size_t colon = rest.find(':');
    if (colon != std::string::npos) {
        rest = rest.substr(0, colon);
    }
    // Strip www.
    std::string lower_domain = to_lower(rest);
    if (lower_domain.rfind("www.", 0) == 0) {
        lower_domain = lower_domain.substr(4);
    }
    return lower_domain;
}

std::string URLScorer::extract_path(const std::string& url) {
    std::string lc = to_lower(url);
    size_t proto = lc.find("://");
    std::string rest = (proto != std::string::npos)
                       ? url.substr(proto + 3)
                       : url;
    size_t slash = rest.find('/');
    if (slash == std::string::npos) return "";
    return to_lower(rest.substr(slash));
}

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

void URLScorer::add_trusted_domain(const std::string& domain) {
    trusted_domains_.insert(to_lower(domain));
}

void URLScorer::add_keyword(const std::string& keyword) {
    keywords_.push_back(to_lower(keyword));
}

// ─────────────────────────────────────────────────────────────────────────────
// Suffix check helper
// ─────────────────────────────────────────────────────────────────────────────

bool URLScorer::has_trusted_suffix(const std::string& domain) const {
    // domain is already lowercased
    auto ends_with = [&](const std::string& suf) {
        if (domain.size() < suf.size()) return false;
        return domain.compare(domain.size() - suf.size(), suf.size(), suf) == 0;
    };
    return ends_with(".edu") || ends_with(".gov") || ends_with(".org")
        || ends_with(".ac.uk") || ends_with(".edu.au");
}

// ─────────────────────────────────────────────────────────────────────────────
// score()  —  O(|url| + K)
// ─────────────────────────────────────────────────────────────────────────────

double URLScorer::score(const std::string& url, int depth) const {
    if (url.empty()) return 0.0;

    double s = 0.0;

    std::string domain = extract_domain(url);
    std::string path   = extract_path(url);
    std::string full   = to_lower(url);

    // ── Signal 1: Trusted domain bonus  +0.40 ───────────────────────────────
    if (!trusted_domains_.empty() && trusted_domains_.count(domain)) {
        s += W_TRUST;
    }

    // ── Signal 2: Depth penalty  -0.10 per hop  (capped) ────────────────────
    // Deeper pages tend to be less canonical.
    // Cap at DEPTH_MAX to avoid negative scores from depth alone.
    int effective_depth = std::min(depth, DEPTH_MAX);
    s -= W_DEPTH * static_cast<double>(effective_depth);

    // ── Signal 3: Keyword relevance  +0.20 ──────────────────────────────────
    // Any keyword present anywhere in the full URL boosts the score.
    // Multiple keywords do NOT stack (single max bonus) to keep [0,1] feasible.
    if (!keywords_.empty()) {
        for (const auto& kw : keywords_) {
            if (full.find(kw) != std::string::npos) {
                s += W_KEYWORD;
                break;   // single bonus
            }
        }
    }

    // ── Signal 4: URL length penalty  -0.10 ─────────────────────────────────
    // Very long URLs are often parameter-heavy, low-quality pages.
    if (static_cast<int>(url.size()) > URL_LEN_THRESHOLD) {
        s -= W_URL_LEN;
    }

    // ── Signal 5: Reputable domain suffix  +0.10 ────────────────────────────
    if (has_trusted_suffix(domain)) {
        s += W_SUFFIX;
    }

    // ── Signal 6: Seed-proximity bonus  +0.05 ───────────────────────────────
    // Reward pages close to the seed (depth 0 or 1).
    if (depth <= 1) {
        s += W_SEED_PROX;
    }

    // Clamp to [0.0, 1.0]
    return std::max(0.0, std::min(1.0, s));
}

// ─────────────────────────────────────────────────────────────────────────────
// score_batch()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<double> URLScorer::score_batch(
    const std::vector<std::string>& urls, int depth) const
{
    std::vector<double> scores;
    scores.reserve(urls.size());
    for (const auto& u : urls) {
        scores.push_back(score(u, depth));
    }
    return scores;
}

// ─────────────────────────────────────────────────────────────────────────────
// make_default_scorer()
//
// Pre-loaded with well-known trusted domains and CS/tech keywords.
// ─────────────────────────────────────────────────────────────────────────────

URLScorer make_default_scorer() {
    URLScorer scorer;

    // High-value reference / educational domains
    const char* trusted[] = {
        "wikipedia.org",
        "github.com",
        "arxiv.org",
        "stackoverflow.com",
        "scholar.google.com",
        "ieee.org",
        "acm.org",
        "mit.edu",
        "stanford.edu",
        "cs.cmu.edu",
        "nature.com",
        "sciencedirect.com",
        "springer.com",
        "researchgate.net",
        "docs.python.org",
        "cppreference.com",
        "developer.mozilla.org",
        nullptr
    };
    for (int i = 0; trusted[i]; ++i) {
        scorer.add_trusted_domain(trusted[i]);
    }

    // CS / AI / algorithms keywords
    const char* keywords[] = {
        "algorithm",
        "machine-learning",
        "deep-learning",
        "neural",
        "graph",
        "data-structure",
        "research",
        "paper",
        "cs",
        "ai",
        "security",
        "cryptography",
        "distributed",
        "database",
        nullptr
    };
    for (int i = 0; keywords[i]; ++i) {
        scorer.add_keyword(keywords[i]);
    }

    return scorer;
}
