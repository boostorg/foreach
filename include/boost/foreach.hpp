///////////////////////////////////////////////////////////////////////////////
// foreach.hpp header file
//
// Copyright 2004 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_FOREACH
#include <cstddef>
#include <utility>  // for std::pair
#include <iterator> // for std::iterator_traits

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1400))             \
 || BOOST_WORKAROUND(BOOST_INTEL_WIN, BOOST_TESTED_AT(800))
# define BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
#endif

#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/range/result_iterator.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
# include <new>
# include <boost/aligned_storage.hpp>
#endif

namespace boost
{

// forward declarations for iterator_range
template<typename T>
class iterator_range;

// forward declarations for sub_range
template<typename T>
class sub_range;

namespace for_each
{

///////////////////////////////////////////////////////////////////////////////
// adl_begin/adl_end
//
template<typename T>
inline typename range_result_iterator<T>::type adl_begin(T &t)
{
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
    return boost::begin(t);
#else
    using boost::begin;
    return typename range_result_iterator<T>::type(begin(t));
#endif
}

template<typename T>
inline typename range_result_iterator<T>::type adl_end(T &t)
{
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
    return boost::end(t);
#else
    using boost::end;
    return typename range_result_iterator<T>::type(end(t));
#endif
}

///////////////////////////////////////////////////////////////////////////////
// static_any_t/static_any
//
struct static_any_base
{
    // static_any_base must evaluate to false in boolean context so that
    // they can be declared in if() statements.
    operator bool() const
    {
        return false;
    }
};

template<typename T>
struct static_any : static_any_base
{
    static_any(T const &t)
        : item(t)
    {
    }

    // temporaries of type static_any will be bound to const static_any_base
    // references, but we still want to be able to mutate the stored
    // data, so declare it as mutable.
    mutable T item;
};

typedef static_any_base const &static_any_t;

template<typename T,bool C>
inline typename mpl::if_c<C,T const,T>::type &static_any_cast(static_any_t a)
{
    return static_cast<static_any<T> const &>(a).item;
}

///////////////////////////////////////////////////////////////////////////////
// container
//
template<typename T,bool C>
struct container
{
    typedef typename mpl::if_c<C,T const,T>::type type;
    typedef typename mpl::eval_if_c<C,range_const_iterator<T>,range_iterator<T> >::type iterator;
};

template<typename T>
inline container<T,false> wrap(T &)
{
    return container<T,false>();
}

template<typename T>
inline container<T,true> wrap(T const &)
{
    return container<T,true>();
}

///////////////////////////////////////////////////////////////////////////////
// convert
//   convertible to anything -- used to eat side-effects
//
struct convert
{
    template<typename T,bool C>
    operator container<T,C>() const
    {
        return container<T,C>();
    }
};

///////////////////////////////////////////////////////////////////////////////
// in_range
//
template<typename T>
inline std::pair<T,T> in_range(T begin, T end)
{
    return std::make_pair(begin, end);
}

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

///////////////////////////////////////////////////////////////////////////////
// rvalue_probe
//
struct rvalue_probe
{
    template<typename T>
    rvalue_probe(T const &t, bool &b)
        : ptemp(const_cast<T *>(&t))
        , rvalue(b)
    {
    }

    template<typename U>
    operator U()
    {
        rvalue = true;
        return *static_cast<U *>(ptemp);
    }

    template<typename V>
    operator V &() const
    {
        return *static_cast<V *>(ptemp);
    }

    void *ptemp;
    bool &rvalue;
};

///////////////////////////////////////////////////////////////////////////////
// simple_variant
//  holds either a T or a T*
template<typename T>
struct simple_variant
{
    simple_variant(T *t)
        : rvalue(false)
    {
        *static_cast<T **>(data.address()) = t;
    }

    simple_variant(T const &t)
        : rvalue(true)
    {
        ::new(data.address()) T(t);
    }

    simple_variant(simple_variant const &that)
        : rvalue(that.rvalue)
    {
        if(rvalue)
            ::new(data.address()) T(*that.get());
        else
            *static_cast<T **>(data.address()) = that.get();
    }

    ~simple_variant()
    {
        if(rvalue)
            get()->~T();
    }

