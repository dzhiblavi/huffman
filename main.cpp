/*
    author dzhiblavi
 */

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>

#include "testing.hpp"

#include "encoder.hpp"
#include "bitset.hpp"
#include "util.hpp"

#define BUFF_SIZE 4096000
#define DECODE_BUFF_SIZE 128000

test::rndgen rnd(1, 10000);

bitset gen_bitset(size_t size) {
    bitset ret(size);
    for (size_t i = 0; i < size; ++i) {
        ret.set(i, rnd.rand() & 1);
    }
    return ret;
}

std::string gen_string(size_t size) {
    std::string ret(size, '\0');
    std::for_each(ret.begin(), ret.end(), [](auto &c) {
        c = char('a' + rnd.rand() % 26);
    });
    return ret;
}

template<typename T>
std::vector<T> gen_vector(size_t size) {
    std::vector<T> ret(size);
    std::for_each(ret.begin(), ret.end(), [](T &x) {
        x = T(rnd.rand());
    });
    return ret;
}

void simple_bitset_test() {
    bitset b1(10), b2(13);
    test::check_equal(b1.to_string(), "0000000000");
    test::check_equal(b2.to_string(), "0000000000000");
    b1.set(2, 1);
    b1.set(4, 1);
    b1.set(5, 1);
    b1.set(8, 1);
    b2.set(0, 1);
    b2.set(1, 1);
    b2.set(2, 1);
    b2.set(4, 1);
    b2.set(7, 1);
    b2.set(8, 1);
    b2.set(10, 1);
    b2.set(12, 1);
    test::check_equal(b1.to_string(), "0010110010");
    test::check_equal(b2.to_string(), "1110100110101");
    b1.append(b2);
    test::check_equal(b1.to_string(), "00101100101110100110101");
    test::check_equal(b2.to_string(), "1110100110101");
    b2.append(b1);
    test::check_equal(b1.to_string(), "00101100101110100110101");
    test::check_equal(b2.to_string(), "111010011010100101100101110100110101");
    b1.append(b2);
    test::check_equal(b1.to_string(), "00101100101110100110101111010011010100101100101110100110101");
    test::check_equal(b2.to_string(), "111010011010100101100101110100110101");
    b2.append(b1);
    test::check_equal(b1.to_string(), "00101100101110100110101111010011010100101100101110100110101");
    test::check_equal(b2.to_string(), "11101001101010010110010111010011010100101100101110100110101"
                                      "111010011010100101100101110100110101");
}

void set_bitset_test() {
    std::string s;
    s.resize(1000, '0');
    bitset bs(1000);
    test::check_equal(bs.to_string(), s);
    for (size_t ch = 0; ch < 10000; ++ch) {
        size_t pos = rnd.rand() % 1000;
        uint8_t bit = rnd.rand() & 1;
        s[pos] = char('0' + bit);
        bs.set(pos, bit);
        test::check_equal(bs.to_string(), s);
    }
}

void bitset_push_test() {
    std::string repr;
    bitset bs;
    for (size_t i = 0; i < 1000; ++i) {
        size_t bit = rnd.rand() % 2;
        repr += char('0' + bit);
        bs.push(bit);
        test::check_equal(repr, bs.to_string());
    }
}

void bitset_push_pop_test() {
    std::string repr;
    bitset bs;
    for (size_t i = 0; i < 10000; ++i) {
        size_t choose = rnd.rand() & 1;
        if (choose && bs.size()) {
            bs.pop();
            repr.resize(repr.size() - 1);
            test::check_equal(repr, bs.to_string());
            test::check_equal(bs.size(), repr.size());
        } else {
            size_t bit = rnd.rand() & 1;
            bs.push(bit);
            repr += char('0' + bit);
            test::check_equal(repr, bs.to_string());
            test::check_equal(bs.size(), repr.size());
        }
    }
}

void complex_bitset_test() {
    for (size_t arg1 = 1; arg1 < 100; ++arg1) {
        for (size_t arg2 = 1; arg2 < 100; ++arg2) {
            bitset a1 = gen_bitset(arg1);
            bitset a2 = gen_bitset(arg2);
            std::string a1s = a1.to_string();
            std::string a2s = a2.to_string();
            a1.append(a2);
//            std::cout << "=> " << a1s << ' ' << a2s << "\n   " << a1.to_string() << '\n';
            test::check_equal(a1.to_string(), a1s + a2s);
            test::check_equal(a2.to_string(), a2s);
        }
    }
    bitset a1 = gen_bitset(10);
    bitset a2 = gen_bitset(10);
    std::string a1s = a1.to_string();
    std::string a2s = a2.to_string();
    for (size_t i = 0; i < 10; ++i) {
        a1.append(a2);
        a2.append(a1);
        a1s += a2s;
        a2s += a1s;
        test::check_equal(a1s, a1.to_string());
        test::check_equal(a2s, a2.to_string());
    }
}

