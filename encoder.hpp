//
//  author dzhiblavi
//

#ifndef HUFFMAN_ENCODER_H_
#define HUFFMAN_ENCODER_H_

#include <iostream>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

#include "bitset.hpp"
#include "util.hpp"

namespace hfm {
class fcounter {
public:
    struct smb {
        char symb = 0;
        size_t cnt = 0;

        friend bool operator<(smb const& a, smb const& b);
    };

private:
    smb freq_[ALPH_SIZE];

public:
    fcounter();

    template<typename InputIt>
    void update(InputIt first, std::enable_if_t<carries_trivially_copyable_v<InputIt>, InputIt> last) {
        std::vector<size_t> frc(256);
        parallel_count(first, last, frc);
        std::transform(freq_, freq_ + 256, frc.begin(), freq_, [](smb a, size_t b){ a.cnt += b; return a;});
    }

    smb const* freq() const;
    smb* freq();
};

class tree {
    struct node {
        size_t f;
        node *l, *r;
        char c;
        int id = -1;
        node* p;
        bool isr;
    };
    using node_ptr = node*;

    bitset alph_map_[ALPH_SIZE];
    char char_by_id_[ALPH_SIZE];
    std::string tree_code_;

    node_ptr root = nullptr;
    node_ptr cur_restore = nullptr;
    std::vector<uint8_t> decoded_;

    uint32_t alphabet_restore_left = -1;
    uint32_t alph_id = 0;
    uint32_t vertex_id = 0;
    uint32_t last_read = 0;

    uint32_t header_cnt = 0;
    uint32_t hash = 0;
    uint32_t count = 0;
    uint32_t expected_hash = 0;

    static bool less_(node_ptr a, node_ptr b, node_ptr c, node_ptr d);
    void build_tree_(fcounter const& fc);
    void calc_code_(node_ptr p, bitset& current_code_bitset,
            bitset& tree_alphabet_bitset, bitset& tree_code_bitset, size_t& cnt);

    bool header_initialized_() const;
    void check_block_hash() const;

    node_ptr decode_(node_ptr v, uint8_t x, uint8_t left);
    void trace_(node_ptr p, size_t d = 0) const;
    static void terminate_(node_ptr p);

    template <typename InputIt>
    InputIt parse_header_(InputIt first, InputIt last) {
        if (!header_cnt) {
            expected_hash = 0;
        }
        for (; header_cnt < HASH_SIZE_BYTES && first != last; ++header_cnt) {
            expected_hash += (uint64_t)convert_to_byte(first++) << (8 * header_cnt);
        }
        if (header_cnt == HASH_SIZE_BYTES) {
            hash = CRCMASK;
        }
        for (; !header_initialized_() && first != last; ++header_cnt) {
            hash = crc32_hash(hash, convert_to_byte(first));
            count += convert_to_byte(first++) << (8 * (header_cnt - HASH_SIZE_BYTES));
        }
        return first;
    }

    template <typename InputIt>
    InputIt restore_tree_(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            for (size_t i = 0; i < 8; ++i) {
                if (convert_to_byte(first) & (1ull << (7 - i))) {
                    auto new_node = new node {228, nullptr, nullptr, 0, -1, cur_restore, false};
                    cur_restore->l = new_node;
                    cur_restore = cur_restore->l;
                } else {
                    ++alphabet_restore_left;
                    cur_restore->id = vertex_id++;
                    while (cur_restore->isr) {
                        cur_restore = cur_restore->p;
                    }
                    if (cur_restore == root) {
                        return ++first;
                    } else {
                        auto new_node = new node {228, nullptr, nullptr, 0, -1, cur_restore->p, true};
                        cur_restore = cur_restore->p;
                        cur_restore->r = new_node;
                        cur_restore = cur_restore->r;
                    }
                }
            }
        }
        return first;
    }

