//
//  author dzhiblavi
//

#include "encoder.hpp"

namespace hfm {
bool operator<(fcounter::smb const& a, fcounter::smb const& b) {
    return a.cnt < b.cnt;
}

fcounter::fcounter() {
    for (size_t i = 0; i < ALPH_SIZE; ++i) {
        freq_[(uint8_t) i].symb = i;
    }
}

fcounter::smb const* fcounter::freq() const {
    return freq_;
}

fcounter::smb* fcounter::freq() {
    return freq_;
}

bool tree::less_(node_ptr a, node_ptr b, node_ptr c, node_ptr d) {
    if (!a || !b) {
        return false;
    }
    if (!c || !d) {
        return true;
    }
    return a->f + b->f <= c->f + d->f;
}

void tree::build_tree_(fcounter const& fc) {
    auto i = std::upper_bound(fc.freq(), fc.freq() + ALPH_SIZE, fcounter::smb{0, 0});
    size_t size = fc.freq() + ALPH_SIZE - i;
    if (!size) {
        root = new node {};
        return;
    }
    std::vector<node_ptr> q1(size), q2(size);
    for (size_t j = 0; j < size; ++j) {
        q1[j] = new node {(i + j)->cnt, nullptr, nullptr, (i + j)->symb};
    }
    size_t i1 = 0, i2 = 0;
    for (size_t k = 0; k < size - 1; ++k) {
        node_ptr q11 = i1 < size ? q1[i1] : nullptr;
        node_ptr q12 = i1 + 1 < size ? q1[i1 + 1] : nullptr;
        node_ptr q21 = i2 < k ? q2[i2] : nullptr;
        node_ptr q22 = i2 + 1 < k ? q2[i2 + 1] : nullptr;
        if (less_(q11, q12, q21, q22) && less_(q11, q12, q11, q21)) {
            q2[k] = new node {q11->f + q12->f, q11, q12, 0};
            i1 += 2;
        } else if (less_(q11, q21, q11, q12) && less_(q11, q21, q21, q22)) {
            q2[k] = new node {q11->f + q21->f, q11, q21, 0};
            ++i1;
            ++i2;
        } else {
            q2[k] = new node {q21->f + q22->f, q21, q22, 0};
            i2 += 2;
        }
    }
    root = q2[i2] ? q2[i2] : new node {q1[i1]->f, q1[i1], new node {}, 0};
}

void tree::calc_code_(node_ptr p, bitset& current_code_bitset,
        bitset& tree_alphabet_bitset, bitset& tree_code_bitset, size_t& cnt) {
    if (!p) {
        return;
    }
    if (!p->l) {
        char_by_id_[cnt] = p->c;
        p->id = cnt++;

        tree_code_bitset.push(0);
        tree_alphabet_bitset.append(bitset::from_uint8_t((uint8_t)p->c));

        alph_map_[(uint8_t)p->c] = current_code_bitset;
    } else {
        tree_code_bitset.push(1);
        current_code_bitset.push(0);
        calc_code_(p->l, current_code_bitset, tree_alphabet_bitset, tree_code_bitset, cnt);
        current_code_bitset.flip(current_code_bitset.size() - 1);
        calc_code_(p->r, current_code_bitset, tree_alphabet_bitset, tree_code_bitset, cnt);
        current_code_bitset.pop();
    }
}

bool tree::header_initialized_() const {
    return header_cnt == HEADER_SIZE;
}

tree::node_ptr tree::decode_(node_ptr v, uint8_t x, uint8_t left) {
    if (v->id != -1) {
        decoded_.push_back(char_by_id_[v->id]);
        v = root;
        if (!--count) {
            return v;
        }
    }
    if (!left) {
        return v;
    }
    if (x & (1 << (left - 1))) {
        return decode_(v->r, x, left - 1);
    }
    return decode_(v->l, x, left - 1);
}

void tree::terminate_(node_ptr p) {
    if (!p) {
        return;
    }
    terminate_(p->l);
    terminate_(p->r);
    delete p;
}

void tree::trace_(node_ptr p, size_t d) const {
    if (!p) {
        return;
    }
    trace_(p->l, d + 1);
    for (size_t i = 0; i < d; ++i) {
        std::cout << ' ';
    }
    std::cout << "(" << (p->id != -1 ? char_by_id_[p->id] : ' ') <<  ")\n";
    trace_(p->r, d + 1);
}

tree::tree(fcounter const& fcc) {
    fcounter fc(fcc);
    std::sort(fc.freq(), fc.freq() + ALPH_SIZE);
    build_tree_(fc);

    size_t cnt = 0;
    bitset tree_code_bitset, tree_alphabet_bitset, current_code_bitset;
    calc_code_(root, current_code_bitset, tree_alphabet_bitset, tree_code_bitset, cnt);

    tree_code_ = std::string(HEADER_SIZE, '\0');
    tree_code_.append(tree_code_bitset.begin(), tree_code_bitset.end());
    tree_code_.append(tree_alphabet_bitset.begin(), tree_alphabet_bitset.end());

    write_binary_((uint32_t) (tree_code_.size() - HEADER_SIZE), tree_code_.begin() + HASH_SIZE_BYTES);
    uint32_t hashh = crc32(tree_code_.begin(), tree_code_.end());
    write_binary_(hashh, tree_code_.begin());
    tree_ok = true;
}

tree::~tree() {
    terminate_(root);
}

bool tree::read_finished_success() const {
    return ((hash ^ CRCMASK) == expected_hash) && !count;
}

void tree::check_block_hash_() const {
    if ((hash ^ CRCMASK) != expected_hash) {
        throw std::runtime_error("corrupted file : incorrect block hash sum");
    }
}

size_t tree::gcount() const {
    return last_read;
}

void tree::free_tree() {
    terminate_(root);
    cur_restore = root = nullptr;
    count = header_cnt = hash = expected_hash = 0;
    alphabet_restore_left = -1;
    decoded_.clear();
    tree_code_.clear();
}

std::string const& tree::encode() const {
    return tree_code_;
}

void tree::clear() {
    decoded_.clear();
}

bitset const& tree::encode(char c) const {
    return alph_map_[(uint8_t)c];
}

size_t tree::chars_left() const {
    return decoded_.size();
}

void tree::trace() const {
    trace_(root);
}
} // namespace hfm