template<typename InputIt>
void test_fcounter(hfm::fcounter &fc, hfm::fcounter::smb *freq,
                   InputIt first, InputIt last) {
    fc.update(first, last);
    std::for_each(first, last, [& freq](auto &c) { ++freq[(uint8_t) c].cnt; });
    test::check_equal(true, std::equal(freq, freq + ALPH_SIZE,
                                       fc.freq(), fc.freq() + ALPH_SIZE,
                                       [](auto &a, auto &b) {
                                           return a.cnt == b.cnt;
                                       }));
}

void simple_fcounter_test() {
    hfm::fcounter fc;
    std::string s = "abcdefgabababababababfffff";
    hfm::fcounter::smb freq[ALPH_SIZE];
    test_fcounter(fc, freq, s.begin(), s.end());
}

void complex_fcounter_test() {
    hfm::fcounter fc;
    hfm::fcounter::smb freq[ALPH_SIZE];
    for (size_t nt = 0; nt < 1000; ++nt) {
        std::string add = gen_string(1000);
        test_fcounter(fc, freq, add.begin(), add.end());
    }
}

void simple_tree_test() {
    hfm::fcounter fc;
    std::string s = "abcdefgabababababababfffff";
    fc.update(s.begin(), s.end());
    hfm::tree ht(fc);
//    for (size_t c = 0; c < 255; ++c) {
//        std::cout << "=> " << (char)c << ' ' << ht.encode(c).to_string() << '\n';
//    }
//    for (char c : ht.encode()) {
//        std::cout << (int)(uint8_t)c << ' ';
//    }

    hfm::fcounter fc2;
    std::string s2 = "abcdefasldkjfaasgsdhdfghbviojboiuoisuaoiuoutojgskdjfnbcxvnbkwejtpoiupasoifgjljalkdsfgabababababababfffff";
    fc2.update(s2.begin(), s2.end());
    hfm::tree ht2(fc2);
//    for (size_t c = 0; c < 255; ++c) {
//        std::cout << "=> " << c << ' ' << ht2.encode(c).to_string() << '\n';
//    }
//    std::cout << ht2.encode() << '\n';
}

void stress_trace_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(10000);
        hfm::fcounter fc;
        fc.update(s.begin(), s.end());
        hfm::tree ht(fc);
    }
}

void rtree_string_test(std::string s) {
    hfm::fcounter fc;
    fc.update(s.begin(), s.end());
    hfm::tree ht(fc);

    std::string code = ht.encode(s.begin(), s.end());

//    for (char c : code) {
//        std::cout << (int)(uint8_t)(c) << ' ';
//    }
//    std::cout << '\n';
//    for (int c = 0; c < 256; ++c) {
//        if (ht.encode((char)c).to_string().size()) {
//            std::cout << (char) c << ' ' << ht.encode((char) c).to_string() << '\n';
//        }
//    }

    ht.prepare(code.begin(), code.end());

    std::string encoded(ht.chars_left(), 'q');
    ht.decode(encoded.begin(), encoded.end());

//    std::cout << s << '\n' << encoded << '\n';
    test::check_equal(s, encoded);
}

void simple_empty_rtree_test() {
    rtree_string_test("");
}

void simple_one_rtree_test() {
    rtree_string_test("a");
    rtree_string_test("aa");
    rtree_string_test("ab");
}

void simple_rtree_test() {
    std::string s = "abracadabrac";
    std::string ss = "ibragim dzhiblavi";
    hfm::fcounter fc;
    fc.update(s.begin(), s.end());
    fc.update(ss.begin(), ss.end());
    hfm::tree ht(fc);

    std::string code = ht.encode(s.begin(), s.end()); // only block
    code += ht.encode(ss.begin(), ss.end());
//    for (char c : code) {
//        std::cout << (int)(uint8_t)(c) << ' ';
//    }
//    std::cout << '\n';

    ht.prepare(code.begin(), code.end());
//    std::cout << ht.chars_left() << '\n';

    std::string encoded(ht.chars_left(), 'q');
    ht.decode(encoded.begin(), encoded.end());

    test::check_equal(s + ss, encoded);
}

