// Capacity + throughput benchmark for the Limit Order Book.
//
// Usage:
//   lob_bench [populate_n] [stream_n] [threads]
//
// Defaults: 1_000_000 populate orders, 1_000_000 stream messages, 1 thread.
// Pass `20000000` for `populate_n` to demonstrate the 20M-order capacity claim
// (expect ~2-3 GB RSS).
//
// Multi-threaded mode runs `threads` independent Book instances in parallel
// (per-symbol sharding), each populated and streamed.

#include "Book.hpp"
#include "GenerateBook.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
  #include <psapi.h>
  static std::size_t residentBytes() {
      PROCESS_MEMORY_COUNTERS pmc{};
      if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
          return pmc.WorkingSetSize;
      }
      return 0;
  }
#else
  #include <sys/resource.h>
  static std::size_t residentBytes() {
      struct rusage ru{};
      getrusage(RUSAGE_SELF, &ru);
      return static_cast<std::size_t>(ru.ru_maxrss) * 1024;
  }
#endif

static double secondsSince(std::chrono::steady_clock::time_point t0)
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now() - t0).count();
}

int main(int argc, char** argv)
{
    int populateN = (argc > 1) ? std::atoi(argv[1]) : 1'000'000;
    int streamN   = (argc > 2) ? std::atoi(argv[2]) : 1'000'000;
    int threads   = (argc > 3) ? std::atoi(argv[3]) : 1;

    std::cout << "lob_bench: populate=" << populateN
              << " stream=" << streamN
              << " threads=" << threads << "\n";

    std::size_t rssBefore = residentBytes();
    std::cout << "  RSS before:   " << (rssBefore / (1024.0 * 1024.0)) << " MB\n";

    // Books and generators live in the outer scope so RSS measurements include them.
    std::vector<std::unique_ptr<Book>>         books;
    std::vector<std::unique_ptr<GenerateBook>> gens;
    books.reserve(threads);
    gens.reserve(threads);
    for (int t = 0; t < threads; ++t) {
        books.emplace_back(std::make_unique<Book>());
        gens.emplace_back(std::make_unique<GenerateBook>(
            /*mid*/ 10000, /*spread*/ 50,
            /*pCancel*/ 0.40, /*pExecute*/ 0.10,
            /*seed*/ 42ull + t));
    }

    std::vector<double> populateSecs(threads, 0.0);
    std::vector<double> streamSecs(threads, 0.0);

    // Phase 1: populate
    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&, t]() {
            auto a = std::chrono::steady_clock::now();
            gens[t]->populate(*books[t], populateN);
            populateSecs[t] = secondsSince(a);
        });
    }
    for (auto& w : workers) w.join();
    workers.clear();
    double populateWall = secondsSince(t0);

    std::size_t rssAfterPopulate = residentBytes();
    std::size_t totalOrders = 0;
    for (auto& b : books) totalOrders += b->orderCount();

    std::cout << "  RSS post-pop: " << (rssAfterPopulate / (1024.0 * 1024.0)) << " MB"
              << "  (+" << ((rssAfterPopulate - rssBefore) / (1024.0 * 1024.0)) << " MB)\n";
    std::cout << "  resident orders: " << totalOrders;
    if (totalOrders > 0) {
        double bytesPerOrder =
            static_cast<double>(rssAfterPopulate - rssBefore) / totalOrders;
        std::cout << "   (~" << bytesPerOrder << " B/order)";
    }
    std::cout << "\n";

    // Phase 2: stream
    if (streamN > 0) {
        auto t1 = std::chrono::steady_clock::now();
        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&, t]() {
                auto a = std::chrono::steady_clock::now();
                gens[t]->stream(*books[t], streamN);
                streamSecs[t] = secondsSince(a);
            });
        }
        for (auto& w : workers) w.join();
        workers.clear();
        double streamWall = secondsSince(t1);
        (void)streamWall;

        std::size_t rssAfterStream = residentBytes();
        std::size_t ordersAfterStream = 0;
        for (auto& b : books) ordersAfterStream += b->orderCount();
        std::cout << "  RSS post-str: " << (rssAfterStream / (1024.0 * 1024.0)) << " MB\n";
        std::cout << "  resident orders after stream: " << ordersAfterStream << "\n";
    }

    double popMaxSec    = 0.0;
    double streamMaxSec = 0.0;
    for (int t = 0; t < threads; ++t) {
        if (populateSecs[t] > popMaxSec)    popMaxSec    = populateSecs[t];
        if (streamSecs[t]   > streamMaxSec) streamMaxSec = streamSecs[t];
    }

    long long totalPopulate = static_cast<long long>(populateN) * threads;
    long long totalStream   = static_cast<long long>(streamN)   * threads;

    if (popMaxSec > 0) {
        std::cout << "  populate (slowest thread): " << popMaxSec << " s, "
                  << (populateN / popMaxSec / 1e6) << " M/s per thread; aggregate "
                  << (totalPopulate / popMaxSec / 1e6) << " M/s\n";
    }
    if (streamMaxSec > 0) {
        std::cout << "  stream   (slowest thread): " << streamMaxSec << " s, "
                  << (streamN / streamMaxSec / 1e6) << " M/s per thread; aggregate "
                  << (totalStream / streamMaxSec / 1e6) << " M/s\n";
    }
    std::cout << "  wall populate: " << populateWall << " s\n";

    // Books are destroyed here; that destruction also has a cost but isn't measured.
    return 0;
}
