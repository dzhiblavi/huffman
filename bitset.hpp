//
//  author dzhiblavi
//

#ifndef HUFFMAN_BITSET_H_
#define HUFFMAN_BITSET_H_

#include <vector>
#include <algorithm>
#include <iostream>

class bitset {
    std::vector<uint8_t> data_;
    int lc_last = 0;

public:
    explicit bitset(size_t = 0);
    bitset(uint8_t const*, size_t);
    bitset(bitset const& rhs) = default;
    bitset(bitset&& rhs) = default;
    ~bitset() = default;

    static bitset from_uint8_t(uint8_t);

    size_t size() const;
    size_t data_size() const;

    bitset& operator=(bitset const& rhs) = default;
    bitset& operator=(bitset&& rhs) = default;

    void append(bitset const&);

    void push(uint8_t);
    void pop();
    void reserve(size_t);

    uint8_t operator[](size_t) const;
    void set(size_t);
    void reset(size_t);
    void flip(size_t);

    uint8_t* data();
    uint8_t* begin();
    uint8_t* end();

    std::string to_string() const;
};

#endif // HUFFMAN_BITSET_H_