void complex_rtree_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(2228 + rnd.rand() % 1337);
        hfm::fcounter fc;
        fc.update(s.begin(), s.end());
        hfm::tree ht(fc);

        std::string code = ht.encode(s.begin(), s.end());
        ht.prepare(code.begin(), code.end());
        std::string encoded(ht.chars_left(), 'q');
        ht.decode(encoded.begin(), encoded.end());

        if (!ht.read_finished_success()) {
            throw std::runtime_error("read failed");
        }

//        std::cout << s << '\n' << encoded << '\n';
        test::check_equal(s, encoded);
    }
}

void tree_restore_test(std::string &&s) {
    hfm::fcounter fc;
    fc.update(s.begin(), s.end());
    hfm::tree ht(fc);
    std::string code = ht.encode() + ht.encode(s.begin(), s.end());
    hfm::tree reht;
    auto st = reht.initialize_tree(code.begin(), code.end());
//    reht.trace();
    reht.prepare(st, code.end());
    std::string encoded(reht.chars_left(), 'q');
    reht.decode(encoded.begin(), encoded.end());

//    std::cout << s << '\n' << encoded << '\n';
    test::check_equal(s, encoded);
}

void simple_tree_restore_test() {
    tree_restore_test("abracadabrac");
}

void simple_tree_empty_restore_test() {
    tree_restore_test("");
}

void simple_tree_one_restore_test() {
    tree_restore_test("a");
    tree_restore_test("aa");
    tree_restore_test("aaa");
    tree_restore_test("ab");
    tree_restore_test("abb");
}

void complex_tree_restore_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(10000);
        tree_restore_test(std::move(s));
    }
}

void megahard_discrete_load_tree_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(10000);
        size_t ind = 0;

        hfm::fcounter fc;
        fc.update(s.begin(), s.end());
        hfm::tree ht(fc);

        std::string code;

        while (ind < s.size()) {
            size_t add = rnd.rand() % (s.size() - ind) % 228;
            if (!add) add = 1;
            code += ht.encode(s.begin() + ind, s.begin() + ind + add);
            ind += add;
        }

        ht.prepare(code.begin(), code.end());
        std::string encoded(ht.chars_left(), 'q');
        ht.decode(encoded.begin(), encoded.end());

//        std::cout << s << '\n' << encoded << '\n';
        test::check_equal(s, encoded);
    }
}

void megahard_discrete_decode_tree_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(10000);

        hfm::fcounter fc;
        fc.update(s.begin(), s.end());
        hfm::tree ht(fc);

        std::string code = ht.encode() + ht.encode(s.begin(), s.end()), encoded;

        hfm::tree decoder;
        auto st = decoder.initialize_tree(code.begin(), code.end());

        char buff[std::max(10000, DECODE_BUFF_SIZE << 3)];

        while (st != code.end()) {
            size_t add = rnd.rand() % (code.end() - st) % 228;
            if (!add) add = 1;

            decoder.prepare(st, st + add);

            decoder.decode(buff, buff + decoder.chars_left());
            encoded.append(buff, buff + decoder.chars_left());

            decoder.clear();
            st += add;
        }

//        std::cout << s << '\n' << encoded << '\n';
        test::check_equal(s, encoded);
    }
}

std::string partial_load(std::string const &s) {
    hfm::fcounter fc;
    fc.update(s.begin(), s.end());
    hfm::tree ht(fc);

    std::string code = ht.encode();
    size_t ind = 0;

    while (ind < s.size()) {
        size_t add = rnd.rand() % (s.size() - ind) % 228;
        if (!add) add = 1;
        code += ht.encode(s.begin() + ind, s.begin() + ind + add);
        ind += add;
    }
    return code;
}

std::string partial_decode(std::string const &code) {
    hfm::tree decoder;
    std::string encoded;
    auto st = decoder.initialize_tree(code.begin(), code.end());

    char buff[std::max(1000, DECODE_BUFF_SIZE << 4)];

    while (st != code.end()) {
        size_t add = rnd.rand() % (code.end() - st) % 228;
        if (!add) add = 1;

        decoder.prepare(st, st + add);

        memset(buff, 0, DECODE_BUFF_SIZE);
        decoder.decode(buff, buff + decoder.chars_left());
        encoded.append(buff, buff + decoder.chars_left());

        decoder.clear();
        st += add;
    }
    if (!decoder.read_finished_success()) {
        throw std::runtime_error("read failed");
    }
    return encoded;
}

