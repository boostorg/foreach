//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   13 December 2004 : Initial version.
*/

#include <list>
#include <vector>
#include <boost/test/minimal.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include "../../../boost/foreach.hpp"

///////////////////////////////////////////////////////////////////////////////
// int_iterator
//
typedef ::boost::counting_iterator<int> int_iterator;

///////////////////////////////////////////////////////////////////////////////
// define come containers
//
char ntcs_buffer[] = "\1\2\3\4\5";
char *ntcs  = ntcs_buffer;
int array[] = { 1,2,3,4,5 };
std::list<int> list(int_iterator(1),int_iterator(6));
std::pair<int_iterator,int_iterator> pair(int_iterator(1),int_iterator(6));

int const (&const_array)[5] = array;
char const *const_ntcs  = ntcs;
std::list<int> const &const_list = list;
std::pair<int_iterator,int_iterator> const &const_pair = pair;

///////////////////////////////////////////////////////////////////////////////
// define a user-defined collection type and teach BOOST_FOREACH how to enumerate it
//
namespace mine
{
    struct dummy {};
}

namespace boost
{
    template<>
    struct range_iterator<mine::dummy>
    {
        typedef char * type;
    };
    template<>
    struct range_const_iterator<mine::dummy>
    {
        typedef char const * type;
    };
    char * begin(mine::dummy&) {return 0;}
    char const * begin(mine::dummy const&) {return 0;}
    char * end(mine::dummy&) {return 0;}
    char const * end(mine::dummy const&) {return 0;}
}

///////////////////////////////////////////////////////////////////////////////
// to_vector_for
//
template<typename Range>
std::vector<int> to_vector_for( Range & rng )
{
    std::vector<int> vect;
    typedef typename ::boost::range_result_iterator<Range>::type iterator;
    for(iterator begin = ::boost::begin(rng), end = ::boost::end(rng);
        begin != end; ++begin)
    {
        vect.push_back(*begin);
    }
    return vect;
}

///////////////////////////////////////////////////////////////////////////////
// to_vector_foreach_byval
//
template<typename Range>
std::vector<int> to_vector_foreach_byval( Range & rng )
{
    std::vector<int> vect;
    typedef typename ::boost::range_result_iterator<Range>::type iterator;
    typedef typename ::boost::iterator_value<iterator>::type value;
    BOOST_FOREACH( value i, rng )
    {
        vect.push_back(i);
    }
    return vect;
}

///////////////////////////////////////////////////////////////////////////////
// to_vector_foreach_byref
//
template<typename Range>
std::vector<int> to_vector_foreach_byref( Range & rng )
{
    std::vector<int> vect;
    typedef typename ::boost::range_result_iterator<Range>::type iterator;
    typedef typename ::boost::iterator_reference<iterator>::type reference;
    BOOST_FOREACH( reference i, rng )
    {
        vect.push_back(i);
    }
    return vect;
}

///////////////////////////////////////////////////////////////////////////////
// mutate_foreach_byref
//
template<typename Range>
void mutate_foreach_byref( Range & rng )
{
    typedef typename ::boost::range_result_iterator<Range>::type iterator;
    typedef typename ::boost::iterator_reference<iterator>::type reference;
    BOOST_FOREACH( reference i, rng )
    {
        ++i;
    }
}


///////////////////////////////////////////////////////////////////////////////
// test_main
//   
int test_main( int, char*[] )
{
    // non-const containers by value
    BOOST_CHECK(to_vector_foreach_byval(array) == to_vector_for(array));
    BOOST_CHECK(to_vector_foreach_byval(ntcs)  == to_vector_for(ntcs));
    BOOST_CHECK(to_vector_foreach_byval(list)  == to_vector_for(list));
    BOOST_CHECK(to_vector_foreach_byval(pair)  == to_vector_for(pair));

    // const containers by value
    BOOST_CHECK(to_vector_foreach_byval(const_array) == to_vector_for(const_array));
    BOOST_CHECK(to_vector_foreach_byval(const_ntcs)  == to_vector_for(const_ntcs));
    BOOST_CHECK(to_vector_foreach_byval(const_list)  == to_vector_for(const_list));
    BOOST_CHECK(to_vector_foreach_byval(const_pair)  == to_vector_for(const_pair));

    // non-const containers by reference
    BOOST_CHECK(to_vector_foreach_byref(array) == to_vector_for(array));
    BOOST_CHECK(to_vector_foreach_byref(ntcs)  == to_vector_for(ntcs));
    BOOST_CHECK(to_vector_foreach_byref(list)  == to_vector_for(list));
    BOOST_CHECK(to_vector_foreach_byref(pair)  == to_vector_for(pair));

    // const containers by reference
    BOOST_CHECK(to_vector_foreach_byref(const_array) == to_vector_for(const_array));
    BOOST_CHECK(to_vector_foreach_byref(const_ntcs)  == to_vector_for(const_ntcs));
    BOOST_CHECK(to_vector_foreach_byref(const_list)  == to_vector_for(const_list));
    BOOST_CHECK(to_vector_foreach_byref(const_pair)  == to_vector_for(const_pair));

    // mutate the mutable collections
    mutate_foreach_byref(array);
    mutate_foreach_byref(ntcs);
    mutate_foreach_byref(list);

    // compare the mutated collections to the actual results
    std::pair<int_iterator,int_iterator> results(int_iterator(2),int_iterator(7));
    BOOST_CHECK(to_vector_foreach_byval(array) == to_vector_for(results));
    BOOST_CHECK(to_vector_foreach_byval(ntcs)  == to_vector_for(results));
    BOOST_CHECK(to_vector_foreach_byval(list)  == to_vector_for(results));

    // loop over a user-defined type (just make sure this compiles)
    mine::dummy d;
    BOOST_FOREACH( char c, d )
    {
        ((void)c); // no-op
    }

    return 0;
}