    template <typename InputIt>
    InputIt restore_alphabet(InputIt first, InputIt last) {
        while (first != last && alphabet_restore_left--) {
            char_by_id_[alph_id++] = convert_to_byte(first++);
        }
        return first;
    }

public:
    struct encoding_policy {};
    struct single_block : encoding_policy {};
    struct any_block : encoding_policy {};

    tree() = default;
    explicit tree(fcounter const& fcc);
    tree& operator=(tree const&) = delete;
    tree(tree const&) = delete;
    ~tree();

    void free_tree();
    void clear();
    size_t chars_left() const;
    void trace() const;
    size_t gcount() const;
    bool read_finished_success() const;

    std::string const& encode() const;
    bitset const& encode(char c) const;

    template <typename InputIt>
    std::string encode(any_block, InputIt first, InputIt last) {
        std::string ret;
        parallel_encode(first, last, ret, alph_map_);
        return ret;
    }

    template <typename InputIt>
    std::string encode(single_block, InputIt first, InputIt last) {
        std::string ret;
        encode_impl(first, last, ret, alph_map_);
        return ret;
    }

    template <typename InputIt>
    std::string encode(InputIt first, InputIt last) {
        return encode(any_block(), first, last);
    }

    template <typename InputIt>
    InputIt initialize_tree(InputIt first, std::enable_if_t<carries_trivially_copyable_v<InputIt>, InputIt> last) {
        if ((header_initialized_() && !header_cnt) || first == last) {
            return first;
        }
        if (!header_initialized_()) {
            first = parse_header_(first, last);
            if (header_initialized_()) {
                tree_code_ = std::string(HEADER_SIZE, '\0');
                write_binary_(count, tree_code_.begin() + HASH_SIZE_BYTES);
                alph_id = vertex_id = 0;
                terminate_(root);
                root = new node {0, nullptr, nullptr};
                cur_restore = root;
                alphabet_restore_left = 0;
            }
        }
        if (first == last) {
            return first;
        }
        while (first != last && count) {
            --count;
            tree_code_ += convert_to_byte(first++);
        }
        if (!count) {
            hash = crc32(tree_code_.begin(), tree_code_.end());
            write_binary_(hash, tree_code_.begin());
            if (hash != expected_hash || !tree_code_.size()) {
                throw std::runtime_error("corrupted file : incorrect tree hash sum");
            }

            auto st = restore_tree_(tree_code_.begin() + HEADER_SIZE, tree_code_.end());
            restore_alphabet(st, tree_code_.end());
            count = header_cnt = hash = expected_hash = 0;
        }
        return first;
    }

    template <typename InputIt>
    void prepare(InputIt first, std::enable_if_t<carries_trivially_copyable_v<InputIt>, InputIt> last) {
        while (first != last) {
            if (header_initialized_()) {
                if (!count) {
                    header_cnt = 0;
                    check_block_hash();
                    first = parse_header_(first, last);
                    cur_restore = root;
                } else {
                    hash = crc32_hash(hash, convert_to_byte(first));
                    cur_restore = decode_(cur_restore, convert_to_byte(first++), 8);
                }
            } else {
                first = parse_header_(first, last);
                cur_restore = root;
            }
        }
    }

    template <typename OutputIt>
    OutputIt decode(OutputIt first, std::enable_if_t<carries_trivially_copyable_v<OutputIt>, OutputIt> last) {
        using value_type = typename std::iterator_traits<OutputIt>::value_type;

        last_read = 0;
        size_t ind = 0;
        auto i = decoded_.begin();
        while (first != last && i != decoded_.end()
            && ind + sizeof(value_type) <= decoded_.size()) {
            ++last_read;
            std::copy(i, i + sizeof(value_type), reinterpret_cast<uint8_t *>(&(*first)));
            i += sizeof(value_type);
            ind += sizeof(value_type);
            ++first;
        }
        return first;
    }
};
} /* namespace hfm */

#endif /* HUFFMAN_ENCODER_H_ */