std::string encode_faulty(std::string const &s, int fault_inject) {
    hfm::fcounter fc;
    fc.update(s.begin(), s.end());
    hfm::tree ht(fc);
    std::string code = ht.encode();
    std::string cd = ht.encode(s.begin(), s.end());
    if (fault_inject) {
        size_t pos = rnd.rand() % code.size();
        code[pos] += 1;
        if (rnd.rand() & 1) {
            code += char(rnd.rand() & UINT8_MAX);
        }
    } else {
        size_t pos = rnd.rand() % cd.size();
        cd[pos] += 1;
        if (rnd.rand() & 1) {
            cd += char(rnd.rand() & UINT8_MAX);
        }
    }
    return code + cd;
}

void megahard_full_tree_test() {
    for (size_t i = 0; i < 100; ++i) {
        std::string s = gen_string(10000);
        test::check_equal(partial_decode(partial_load(s)), s);
    }
}

struct Deleter {
    void operator()(char *p) {
        operator delete(p);
    }
};

void encode_file_faulty(char const *in_file, char const *out_file, int fault_inject, bool verbose = true) {
    std::ifstream file;
    file.open(in_file);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }

    hfm::fcounter fc;

    auto buff = static_cast<char *>(operator new(BUFF_SIZE));
    std::unique_ptr<char, Deleter> uniq(buff);

    size_t count = 0;
    auto stp = std::chrono::high_resolution_clock::now();

    while (!file.eof()) {
        file.read(buff, BUFF_SIZE);
        fc.update(buff, buff + file.gcount());
        count += file.gcount();
    }
    file.close();

    hfm::tree ht(fc);

    file.open(in_file);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }
    std::ofstream ofs;
    ofs.open(out_file);
    if (!ofs) {
        throw std::runtime_error("failed to open output file");
    }

    auto code = ht.encode();
    if (fault_inject == 1) {
        code[rnd.rand() % code.size()] = rnd.rand() & UINT8_MAX;
    }
    if (fault_inject && (rnd.rand() & 1)) {
        code += char(rnd.rand() & UINT8_MAX);
    }
    ofs.write(code.data(), code.size());
    code.clear();

    while (!file.eof()) {
        file.read(buff, BUFF_SIZE);

        code = ht.encode(buff, buff + file.gcount());

        if (fault_inject == 2) {
            code[rnd.rand() % code.size()] = rnd.rand() & UINT8_MAX;
        }
        if (fault_inject && (rnd.rand() & 1)) {
            code += char(rnd.rand() & UINT8_MAX);
        }

        ofs.write(code.data(), code.size());

        code.clear();
        ht.clear();
    }

    if (verbose) {
        std::chrono::duration<double> dur = std::chrono::high_resolution_clock::now() - stp;
        std::cout << "encoding speed : " << (size_t) (1.0f * count / dur.count()) / 1000000.0f << " Mb/sec\n";
    }

    file.close();
    ofs.close();
}

void decode_file(char const *in_file, char const *out_file, bool verbose = true) {
    std::ifstream file;
    file.open(in_file);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }
    std::ofstream ofs;
    ofs.open(out_file);
    if (!file) {
        throw std::runtime_error("failed to open output file");
    }
    char buff[std::max(1000, DECODE_BUFF_SIZE << 3)];
    hfm::tree ht;
    size_t count = 0;
    auto stp = std::chrono::high_resolution_clock::now();

    while (!file.eof()) {
        file.read(buff, DECODE_BUFF_SIZE);
        auto p = ht.initialize_tree(buff, buff + file.gcount());
        count += file.gcount();
        if (p != buff + file.gcount()) {
            ht.prepare(p, buff + file.gcount());
            ht.decode(buff, buff + ht.chars_left());
            ofs.write(buff, ht.chars_left());
            ht.clear();
            break;
        }
    }
    while (!file.eof()) {
        file.read(buff, DECODE_BUFF_SIZE);
        ht.prepare(buff, buff + file.gcount());
        ht.decode(buff, buff + ht.chars_left());
        ofs.write(buff, ht.chars_left());
        ht.clear();
        count += file.gcount();
    }

    if (!ht.read_finished_success()) {
        throw std::runtime_error("read failed");
    }

    if (verbose) {
        std::chrono::duration<double> dur = std::chrono::high_resolution_clock::now() - stp;
        std::cout << "decoding speed : " << (size_t) (1.0f * count / dur.count()) / 1000000.0f << " Mb/sec\n";
    }

    file.close();
    ofs.close();
}

