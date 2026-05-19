#ifndef ORDER_HPP
#define ORDER_HPP

class Limit;

class Order {
private:
    int idNumber;
    bool buyOrSell;
    int shares;
    int limit;
    long long entryTime;
    long long eventTime;
    Order *nextOrder;
    Order *prevOrder;
    Limit *parentLimit;

    friend class Limit;
    friend class Book;
public:
    Order(int _idNumber, bool _buyOrSell, int _shares, int _limit);
    Order(int _idNumber, bool _buyOrSell, int _shares, int _limit, long long _entryTime);
    ~Order();
    void cancel();
    void print() const;
};

#endif
