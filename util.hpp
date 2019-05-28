/*
    author dzhiblavi
 */
// ВНИМАНИЕ:
// Ребята не стоит вскрывать эту тему. Вы молодые, шутливые, вам все легко.
// Это не то. Это не Чикатило и даже не архивы спецслужб. Сюда лучше не лезть.
// Серьезно, любой из вас будет жалеть. Лучше закройте файл и забудьте что
// тут писалось. Я вполне понимаю что данным сообщением вызову дополнительный интерес,
// но хочу сразу предостеречь пытливых - стоп. Остальные просто не найдут.

#ifndef HUFFMAN_UTIL_HPP_
#define HUFFMAN_UTIL_HPP_

#define CRCMASK 0xFFFFFFFFUL
#define MTHREAD_LAUNCH_MINIMAL 4096000
#define THREAD_CNT 8
#define THREAD_EXP 3

#define ALPH_SIZE 256
#define BLOCK_SIZE_BYTES 4
#define HASH_SIZE_BYTES 4
#define HEADER_SIZE 8

#include <thread>
#include <vector>

#include "bitset.hpp"

template<typename It>
uint8_t convert_to_byte(It p) {
    return *reinterpret_cast<uint8_t const*>(&(*p));
}

template<typename T>
void write_binary_(T value, std::string::iterator data) {
    std::copy((uint8_t*)&value, (uint8_t*)&value + sizeof(T), data);
}

template<size_t t>
constexpr uint32_t poly_hash = t & 1 ? (t >> 1) ^ 0xEDB88320UL : t >> 1;

template<size_t i, size_t t>
constexpr uint32_t for_hash256 = for_hash256<i - 1, poly_hash<t>>;

template<size_t t>
constexpr uint32_t for_hash256<0, t> = poly_hash<t>;

template<size_t t>
constexpr uint32_t hash256 = for_hash256<7, t>;

template<size_t t, size_t r>
struct crc32_table : crc32_table<t - 1, r + 1> {
    crc32_table() { this->vals[t] = hash256<t>; }
};

template<size_t r>
struct crc32_table<0, r> {
    crc32_table() { this->vals[0] = hash256<0>; }
    uint32_t operator[](size_t i) { return vals[i]; }

protected:
    uint32_t vals[r + 1];
};

using CRC32_TABLE = crc32_table<255, 0>;

uint32_t crc32_hash(uint32_t hash, uint8_t c);

template<typename InputIt>
uint32_t crc32(InputIt first, InputIt last) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;
    static_assert(sizeof(value_type) == 1);

    uint32_t crc = CRCMASK;
    std::for_each(first, last, [& crc](value_type const& c) {
        crc = crc32_hash(crc, convert_to_byte(&c));
    });
    return crc ^ CRCMASK;
}

template<typename InputIt, typename F, typename URet, typename U, typename... Args>
void parallel_calc_impl(F&& f, U&&, InputIt first, InputIt last, URet& ret, std::input_iterator_tag, Args&&... args) {
    f(first, last, ret, std::forward<Args>(args)...);
}

template<typename ForwardIt, typename F, typename URet, typename U, typename... Args>
void parallel_calc_impl(F&& f, U&& u, ForwardIt first, ForwardIt last, URet& ret, std::forward_iterator_tag, Args&&... args) {
    typename std::iterator_traits<ForwardIt>::difference_type dist = std::distance(first, last);

    if (dist < MTHREAD_LAUNCH_MINIMAL) {
        f(first, last, ret, std::forward<Args>(args)...);
        return;
    }

    std::vector<URet> s(THREAD_CNT, ret);
    std::vector<std::thread> t;

    for (size_t i = 0; i < THREAD_CNT - 1; ++i) {
        auto next = std::next(first, dist >> THREAD_EXP);
        t.emplace_back(std::thread(f, first, next, std::ref(s[i]), std::ref(args)...));
        first = next;
    }
    t.emplace_back(std::thread(f, first, last, std::ref(s.back()), std::ref(args)...));

    t.front().join();
    ret = std::move(s[0]);
    for (size_t i = 1; i < t.size(); ++i) {
        t[i].join();
        u(ret, s[i]);
    }
}

template<typename Iterator, typename F, typename URet, typename U, typename... Args>
void parallel_calc(F&& f, U&& u, Iterator first, Iterator last, URet& ret, Args&&... args) {
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    parallel_calc_impl(std::forward<F>(f), std::forward<U>(u), first, last, ret, category(), std::forward<Args>(args)...);
}

template<typename InputIt>
void count_impl(InputIt first, InputIt last, std::vector<size_t>& store) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;

    while (first != last) {
        auto reintr_ptr = reinterpret_cast<uint8_t const*>(&(*first++));
        for (size_t i = 0; i < sizeof(value_type); ++i) {
            ++store[reintr_ptr[i]];
        }
    }
}

template<typename InputIt>
void encode_impl(InputIt first, InputIt last, std::string& ret, bitset const* bs) {
    using value_type = typename std::iterator_traits<InputIt>::value_type;
    static_assert(std::is_trivially_copyable_v<value_type>);

    bitset bsret;
    uint32_t encoded = 0;

    while (first != last) {
        auto reintr_ptr = reinterpret_cast<uint8_t const*>(&(*first++));
        for (size_t i = 0; i < sizeof(value_type); ++i) {
            ++encoded;
            bsret.append(bs[reintr_ptr[i]]);
        }
    }

    ret = std::string(HEADER_SIZE, '0');
    ret.append(bsret.begin(), bsret.end());

    write_binary_(encoded, ret.begin() + HASH_SIZE_BYTES);
    uint32_t hash = crc32(ret.begin() + HASH_SIZE_BYTES, ret.end());
    write_binary_(hash, ret.begin());
}

template<typename Iterator>
void parallel_count(Iterator first, Iterator last, std::vector<size_t>& store) {
    parallel_calc(count_impl<Iterator>,
            [](std::vector<size_t>& dst, std::vector<size_t> const& src) {
                std::transform(dst.begin(), dst.end(), src.begin(), dst.begin(), std::plus<>());
            },
            first, last, store);
}

template<typename Iterator>
void parallel_encode(Iterator first, Iterator last, std::string& ret, bitset* bs) {
    parallel_calc(encode_impl<Iterator>,
            [](std::string& dst, std::string const& src){ dst += src; },
            first, last, ret, bs);
}

#endif /* HUFFMAN_UTIL_HPP */