bool compare_files(char const *f1, char const *f2) {
    std::ifstream ff1, ff2;
    ff1.open(f1);
    ff2.open(f2);
    char buff1[DECODE_BUFF_SIZE], buff2[DECODE_BUFF_SIZE];
    while (!ff1.eof() && !ff2.eof()) {
        memset(buff1, 0, DECODE_BUFF_SIZE);
        ff1.read(buff1, DECODE_BUFF_SIZE);
        memset(buff2, 0, DECODE_BUFF_SIZE);
        ff2.read(buff2, DECODE_BUFF_SIZE);
        if (ff1.gcount() != ff2.gcount()) {
            return false;
        }
        if (!std::equal(buff1, buff1 + ff1.gcount(), buff2)) {
            return false;
        }
    }
    return !(!ff1.eof() || !ff2.eof());
}

void faulty_encode_decode_test(bool header) {
    std::string s = gen_string(100000);
    std::string faulty_code = encode_faulty(s, header);
    partial_decode(faulty_code); // must throw
}

void complex_data_faulty_test(bool fault) {
    auto data = gen_vector<uint64_t>(10000);
    hfm::fcounter fc;
    fc.update(data.begin(), data.end());
    hfm::tree enc(fc);
    std::string code = enc.encode() + enc.encode(data.begin(), data.end());
    if (fault) {
        ++code[rnd.rand() % code.size()];
    }
    hfm::tree decoder;
    auto st = decoder.initialize_tree(code.begin(), code.end());
    std::vector<uint64_t> encoded(data.size() << 1);

    decoder.prepare(st, code.end());
    auto en = decoder.decode(encoded.begin(), encoded.end());

    if (!decoder.read_finished_success()) {
        throw std::runtime_error("read failed");
    }

    test::check_equal(true, std::equal(encoded.begin(), en, data.begin()));
}

void file_check_test_fault(char const* in_file) {
    int fault = (rnd.rand() & 1) + 1;
    encode_file_faulty(in_file, "out.temp", fault, false);
    decode_file("out.temp", "check.txt", false); // must throw
}

int main(int argc, char *argv[]) {
    test::run_test("simple bitset test", simple_bitset_test);
    test::run_test("set bitset test", set_bitset_test);
    test::run_test("complex bitset test", complex_bitset_test);
    test::run_test("bitset push test", bitset_push_test);
    test::run_test("bitset push-pop test", bitset_push_pop_test);

    test::run_test("simple fcounter test", simple_fcounter_test);
    test::run_test("complex fcounter test", complex_fcounter_test);
    test::run_test("simple tree test", simple_tree_test);
    test::run_test("stress tree trace test", stress_trace_test);

    test::run_test("simple empty rtree test", simple_empty_rtree_test);
    test::run_test("simple one rtree test", simple_one_rtree_test);
    test::run_test("simple rtree test", simple_rtree_test);
    test::run_test("complex rtree test", complex_rtree_test);

    test::run_test("simple tree restore test", simple_tree_restore_test);
    test::run_test("simple tree empty restore test", simple_tree_empty_restore_test);
    test::run_test("simple tree one restore test", simple_tree_one_restore_test);
    test::run_test("complex tree restore test", complex_tree_restore_test);
    test::run_test("partial load tree test", megahard_discrete_load_tree_test);
    test::run_test("partial decode tree test", megahard_discrete_decode_tree_test);
    test::run_test("partial full tree test", megahard_full_tree_test);
    test::run_multitest_faulty("faulty decode test, header corrupt", 100, faulty_encode_decode_test, true);
    test::run_multitest_faulty("faulty decode test, block corrupt", 100, faulty_encode_decode_test, false);
    test::run_multitest("e/d complex data", 100, complex_data_faulty_test, false);
    test::run_multitest_faulty("e/d complex data faulty", 100, complex_data_faulty_test, true);

    test::run_test("encode large file without faults", encode_file_faulty, "../100mb.txt", "../encoded.hfm", 0, false);
    test::run_test("decode large file without faults", decode_file, "../encoded.hfm", "../decoded.txt", false);
    test::check_equal(true, compare_files("../100mb.txt", "../decoded.txt"));

    test::run_multitest_faulty("e/d large file with faults", 5, file_check_test_fault, "../100mb.txt");
    return 0;
}






































