#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// url_scorer.h
//
// Heuristic URL scorer for Best-First (Priority Queue) crawling.
//
// During crawling the full web graph is not yet known, so PageRank cannot
// be used.  Instead this scorer combines several cheap, stateless signals:
//
//   Signal              Weight   Rationale
//   ─────────────────────────────────────────────────────────────────────
//   Trusted domain      +0.40    High-value domains (wikipedia, github …)
//   Depth penalty       -0.10×d  Prefer shallower pages (d = hop depth)
//   Keyword relevance   +0.20    URL contains target keywords
//   URL length penalty  -0.10    Shorter URLs tend to be more canonical
//   Domain reputation   +0.10    Domain has .edu / .gov / .org suffix
//   Seed distance       +0.05    Explicitly counted incoming link frequency
//
// Final score is clamped to [0.0, 1.0].
//
// Complexity: O(|url| + K) per URL, where K = number of keywords (~constant).
// ─────────────────────────────────────────────────────────────────────────────

#ifndef URL_SCORER_H
#define URL_SCORER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>

class URLScorer {
public:
    // ── Configuration ─────────────────────────────────────────────────────────

    /** Add a domain name (no www., lowercase) that should receive a trust bonus. */
    void add_trusted_domain(const std::string& domain);

    /** Add a keyword that boosts URLs whose path/query contains it. */
    void add_keyword(const std::string& keyword);

    /**
     * Compute priority score for a URL at a given hop depth.
     *
     * Higher score = crawled sooner in PRIORITY mode.
     *
     * @param url    Fully-qualified URL (http/https)
     * @param depth  Hop distance from seed URL
     * @return       Score in [0.0, 1.0]
     */
    double score(const std::string& url, int depth) const;

    /**
     * Convenience: returns a batch of scores for a vector of URLs.
     */
    std::vector<double> score_batch(const std::vector<std::string>& urls, int depth) const;

    // ── Static helpers ────────────────────────────────────────────────────────

    /** Extract domain from URL (no www., lowercase). */
    static std::string extract_domain(const std::string& url);

    /** Extract path portion of URL (after domain). */
    static std::string extract_path(const std::string& url);

    /** Lowercase a string in-place. */
    static std::string to_lower(std::string s);

private:
    std::unordered_set<std::string> trusted_domains_;
    std::vector<std::string>        keywords_;

    // Weights
    static constexpr double W_TRUST    = 0.40;
    static constexpr double W_DEPTH    = 0.10;   // penalty per hop
    static constexpr double W_KEYWORD  = 0.20;
    static constexpr double W_URL_LEN  = 0.10;   // penalty for long URLs
    static constexpr double W_SUFFIX   = 0.10;
    static constexpr double W_SEED_PROX= 0.05;

    static constexpr int    DEPTH_MAX  = 5;       // beyond this no further penalty
    static constexpr int    URL_LEN_THRESHOLD = 80;

    bool has_trusted_suffix(const std::string& domain) const;
};

// ── Default scorer factory ────────────────────────────────────────────────────
/**
 * Creates a URLScorer pre-loaded with well-known trusted domains
 * and a set of CS / tech keywords.
 * Suitable for academic demonstration without additional configuration.
 */
URLScorer make_default_scorer();

#endif // URL_SCORER_H
