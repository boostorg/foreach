///////////////////////////////////////////////////////////////////////////////
// foreach_perf.cpp
//
// Copyright 2004 John Torjo, 2005 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <vector>
#include <list>
#include <deque>
#include <algorithm>
#include <stdlib.h>
#include <sstream>
#include <time.h>
#include <fstream>
#include <iostream>
#include "../../../boost/foreach.hpp"


// contains a number, as a string
struct str_sequence {
    str_sequence(int i) { set(i); }
    void set(int i) {
        std::ostringstream out;
        out << i;
        str = out.str();
    }
    std::string str;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Test functions

// ... note: they all modify the value, somehow. calling this function twice with the same reference,
//     will modify the value twice (unlike something like void setzero(int & val) { val = 0; }

struct set_to_idx {
    set_to_idx() : idx(0) {}
    template< class T> void operator()(T & val) { val = idx++; }
private:
    int idx;
};

inline void multiply_by_2(int & i)                 { i *= 2; }
inline void multiply_by_max20(int & i)             { i *= rand() % 20; }
inline void add_one_char(str_sequence & s)         { s.str.resize(s.str.size() + 1); }
inline void add_10(int & i)                        { i += 10; } 
inline void noop_int(int &)                        {}
inline void noop_seq(str_sequence &)               {}
inline void set_to_len(str_sequence & s)           { 
    s.set(s.str.length() + 1); 
    s.str.resize(s.str.length() * 5, s.str[0]); 
}

struct set_to_az {
    set_to_az() : end('a') {}
    void operator()(str_sequence & s) {
        s.str.erase();
        for (char start = 'a'; start < end; ++start)
            s.str = start + s.str;
        if (++end == 'z') end = 'a';
    }
private:
    char end;
};


////////////////////////////////////////////////////////////////////////////////////////////////
// Runs the tests

// modify these to suit your needs (also, you can use command-line arguments)
int MAX_ELEMS_COUNT = 128000; // how many elements to add in each array?
int ELEMS_COUNT = 128000; // how many elements to add in each array? (is doubled, until reaches MAX_ELEMS_COUNT)
const int TESTS = 3; // how many tests

bool first_test_run = false;
int run_times_so_far = 0;
double sum_raw = 0;
double sum_each = 0;
double sum_each_percentage = 0;

std::ofstream results_file("result.txt");

template<class container>
void fill_container(container & c, int elems) {
    for (int idx = 0; idx < elems; ++idx) 
        c.push_back(rand() % 10000); // not too big numbers, we don't want overflows
}

template<class container, class test_func>
inline void run_test(const std::string & test_name, const container & , test_func f, int times, int elems) {
    container copy;
    fill_container(copy, elems);
    container c; // run all tests on exactly the same content of the container

    // use raw iterators
    c = copy; // run all tests on exactly the same content of the container
    clock_t start = clock();
    test_func call = f;
    for (int idx = 0; idx < times; ++idx)
        for (typename container::iterator b = c.begin(), e = c.end(); b != e; ++b)
            call(*b);
    clock_t end = clock();
    double raw_secs = (double)(end - start) / CLOCKS_PER_SEC;

    // use EricNiebler's for_each
    c = copy; // run all tests on exactly the same content of the container
    typedef typename container::value_type value;
    start = clock();
    call = f;
    for (int idx = 0; idx < times; ++idx)
        BOOST_FOREACH(value & v, c)
            call(v);
    end = clock();
    double each_secs = (double)(end - start) / CLOCKS_PER_SEC;

    // john.torjo - added after I've come up with the results
    // ... account for inaccurate clock
    if (each_secs < raw_secs) each_secs = raw_secs;

    std::ostringstream msg;
    msg << "\nTest " << test_name << " took "
        << "\n  RAW            : " << raw_secs << " secs";
    msg << "\n  foreach calls: " << each_secs << " secs";
    if (raw_secs > 0)
        msg << " (" << ((each_secs * 100) / raw_secs) << "%)";
    results_file << msg.str() << std::endl;
    std::cout << msg.str() << std::endl;

    // when computing averages, we're ignoring first test (we're just warming up ;))
    if (first_test_run && raw_secs > 0) {
        ++run_times_so_far;
        sum_raw += raw_secs;
        sum_each += each_secs;
        sum_each_percentage += ((each_secs * 100) / raw_secs);
    }
}


void test() {
    typedef std::vector<int> int_vector;
    typedef std::deque<int> int_deque;
    typedef std::list<int> int_list;

    typedef std::vector<str_sequence> seq_vector;
    typedef std::deque<str_sequence> seq_deque;
    typedef std::list<str_sequence> seq_list;

    int_vector vi;
    int_deque di;
    int_list li;
    seq_vector vs;
    seq_deque ds;
    seq_list ls;

    srand((unsigned)time(0));

    const int TIMES = 10; // how many times to run each test?
    run_test("set_to_idx, vector<int>", vi,       set_to_idx(), TIMES * 10, ELEMS_COUNT * 10);
    run_test("set_to_idx, list<int>", li,         set_to_idx(), TIMES * 10, ELEMS_COUNT * 10);
    run_test("set_to_idx, deque<int>", di,        set_to_idx(), TIMES * 10, ELEMS_COUNT * 10);
    run_test("set_to_idx, vector<seq>", vs,       set_to_idx(), TIMES, ELEMS_COUNT);
    run_test("set_to_idx, list<seq>", ls,         set_to_idx(), TIMES, ELEMS_COUNT);
    run_test("set_to_idx, deque<seq>", ds,        set_to_idx(), TIMES, ELEMS_COUNT);

    run_test("multiply_by_2, vector<int>", vi,       multiply_by_2, TIMES * 10, ELEMS_COUNT * 10);
    run_test("multiply_by_2, list<int>", li,         multiply_by_2, TIMES * 10, ELEMS_COUNT * 10);
    run_test("multiply_by_2, deque<int>", di,        multiply_by_2, TIMES * 10, ELEMS_COUNT * 10);

    run_test("multiply_by_max20, vector<int>", vi,       multiply_by_max20, TIMES * 10, ELEMS_COUNT * 20);
    run_test("multiply_by_max20, list<int>", li,         multiply_by_max20, TIMES * 10, ELEMS_COUNT * 15);
    run_test("multiply_by_max20, deque<int>", di,        multiply_by_max20, TIMES * 10, ELEMS_COUNT * 15);

    run_test("add_10, vector<int>", vi,       add_10, TIMES * 5, ELEMS_COUNT * 10);
    run_test("add_10, list<int>", li,         add_10, TIMES * 5, ELEMS_COUNT * 10);
    run_test("add_10, deque<int>", di,        add_10, TIMES * 5, ELEMS_COUNT * 10);

    run_test("add_one_char, vector<seq>", vs,       add_one_char, TIMES * 5, ELEMS_COUNT);
    run_test("add_one_char, list<seq>", ls,         add_one_char, TIMES * 5, ELEMS_COUNT);
    run_test("add_one_char, deque<seq>", ds,        add_one_char, TIMES * 5, ELEMS_COUNT);

    run_test("set_to_len, vector<seq>", vs,       set_to_len, TIMES, ELEMS_COUNT);
    run_test("set_to_len, list<seq>", ls,         set_to_len, TIMES, ELEMS_COUNT);
    run_test("set_to_len, deque<seq>", ds,        set_to_len, TIMES, ELEMS_COUNT);

    run_test("set_to_az, vector<seq>", vs,       set_to_az(), TIMES, ELEMS_COUNT);
    run_test("set_to_az, list<seq>", ls,         set_to_az(), TIMES, ELEMS_COUNT);
    run_test("set_to_az, deque<seq>", ds,        set_to_az(), TIMES, ELEMS_COUNT);

    run_test("noop, vector<int>", vi,       noop_int, TIMES * 5, ELEMS_COUNT * 25);
    run_test("noop, list<int>", li,         noop_int, TIMES * 5, ELEMS_COUNT * 25);
    run_test("noop, deque<int>", di,        noop_int, TIMES * 5, ELEMS_COUNT * 25);
    run_test("noop, vector<seq>", vs,       noop_seq, TIMES * 50, ELEMS_COUNT);
    run_test("noop, list<seq>", ls,         noop_seq, TIMES * 50, ELEMS_COUNT);
    run_test("noop, deque<seq>", ds,        noop_seq, TIMES * 50, ELEMS_COUNT);

}

std::stringstream avg;
void test_for_elems() {
    first_test_run = false;
    run_times_so_far = 0;
    sum_raw = 0;
    sum_each = 0;
    sum_each_percentage = 0;

    for (int idx = 0; idx < TESTS; ++idx) {
        results_file << "Test " << idx + 1 << " of " << TESTS << " (for " << ELEMS_COUNT << " elements)" << std::endl;
        std::cout << "Test " << idx + 1 << " of " << TESTS << " (for " << ELEMS_COUNT << " elements)" << std::endl;
        test();
        first_test_run = true;
    }

    avg << "\nAverages for " << ELEMS_COUNT << "elements (except first test): \n" 
        << "raw time    : " << sum_raw << " secs \n"
        << "foreach calls: " << sum_each << " secs (avg percentage " << sum_each_percentage / run_times_so_far << ")" << std::endl;
}

// results are outputted to "result.txt" also
int main(int argc, char * argv[]) {
    // Example of usage: 'testcrange 32000 128000'
    //                    will run each test for with 32000, 64000, 128000 elements
    if (argc > 1) ELEMS_COUNT = atoi(argv[1]);
    if (argc > 2) MAX_ELEMS_COUNT = atoi(argv[2]);

    clock_t start = clock();
    for (int count = ELEMS_COUNT; count <= MAX_ELEMS_COUNT; count *= 2) {
        ELEMS_COUNT = count;
        test_for_elems();
    }
    clock_t end = clock();
    double total = (double)(end - start) / CLOCKS_PER_SEC;
    std::cout << avg.str() << std::endl;
    results_file << avg.str() << std::endl;
    std::cout << "Test took " << total << " secs " << std::endl;
    results_file << "Test took " << total << " secs " << std::endl;
    return 0;
}

