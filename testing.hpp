//
//  author dzhiblavi
//

#ifndef TESTING_HPP_H_
#define TESTING_HPP_H_

#include <iostream>
#include <random>
#include <chrono>
#include <ctime>
#include <limits>

namespace test {
    void TEST_RUN(std::string const& name) { std::cerr << "\033[01;34m[RUN...] \033[0m" << name; }
    void TEST_RM(std::string const &name) { for (size_t i = 0; i < name.size() + 9; ++i) std::cout << '\b'; std::cout << std::flush; }
    void TEST_NOTE(std::string const& note) { std::cerr << "\033[01;36mN:\033[0m " << note << "\n"; }
    void TEST_WARN(std::string const& mess) { std::cerr << "\033[01;35mW:\033[0m " << mess << "\n"; }
    void TEST_ERR(std::string const& mess) { std::cerr << "\033[01;31mE:\033[0m " << mess << "\n"; }
    void TEST_OK(std::string const& name) { std::cerr << "\033[32m[  OK  ] \033[0m" << name << '\n'; }
    void TEST_FAIL(std::string const& name, std::string const& why) { std::cerr << "\033[31m[ FAIL ] \033[0m" << name  << " : " << why << '\n'; }
    void TEST_OK_FAULT(std::string const& name, std::string const& why) { std::cerr << "\033[32m[  OK  ] \033[0m" << name << " :\033[1;33m error expected \033[0m: " << why << '\n'; }
    void TEST_RMOK(std::string const& name) { TEST_RM(name); TEST_OK(name); }
    void TEST_RMOK_FAULT(std::string const& name, std::string const& why) { TEST_RM(name); TEST_OK_FAULT(name, why); }
    void TEST_RMFAIL(std::string const& name, std::string const& why) { TEST_RM(name); TEST_FAIL(name, why); }

    class timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> tp, start;

public:
    timer()
    : tp(std::chrono::high_resolution_clock::now())
    , start(std::chrono::high_resolution_clock::now()) {}

    double total() {
        std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
        return duration.count();
    }

    double reset() {
        std::chrono::time_point<std::chrono::high_resolution_clock> ntp = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = ntp - tp;
        tp = ntp;
        return duration.count();
    }
};

template<typename T, typename = typename std::enable_if<std::is_integral_v<T>>::type>
class rndgen {
    std::mt19937 gen;
    std::uniform_int_distribution<T> distr;
    
public:
    rndgen(T l, T h)
    : gen((int)time(nullptr)),
      distr(l, h) {}
    
    T rand() {
        return distr(gen);
    }
};

template<typename F, typename... Args>
auto execute(F&& f, Args&& ...args)
{ return f(std::forward<Args>(args)...); }

template<typename V1, typename V2>
void
check_equal(V1&& v1, V2&& v2) {
    if (std::forward<V1>(v1) != std::forward<V2>(v2))
        throw std::runtime_error("test::expect_equal : equality check failed");
}

template<typename V1, typename V2, typename Cmp>
void
check_equal(V1&& v1, V2&& v2, Cmp&& cmp) {
    if (!cmp(std::forward<V1>(v1), std::forward<V2>(v2)))
        throw std::runtime_error("test::expect_equal : equality check failed");
}

template<typename F1, typename F2, typename... Args>
void
expect_equal(F1&& f1, F2&& f2, Args&& ...args) { check_equal(f1(args...), f2(args...)); }

template<typename F1, typename F2, typename... Args, typename Cmp>
void
expect_equal(F1&& f1, F2&& f2, Args&& ...args, Cmp cmp) { check_equal(f1(args...), f2(args...), cmp); }

template<typename F, typename... Args>
double
measure_time(F&& f, Args&& ...args) {
    timer t;
    f(std::forward<Args>(args)...);
    return t.total();
}

template<typename F, typename ...Args>
void run_test(std::string test_name, F&& f, Args&&... args) {
    TEST_RUN(test_name);
    try {
        execute(std::forward<F>(f), std::forward<Args>(args)...);
        TEST_RMOK(test_name);
    } catch (std::exception const& e) {
        TEST_RMFAIL(test_name, e.what());
    }
}

template<typename F, typename... Args>
void run_fault_test(std::string test_name, F&& f, Args&&... args) {
    TEST_RUN(test_name);
    try {
        execute(std::forward<F>(f), std::forward<Args>(args)...);
        TEST_RMFAIL(test_name, "error expected");
    } catch (std::exception const& e) {
        TEST_RMOK_FAULT(test_name, e.what());
    }
}

template<typename Gen, typename... Args>
void run_multitest(std::string test_name, size_t launch, Gen&& g, Args&& ...args) {
    TEST_RUN(test_name);
    try {
        while (launch--) {
            g(args...);
        }
        TEST_RMOK(test_name);
    } catch (std::exception const& e) {
        TEST_RMFAIL(test_name, e.what());
    }
}

template<typename Gen, typename... Args>
void run_multitest_faulty(std::string test_name, size_t launch, Gen&& g, Args&& ...args) {
    TEST_RUN(test_name);
    while (launch--) {
        try {
            g(args...);
            TEST_RMFAIL(test_name, "error expected");
            return;
        } catch (std::exception const& e) {
            // ok, exception expected
        }
    }
    TEST_RMOK(test_name);
}
} // namespace test

#endif // TESTING_HPP_H_
