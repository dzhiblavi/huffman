/*
    author dzhiblavi
 */

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <any>
#include <fstream>

#include "testing.hpp"

#include "encoder.hpp"
#include "bitset.hpp"

#define BUFF_SIZE 128000
#define BIG_BUFF_SIZE 4096000

bool verbose = false;

std::string st = "\033[01;34m[RUN...] \033[0m[";
std::string curr_status;

void status_remove() {
    for (size_t i = 0; i < curr_status.size(); ++i) {
        std::cout << '\b';
    }
    std::cout << std::flush;
    curr_status = "";
}

void status(float perc) {
    auto parts = (size_t)(20.0f * perc);
    status_remove();
    curr_status = st;
    for (size_t i = 0; i < 20; ++i) {
        curr_status += (i <= parts ? "=" : " ");
    }
    curr_status += "], " + std::to_string(100 * perc).substr(0, 5) + "%";
    std::cout << curr_status << std::flush;
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

        status(1.0f * ncount / count);

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

    st = "\033[32m[  OK  ] \033[0m[";
    status(1.0f);
    std::cout << std::endl;
}

void decode_file(char const *in_file, char const *out_file) {
    std::ifstream file;
    file.open(in_file);
    file.seekg (0, file.end);
    size_t length = file.tellg();
    file.seekg (0, file.beg);
    if (!file) {
        throw std::runtime_error("failed to open input file");
    }
    std::ofstream ofs;
    ofs.open(out_file);
    if (!ofs) {
        throw std::runtime_error("failed to open output file");
    }
    char buff[std::max(BUFF_SIZE << 3, 1000)];
    hfm::tree ht;
    size_t count = 0;
    auto stp = std::chrono::high_resolution_clock::now();

    while (!file.eof()) {
        file.read(buff, BUFF_SIZE);
        auto p = ht.initialize_tree(buff, buff + file.gcount());
        count += file.gcount();
        status(1.0f * count / length);
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
        status(1.0f * count / length);
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

    st = "\033[32m[  OK  ] \033[0m[";
    status(1.0f);
    std::cout << std::endl;
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
    std::string output_file(i + 1 < argc ? argv[i + 1] : "out.txt");
    try {
        if (compress) {
            encode_file(input_file.c_str(), output_file.c_str());
        } else {
            decode_file(input_file.c_str(), output_file.c_str());
        }
    } catch (std::exception const& e) {
        status_remove();
        std::cerr << "\033[31m[ FAIL ] \033[0m : " << e.what() << '\n';
    }
    return 0;
}