    T *get() const
    {
        if(rvalue)
            return static_cast<T *>(data.address());
        else
            return *static_cast<T **>(data.address());
    }

private:
    enum { size = sizeof(T) > sizeof(T*) ? sizeof(T) : sizeof(T*) };
    bool const                      rvalue;
    mutable aligned_storage<size>   data;
};

#else // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

///////////////////////////////////////////////////////////////////////////////
// yes/no
//
typedef char yes_type;
typedef char (&no_type)[2];

///////////////////////////////////////////////////////////////////////////////
// is_rvalue
//
template<typename T>
no_type is_rvalue(T &, int);

template<typename T>
yes_type is_rvalue(T const &, ...);

#endif // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

///////////////////////////////////////////////////////////////////////////////
// set_false
//
inline bool set_false(bool &b)
{
    return b = false;
}

///////////////////////////////////////////////////////////////////////////////
// cheap_copy
//   Overload this for user-defined collection types if they are inexpensive to copy.
//   This tells BOOST_FOREACH it can avoid the r-value/l-value detection stuff.
template<typename T,bool C>
inline mpl::false_ cheap_copy(container<T,C>) { return mpl::false_(); }

template<typename T,bool C>
inline mpl::true_ cheap_copy(container<std::pair<T,T>,C>) { return mpl::true_(); }

template<typename T,bool C>
inline mpl::true_ cheap_copy(container<T *,C>) { return mpl::true_(); }

template<typename T,bool C>
inline mpl::true_ cheap_copy(container<iterator_range<T>,C>) { return mpl::true_(); }

template<typename T,bool C>
inline mpl::true_ cheap_copy(container<sub_range<T>,C>) { return mpl::true_(); }

///////////////////////////////////////////////////////////////////////////////
// contain
//
template<typename T>
inline static_any<T> contain(T const &t, bool const &, mpl::true_)
{
    return t;
}

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
template<typename T>
inline static_any<T *> contain(T &t, bool const &, mpl::false_)
{
    return &t;
}

template<typename T>
inline static_any<simple_variant<T const> > contain(T const &t, bool const &rvalue, mpl::false_)
{
    return rvalue ? simple_variant<T const>(t) : simple_variant<T const>(&t);
}
#else
template<typename T>
inline static_any<T *> contain(T &t, mpl::false_, mpl::false_) // l-value
{
    return &t;
}

template<typename T>
inline static_any<T> contain(T const &t, mpl::true_, mpl::false_) // r-value
{
    return t;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// begin
//
template<typename T,bool C>
inline static_any<typename container<T,C>::iterator>
begin(static_any_t col, container<T,C>, bool, mpl::true_)
{
    return for_each::adl_begin(static_any_cast<T,C>(col));
}

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
template<typename T>
inline static_any<typename container<T,false>::iterator>
begin(static_any_t col, container<T,false>, bool, mpl::false_)
{
    return for_each::adl_begin(*static_any_cast<T *,false>(col));
}

template<typename T>
inline static_any<typename container<T,true>::iterator>
begin(static_any_t col, container<T,true>, bool, mpl::false_)
{
    return for_each::adl_begin(*static_any_cast<simple_variant<T const>,false>(col).get());
}
#else
template<typename T,bool C>
inline static_any<typename container<T,C>::iterator>
begin(static_any_t col, container<T,C>, mpl::false_, mpl::false_) // l-value
{
    typedef typename container<T,C>::type type;
    return for_each::adl_begin(*static_any_cast<type *,false>(col));
}

template<typename T>
inline static_any<typename container<T,true>::iterator>
begin(static_any_t col, container<T,true>, mpl::true_, mpl::false_) // r-value
{
    return for_each::adl_begin(static_any_cast<T,true>(col));
}
#endif

///////////////////////////////////////////////////////////////////////////////
// end
//
template<typename T,bool C>
inline static_any<typename container<T,C>::iterator>
end(static_any_t col, container<T,C>, bool, mpl::true_)
{
    return for_each::adl_end(static_any_cast<T,C>(col));
}

template<typename T,bool C>
inline static_any<int>
end(static_any_t col, container<T *,C>, bool, mpl::true_)
{
    return 0; // not used
}

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
template<typename T>
inline static_any<typename container<T,false>::iterator>
end(static_any_t col, container<T,false>, bool, mpl::false_)
{
    return for_each::adl_end(*static_any_cast<T *,false>(col));
}

template<typename T>
inline static_any<typename container<T,true>::iterator>
end(static_any_t col, container<T,true>, bool, mpl::false_)
{
    return for_each::adl_end(*static_any_cast<simple_variant<T const>,false>(col).get());
}
#else
template<typename T,bool C>
inline static_any<typename container<T,C>::iterator>
end(static_any_t col, container<T,C>, mpl::false_, mpl::false_) // l-value
{
    typedef typename container<T,C>::type type;
    return for_each::adl_end(*static_any_cast<type *,false>(col));
}

template<typename T>
inline static_any<typename container<T,true>::iterator>
end(static_any_t col, container<T,true>, mpl::true_, mpl::false_) // r-value
{
    return for_each::adl_end(static_any_cast<T,true>(col));
}
#endif

///////////////////////////////////////////////////////////////////////////////
// done
//
template<typename T,bool C>
inline bool done(static_any_t cur, static_any_t end, container<T,C>)
{
    typedef typename container<T,C>::iterator iter_t;
    return static_any_cast<iter_t,false>(cur) == static_any_cast<iter_t,false>(end);
}

template<typename T,bool C>
inline bool done(static_any_t cur, static_any_t, container<T *,C>)
{
    return ! *static_any_cast<T *,false>(cur);
}

///////////////////////////////////////////////////////////////////////////////
// next
//
template<typename T,bool C>
inline void next(static_any_t cur, container<T,C>)
{
    typedef typename container<T,C>::iterator iter_t;
    ++static_any_cast<iter_t,false>(cur);
}

///////////////////////////////////////////////////////////////////////////////
// deref
//
template<typename T,bool C>
inline typename std::iterator_traits<typename container<T,C>::iterator>::reference
deref(static_any_t cur, container<T,C>)
{
    typedef typename container<T,C>::iterator iter_t;
    return *static_any_cast<iter_t,false>(cur);
}

} // namespace for_each
} // namespace boost


#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

# define BOOST_FOREACH_RVALUE(COL) _foreach_rvalue

// Evaluate the container expression, and detect if it is an l-value or and r-value
# define BOOST_FE_EVAL(COL)                                                     \
    (true ? ::boost::for_each::rvalue_probe((COL),_foreach_rvalue) : (COL))

#else // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

# define BOOST_FOREACH_RVALUE(COL)                                              \
    (::boost::mpl::bool_<(sizeof(::boost::for_each::is_rvalue((COL),0))         \
                        ==sizeof(::boost::for_each::yes_type))>())

// Evaluate the container expression, and detect if it is an l-value or and r-value
# define BOOST_FE_EVAL(COL) (COL)

#endif // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

// A sneaky way to get the type of the collection without evaluating the expression
#define BOOST_FOREACH_TYPEOF(COL)                                               \
    (true ? ::boost::for_each::convert() : ::boost::for_each::wrap(COL))

#define BOOST_FOREACH_CONTAIN(COL)                                              \
    ::boost::for_each::contain(                                                 \
        BOOST_FE_EVAL(COL)                                                      \
      , BOOST_FOREACH_RVALUE(COL)                                               \
      , ::boost::for_each::cheap_copy(BOOST_FOREACH_TYPEOF(COL)))

#define BOOST_FOREACH_BEGIN(COL)                                                \
    ::boost::for_each::begin(                                                   \
        _foreach_col                                                            \
      , BOOST_FOREACH_TYPEOF(COL)                                               \
      , BOOST_FOREACH_RVALUE(COL)                                               \
      , ::boost::for_each::cheap_copy(BOOST_FOREACH_TYPEOF(COL)))

#define BOOST_FOREACH_END(COL)                                                  \
    ::boost::for_each::end(                                                     \
        _foreach_col                                                            \
      , BOOST_FOREACH_TYPEOF(COL)                                               \
      , BOOST_FOREACH_RVALUE(COL)                                               \
      , ::boost::for_each::cheap_copy(BOOST_FOREACH_TYPEOF(COL)))

#define BOOST_FOREACH_DONE(COL)                                                 \
    ::boost::for_each::done(                                                    \
        _foreach_cur                                                            \
      , _foreach_end                                                            \
      , BOOST_FOREACH_TYPEOF(COL))

#define BOOST_FOREACH_NEXT(COL)                                                 \
    ::boost::for_each::next(                                                    \
        _foreach_cur                                                            \
      , BOOST_FOREACH_TYPEOF(COL))

#define BOOST_FOREACH_DEREF(COL)                                                \
    ::boost::for_each::deref(_foreach_cur, BOOST_FOREACH_TYPEOF(COL))

///////////////////////////////////////////////////////////////////////////////
// BOOST_FOREACH
//
//   For iterating over collections. Collections can be
//   arrays, null-terminated strings, or STL containers.
//   The loop variable can be a value or reference. For
//   example:
//
//   std::list<int> int_list(/*stuff*/);
//   BOOST_FOREACH(int &i, int_list)
//   {
//       /* 
//        * loop body goes here.
//        * i is a reference to the int in int_list.
//        */
//   }
//
//   Alternately, you can declare the loop variable first,
//   so you can access it after the loop finishes. Obviously,
//   if you do it this way, then the loop variable cannot be
//   a reference.
//
//   int i;
//   BOOST_FOREACH(i, int_list)
//       { ... }
//
#define BOOST_FOREACH(VAR, COL)                                                                 \
    if       (bool _foreach_rvalue = false) {}                                                  \
    else if  (::boost::for_each::static_any_t _foreach_col = BOOST_FOREACH_CONTAIN(COL)) {}     \
    else if  (::boost::for_each::static_any_t _foreach_cur = BOOST_FOREACH_BEGIN(COL)) {}       \
    else if  (::boost::for_each::static_any_t _foreach_end = BOOST_FOREACH_END(COL)) {}         \
    else for (bool _foreach_continue = true;                                                    \
              _foreach_continue && !BOOST_FOREACH_DONE(COL);                                    \
              _foreach_continue ? BOOST_FOREACH_NEXT(COL) : (void)0)                            \
         if       (::boost::for_each::set_false(_foreach_continue)) {}                          \
         else for (VAR = BOOST_FOREACH_DEREF(COL); !_foreach_continue; _foreach_continue = true)

#endif
