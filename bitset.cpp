//
//  author dzhiblavi
//

#include "bitset.hpp"

#define BASE 8
#define BITMAX 7
#define BITLOG 3

bitset::bitset(size_t size)
: data_((size & BITMAX) ? (size >> BITLOG) + 1 : (size >> BITLOG)),
  lc_last(size & BITMAX) {}

bitset::bitset(uint8_t const* data, size_t size)
: data_(size) {
    std::copy(data, data + size, (uint8_t*)data_.data());
}

bitset bitset::from_uint8_t(uint8_t x) {
    bitset ret;
    ret.data_.push_back(x);
    return ret;
}

size_t bitset::size() const {
    return data_.size() * BASE - (lc_last ? BASE - lc_last : 0);
}

size_t bitset::data_size() const {
    return data_.size();
}

void bitset::reserve(size_t size) {
    data_.reserve(size);
}

void bitset::append(bitset const& rhs) {
    if (data_.empty()) {
        *this = rhs;
        return;
    }
    if (rhs.data_.empty()) {
        return;
    }
    size_t old_size = data_.size();
    if (!lc_last) {
        data_.resize(data_.size() + rhs.data_.size());
        std::copy(rhs.data_.begin(), rhs.data_.end(), data_.begin() + old_size);
        lc_last = rhs.lc_last;
        return;
    }
    if (rhs.data_.size() == 1) {
        data_.back() |= (rhs.data_.front() >> lc_last);
        if (BASE - lc_last < rhs.lc_last || !rhs.lc_last) {
            data_.push_back(rhs.data_[0] << (BASE - lc_last));
        }
        lc_last = (lc_last + rhs.lc_last) & BITMAX;
        return;
    }
    data_.back() |= (rhs.data_.front() >> lc_last);
    data_.resize(data_.size() + rhs.data_.size() - 1);
    for (size_t i = 0; i < rhs.data_.size() - 1; ++i) {
        data_[old_size + i] = ((rhs.data_[i] << (BASE - lc_last)) | (rhs.data_[i + 1] >> lc_last));
    }
    if (BASE - lc_last < rhs.lc_last || !rhs.lc_last) {
        data_.push_back(rhs.data_.back() << (BASE - lc_last));
    }
    lc_last = (lc_last + rhs.lc_last) & BITMAX;
}

void bitset::push(uint8_t bit) {
    if (!lc_last) {
        data_.push_back(0);
    }
    if (bit) {
        data_.back() |= (1 << (BITMAX - lc_last));
    }
    ++lc_last;
    lc_last &= BITMAX;
}

void bitset::pop() {
    if (lc_last == 1) {
        lc_last = 0;
        data_.pop_back();
    } else {
        set(size() - 1, 0);
        lc_last = (lc_last + 7) & BITMAX;
    }
}

uint8_t bitset::get(size_t i) const {
    return (data_[i >> BITLOG] & (1ull << (BITMAX - (i & BITMAX)))) != 0;
}

void bitset::set(size_t i, uint8_t bit) {
    if (bit) {
        data_[i >> BITLOG] |= (1ull << (BITMAX - (i & BITMAX)));
    } else {
        if (get(i)) {
            data_[i >> BITLOG] ^= (1ull << (BITMAX - (i & BITMAX)));
        }
    }
}

uint8_t* bitset::data() {
    return data_.data();
}

uint8_t* bitset::begin() {
    return data();
}

uint8_t* bitset::end() {
    return data_.data() + data_.size();
}

std::string bitset::to_string() const {
    std::string ret;
    for (size_t i = 0; i < size(); ++i) {
        ret += char('0' + get(i));
    }
    return ret;
}
