#!/usr/bin/env python3
"""
Quick API Test Script
Tests all endpoints to verify API server is working
"""

import requests
import json
import time

API_BASE = "http://localhost:5000/api"

def test_endpoint(method, endpoint, data=None, desc=""):
    """Test a single API endpoint"""
    url = f"{API_BASE}{endpoint}"
    print(f"\n{'='*60}")
    print(f"Testing: {method} {endpoint}")
    if desc:
        print(f"Purpose: {desc}")
    print(f"{'='*60}")
    
    try:
        if method == "GET":
            response = requests.get(url, timeout=5)
        elif method == "POST":
            response = requests.post(url, json=data, timeout=5)
        
        print(f"Status Code: {response.status_code}")
        
        if response.status_code == 200:
            result = response.json()
            print(f"Response Type: {type(result)}")
            
            if isinstance(result, list):
                print(f"Items Count: {len(result)}")
                if len(result) > 0:
                    print(f"First Item: {json.dumps(result[0], indent=2)}")
            elif isinstance(result, dict):
                print(f"Response: {json.dumps(result, indent=2)}")
            
            print("✅ PASS")
            return True
        else:
            print(f"Error: {response.text}")
            print("❌ FAIL")
            return False
    
    except requests.exceptions.ConnectionError:
        print("❌ FAIL - Cannot connect to API server")
        print("   Make sure api_server.py is running on port 5000")
        return False
    except Exception as e:
        print(f"❌ FAIL - {str(e)}")
        return False

def main():
    print("""
╔══════════════════════════════════════════════════════════════╗
║          Web Crawler API Test Suite                         ║
╚══════════════════════════════════════════════════════════════╝
    """)
    
    results = []
    
    # Test 1: Server health
    results.append(test_endpoint(
        "GET", "",
        desc="Check if API server is running"
    ))
    
    # Test 2: PageRank data
    results.append(test_endpoint(
        "GET", "/pagerank",
        desc="Load PageRank results from CSV"
    ))
    
    # Test 3: SCC data
    results.append(test_endpoint(
        "GET", "/scc",
        desc="Load SCC (Strongly Connected Components) data"
    ))
    
    # Test 4: Metrics data
    results.append(test_endpoint(
        "GET", "/metrics",
        desc="Load performance metrics"
    ))
    
    # Test 5: Graph data
    results.append(test_endpoint(
        "GET", "/graph-data",
        desc="Load graph visualization data"
    ))
    
    # Test 6: Crawl status
    results.append(test_endpoint(
        "GET", "/status",
        desc="Get current crawl status"
    ))
    
    # Test 7: Start crawl (optional - comment out if you don't want to actually crawl)
    print("\n" + "="*60)
    print("Test 7: Start Crawl (SKIPPED - uncomment to test)")
    print("To test crawl functionality, uncomment lines below")
    print("="*60)
    
    # Uncomment to test actual crawling:
    # results.append(test_endpoint(
    #     "POST", "/crawl",
    #     data={
    #         "seed_url": "https://example.com",
    #         "strategy": "bfs",
    #         "max_pages": 10,
    #         "num_threads": 2
    #     },
    #     desc="Start a small test crawl"
    # ))
    # 
    # if results[-1]:  # If crawl started successfully
    #     print("\nWaiting for crawl to complete...")
    #     for i in range(30):  # Wait up to 15 seconds
    #         time.sleep(0.5)
    #         status = requests.get(f"{API_BASE}/status").json()
    #         print(f"  Progress: {status['progress_percent']}% - Status: {status['status']}")
    #         if status['status'] in ['completed', 'error']:
    #             break
    
    # Summary
    print("\n" + "="*60)
    print("TEST SUMMARY")
    print("="*60)
    passed = sum(results)
    total = len(results)
    print(f"Passed: {passed}/{total}")
    
    if passed == total:
        print("✅ All tests passed! API server is working correctly.")
    else:
        print(f"⚠️  {total - passed} test(s) failed.")
        print("\nCommon issues:")
        print("  1. API server not running → Start with: python api_server.py")
        print("  2. CSV files missing → Run crawler once to generate files")
        print("  3. Wrong port → Check if server is on port 5000")
    
    print("\n" + "="*60)

if __name__ == "__main__":
    main()
