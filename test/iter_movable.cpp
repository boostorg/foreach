//  (C) Copyright Janusz Lewandowski 2013.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
Revision history:
18 December 2013 : Initial version.
*/

#include <boost/test/minimal.hpp>

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
///////////////////////////////////////////////////////////////////////////////
// define a type with movable iterator and teach BOOST_FOREACH how to enumerate it
//
namespace mine
{
    struct dummy
    {
        struct iterator : public std::iterator<std::output_iterator_tag, int>
        {
            iterator() = default;
            iterator(const iterator&) = delete;
            iterator(iterator&&) = default;
            iterator& operator++() {return *this;}
            int& operator*() {return *(new int());}
        };
    };

    bool operator==(const dummy::iterator&, const dummy::iterator&) { return false; }
    bool operator!=(const dummy::iterator&, const dummy::iterator&) { return true; }

    dummy::iterator range_begin(dummy&) {return {};}
    dummy::iterator range_end(dummy&) {return {};}
    dummy::iterator range_begin(const dummy&) {return {};}
    dummy::iterator range_end(const dummy&) {return {};}
}

#include <boost/foreach.hpp>

namespace boost
{
    template<>
    struct range_mutable_iterator<mine::dummy>
    {
        typedef mine::dummy::iterator type;
    };
    template<>
    struct range_const_iterator<mine::dummy>
    {
        typedef mine::dummy::iterator type;
    };
}
#endif

///////////////////////////////////////////////////////////////////////////////
// test_main
//
int test_main( int, char*[] )
{
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    // loop over the dummy type (just make sure this compiles)
    mine::dummy t;
    BOOST_FOREACH(int c , t)
    {
        ((void)c); // no-op
    }
#endif
    return 0;
}
