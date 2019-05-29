//
//  author dzhiblavi
//

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <any>
#include <fstream>

#include "encoder.hpp"
#include "bitset.hpp"

#define BUFF_SIZE 128000
#define BIG_BUFF_SIZE 4096000

static bool verbose = false;
char const ok_status[] = "\033[32m[  OK  ] \033[0m";
char const fail_status[] = "\033[31m[ FAIL ] \033[0m";
static char status[51] = "\033[01;34m[RUN...] \033[0m[                    ] 00.00%";
const size_t status_size = 51;
const size_t start_ind = 22, end_ind = 42;
const size_t perc_pos = 44;

void status_remove() {
    for (size_t i = 0; i < status_size; ++i) {
        std::cout << '\b';
    }
    std::cout << std::flush;
}

void show_status(float perc) {
    auto parts = (size_t) (20.0f * perc);
    status_remove();
    for (size_t i = start_ind; i < end_ind; ++i) {
        status[i] = (i - start_ind <= parts ? '=' : ' ');
    }
    perc *= 100;
    std::string num = std::to_string(perc);
    for (size_t i = 0; i <= 3 + (perc >= 10.f); ++i) {
        status[i + perc_pos + 1 - (perc >= 10.f)] = num[i];
    }
    std::cout << status << std::flush;
}

struct Deleter {
    void operator()(char* p) {
        operator delete(p);
    }
};

void encode_file(char const *in_file, char const *out_file) {
    std::ifstream file;
    file.open(in_file, std::ifstream::binary);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }

    hfm::fcounter fc;

    auto buff = static_cast<char*>(operator new(BIG_BUFF_SIZE));
    std::unique_ptr<char, Deleter> uniq(buff);

    size_t count = 0;
    auto stp = std::chrono::high_resolution_clock::now();

    while (!file.eof()) {
        file.read(buff, BIG_BUFF_SIZE);
        fc.update(buff, buff + file.gcount());
        count += file.gcount();
    }
    file.close();

    std::chrono::duration<double> counter_duration = std::chrono::high_resolution_clock::now() - stp;
    std::cout << "updating speed = " << 1.0f * count / counter_duration.count() / 1000000.0f << " Mb/sec\n";
    stp = std::chrono::high_resolution_clock::now();

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
    ofs.write(code.data(), code.size());
    code.clear();

    size_t ncount = 0;

    while (!file.eof()) {
        file.read(buff, BIG_BUFF_SIZE);
        ncount += file.gcount();
        code = ht.encode(buff, buff + file.gcount());
        ofs.write(code.data(), code.size());
        show_status(1.0f * ncount / count);
        code.clear();
        ht.clear();
    }
    status_remove();

    if (verbose) {
        std::chrono::duration<double> dur = std::chrono::high_resolution_clock::now() - stp;
        std::cout << "average encoding speed : " << (size_t) (1.0f * count / dur.count()) / 1000000.0f << " Mb/sec\n";
        std::cout << "symbols encoded : " << count << '\n' << "time elapsed : " << dur.count() << '\n';
    }

    file.close();
    ofs.close();

    show_status(1.0f);
}

void decode_file(char const *in_file, char const *out_file) {
    std::ifstream file;
    std::ofstream ofs;

    file.open(in_file);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }
    if (!ofs) {
        throw std::runtime_error("failed to open output file");
    }

    file.seekg (0, file.end);
    size_t length = file.tellg();
    file.seekg (0, file.beg);
    ofs.open(out_file);

    char buff[std::max(BUFF_SIZE << 3, 1000)];
    hfm::tree ht;
    size_t count = 0;
    auto stp = std::chrono::high_resolution_clock::now();

    while (!file.eof()) {
        file.read(buff, BUFF_SIZE);
        auto p = ht.initialize_tree(buff, buff + file.gcount());
        count += file.gcount();
        show_status(1.0f * count / length);
        if (p != buff + file.gcount()) {
            ht.prepare(p, buff + file.gcount());
            ht.decode(buff, buff + ht.chars_left());
            ofs.write(buff, ht.chars_left());
            ht.clear();
            break;
        }
    }
    while (!file.eof()) {
        file.read(buff, BUFF_SIZE);
        ht.prepare(buff, buff + file.gcount());
        ht.decode(buff, buff + ht.chars_left());
        ofs.write(buff, ht.chars_left());
        show_status(1.0f * count / length);
        ht.clear();
        count += file.gcount();
    }
    if (!ht.read_finished_success()) {
        throw std::runtime_error("decode failed");
    }
    status_remove();

    if (verbose) {
        std::chrono::duration<double> dur = std::chrono::high_resolution_clock::now() - stp;
        std::cout << "average decoding speed : " << (size_t) (1.0f * count / dur.count()) / 1000000.0f << " Mb/sec\n";
        std::cout << "symbols decoded : " << count << '\n' << "time elapsed : " << dur.count() << '\n';
    }

    file.close();
    ofs.close();

    show_status(1.0f);
}

int main(int argc, char *argv[]) {
    int i = 1;
    bool compress = false, decompress = false;
    std::vector<std::string> args;
    for (; i < argc; ++i) {
        args.emplace_back(argv[i]);
        if (args.back() == "-c") {
            compress = true;
        } else if (args.back() == "-dc") {
            decompress = true;
        } else if (args.back() == "--verbose") {
            verbose = true;
        } else {
            break;
        }
    }
    if (i >= argc || ((compress && decompress) || (!compress && !decompress))) {
        std::cerr << "usage : huffman <args...> <in> [out = out.txt], possible args : -c, -dc (either), --verbose\n";
        return 0;
    }
    std::string input_file(argv[i]);
    std::string output_file(i + 1 < argc ? argv[i + 1] : (compress ? "out.hfm" : "out.txt"));
    try {
        if (compress) {
            encode_file(input_file.c_str(), output_file.c_str());
        } else {
            decode_file(input_file.c_str(), output_file.c_str());
        }
        status_remove();
        std::cout << ok_status << '\n';
    } catch (std::exception const& e) {
        status_remove();
        std::cout << fail_status << " : " << e.what() << '\n';
    }
    return 0;
}
