#ifndef GENERATE_BOOK_HPP
#define GENERATE_BOOK_HPP

#include <cstdint>
#include <random>
#include <vector>

class Book;

class GenerateBook {
public:
    GenerateBook(int midPrice, int spread, double pCancel,
                 double pExecute, std::uint64_t seed);

    // Seed the book with `numOrders` resting orders (no cancels/executes).
    void populate(Book& book, int numOrders);

    // Mixed flow: each message is add / cancel / execute according to
    // pCancel/pExecute (remainder is add).
    void stream(Book& book, int numMessages);

private:
    int midPrice;
    int spread;
    double pCancel;
    double pExecute;
    int nextOrderId = 1;

    std::mt19937_64 rng;
    std::normal_distribution<double> priceDist;
    std::uniform_int_distribution<int> sizeDist;
    std::uniform_real_distribution<double> sideDist;
    std::uniform_real_distribution<double> opDist;

    std::vector<int> liveIds;                              // pool for cancel/execute
};

#endif
