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

#include <boost/mpl/has_xxx.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/range/result_iterator.hpp>

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION
# include <new>
# include <boost/aligned_storage.hpp>
#else
# include <boost/mpl/bool.hpp>
#endif

namespace boost { namespace for_each {

///////////////////////////////////////////////////////////////////////////////
// has_iterator<>
//
BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator)

///////////////////////////////////////////////////////////////////////////////
// yes/no
//
typedef char yes_type;
typedef char (&no_type)[2];

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

template<typename T>
inline T &static_any_cast(static_any_t a)
{
    return static_cast<static_any<T> const &>(a).item;
}

///////////////////////////////////////////////////////////////////////////////
// wrap_type
//
template<typename T>
struct wrap_type {};

template<typename T>
inline wrap_type<T> wrap(T &t)
{
    return wrap_type<T>();
}

template<typename T>
inline wrap_type<T const> wrap(T const &t)
{
    return wrap_type<T const>();
}

// for std::pair, it's easier to remove the const here than below
template<typename T>
inline wrap_type<std::pair<T,T> > wrap(std::pair<T,T> const &t)
{
    return wrap_type<std::pair<T,T> >();
}

///////////////////////////////////////////////////////////////////////////////
// convert
//   convertible to anything -- used to eat side-effects
//
struct convert
{
    template<typename T>
    operator wrap_type<T>() const
    {
        return wrap_type<T>();
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
        {
            ::new(data.address()) T(*that.get());
        }
        else
        {
            *static_cast<T **>(data.address()) = that.get();
        }
    }

    ~simple_variant()
    {
        if(rvalue)
        {
            get()->~T();
        }
    }

    T *get() const
    {
        if(rvalue)
        {
            return static_cast<T *>(data.address());
        }
        else
        {
            return *static_cast<T **>(data.address());
        }
    }

private:
    enum { size = sizeof(T) > sizeof(T*) ? sizeof(T) : sizeof(T*) };
    bool const                      rvalue;
    mutable aligned_storage<size>   data;
};

#else // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

///////////////////////////////////////////////////////////////////////////////
// is_rvalue
//
template<typename T>
no_type is_rvalue(T &, int);

template<typename T>
yes_type is_rvalue(T, ...);

typedef mpl::true_  rvalue;
typedef mpl::false_ lvalue;

#endif // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

///////////////////////////////////////////////////////////////////////////////
// set_false
//
inline bool set_false( bool &b )
{
    return b = false;
}

///////////////////////////////////////////////////////////////////////////////
// selectors
//
typedef char (&stl_container)[1];
typedef char (&native_array)[2];
typedef char (&c_style_string)[3];
typedef char (&iterator_range)[4];
typedef char (&unknown_type)[16];

template<typename T>
stl_container select(T, typename enable_if<has_iterator<T>,int>::type);

template<typename T, std::size_t N>
native_array select(T (&)[N], int);

template<typename T>
iterator_range select(std::pair<T,T>, int);

template<typename T>
unknown_type select(T, ...);

c_style_string select(char const *, ...);
c_style_string select(wchar_t const *, ...);

///////////////////////////////////////////////////////////////////////////////
// traits
//
template<std::size_t Collection>
struct traits;

///////////////////////////////////////////////////////////////////////////////
// stl containers
//
template<>
struct traits<sizeof(stl_container)>
{
    traits(int) {}
    operator bool() const { return false; }

#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

    template<typename T>
    static static_any<simple_variant<T> > contain(T &t, bool const &)
    {
        return simple_variant<T>(&t);
    }

    template<typename T>
    static static_any<simple_variant<T const> > contain(T const &t, bool const &rvalue)
    {
        return rvalue ? simple_variant<T const>(t) : simple_variant<T const>(&t);
    }

    template<typename T>
    static static_any<typename range_result_iterator<T>::type>
    begin(static_any_t col, wrap_type<T>, bool)
    {
        return static_any_cast<simple_variant<T> >(col).get()->begin();
    }

    template<typename T>
    static static_any<typename range_result_iterator<T>::type>
    end(static_any_t col, wrap_type<T>, bool)
    {
        return static_any_cast<simple_variant<T> >(col).get()->end();
    }

#else // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

    template<typename T>
    static static_any<T *> contain(T &t, lvalue) // lvalue, store by reference
    {
        return &t;
    }

    template<typename T>
    static static_any<T> contain(T const &t, rvalue) // rvalue, store a copy
    {
        return t;
    }

    template<typename T>
    static static_any<typename range_result_iterator<T>::type>
    begin(static_any_t col, wrap_type<T>, lvalue) // lvalue, const matters here
    {
        return static_any_cast<T *>(col)->begin();
    }

    template<typename T>
    static static_any<typename T::const_iterator>
    begin(static_any_t col, wrap_type<T const>, rvalue) // rvalue, allow only const iteration
    {
        // const_cast here is being used to add const, not remove it.
        return const_cast<T const &>(static_any_cast<T>(col)).begin();
    }

    template<typename T>
    static static_any<typename range_result_iterator<T>::type>
    end(static_any_t col, wrap_type<T>, lvalue) // lvalue, const matters here
    {
        return static_any_cast<T *>(col)->end();
    }

    template<typename T>
    static static_any<typename T::const_iterator>
    end(static_any_t col, wrap_type<T const>, rvalue) // rvalue, allow only const iteration
    {
        // const_cast here is being used to add const, not remove it.
        return const_cast<T const &>(static_any_cast<T>(col)).end();
    }

#endif // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

    template<typename T>
    static bool done(static_any_t cur, static_any_t end, wrap_type<T>)
    {
        typedef typename range_result_iterator<T>::type iter_t;
        return static_any_cast<iter_t>(cur) == static_any_cast<iter_t>(end);
    }

    template<typename T>
    static void next(static_any_t cur, wrap_type<T>)
    {
        typedef typename range_result_iterator<T>::type iter_t;
        ++static_any_cast<iter_t>(cur);
    }

    template<typename T>
    static typename std::iterator_traits<typename range_result_iterator<T>::type>::reference
    deref(static_any_t cur, wrap_type<T>)
    {
        typedef typename range_result_iterator<T>::type iter_t;
        return *static_any_cast<iter_t>(cur);
    }
};

///////////////////////////////////////////////////////////////////////////////
// native arrays
//
template<>
struct traits<sizeof(native_array)>
{
    traits(int) {}
    operator bool() const { return false; }

    template<typename T, std::size_t N>
    static static_any<T *> contain(T (&t)[N], bool const &)
    {
        return t;
    }

    template<typename T, std::size_t N>
    static static_any<T *> begin(static_any_t col, wrap_type<T[N]>, bool)
    {
        return static_any_cast<T *>(col);
    }

    template<typename T, std::size_t N>
    static static_any<T *> end(static_any_t col, wrap_type<T[N]>, bool)
    {
        return static_any_cast<T *>(col) + N;
    }

    template<typename T, std::size_t N>
    static bool done(static_any_t cur, static_any_t end, wrap_type<T[N]>)
    {
        return static_any_cast<T *>(cur) == static_any_cast<T *>(end);
    }

    template<typename T, std::size_t N>
    static void next(static_any_t cur, wrap_type<T[N]>)
    {
        ++static_any_cast<T *>(cur);
    }

    template<typename T, std::size_t N>
    static T &deref(static_any_t cur, wrap_type<T[N]>)
    {
        return *static_any_cast<T *>(cur);
    }
};

///////////////////////////////////////////////////////////////////////////////
// c-style strings
//
template<>
struct traits<sizeof(c_style_string)>
{
    traits(int) {}
    operator bool() const { return false; }

    template<typename T>
    static static_any<T *> contain(T *t, bool const &)
    {
        return t;
    }

    template<typename T>
    static static_any<T *> begin(static_any_t col, wrap_type<T *>, bool)
    {
        return static_any_cast<T *>(col);
    }

    template<typename T>
    static static_any<int> end(static_any_t col, wrap_type<T *>, bool)
    {
        return 0; // not used
    }

    template<typename T>
    static bool done(static_any_t cur, static_any_t, wrap_type<T *>)
    {
        return ! *static_any_cast<T *>(cur);
    }

    template<typename T>
    static void next(static_any_t cur, wrap_type<T *>)
    {
        ++static_any_cast<T *>(cur);
    }

    template<typename T>
    static T &deref(static_any_t cur, wrap_type<T *>)
    {
        return *static_any_cast<T *>(cur);
    }
};

/////////////////////////////////////////////////////////////////////////////
// iterator ranges
//
template<>
struct traits<sizeof(iterator_range)>
{
    traits(int) {}
    operator bool() const { return false; }

    template<typename T>
    static static_any<std::pair<T,T> > contain(std::pair<T,T> const &t, bool const &)
    {
        return t;
    }

    template<typename T>
    static static_any<T> begin(static_any_t col, wrap_type<std::pair<T,T> >, bool)
    {
        return static_any_cast<std::pair<T,T> >(col).first;
    }

    template<typename T>
    static static_any<T> end(static_any_t col, wrap_type<std::pair<T,T> >, bool)
    {
        return static_any_cast<std::pair<T,T> >(col).second;
    }

    template<typename T>
    static bool done(static_any_t cur, static_any_t end, wrap_type<std::pair<T,T> >)
    {
        return static_any_cast<T>(cur) == static_any_cast<T>(end);
    }

    template<typename T>
    static void next(static_any_t cur, wrap_type<std::pair<T,T> >)
    {
        ++static_any_cast<T>(cur);
    }

    template<typename T>
    static typename std::iterator_traits<T>::reference 
    deref(static_any_t cur, wrap_type<std::pair<T,T> >)
    {
        return *static_any_cast<T>(cur);
    }
};

} // namespace for_each
} // namespace boost


#ifndef BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

# define BOOST_FE_RVALUE(COL) _foreach_rval

// Evaluate the container expression, and detect if it is an l-value or and r-value
# define BOOST_FE_EVAL(COL)                                                                     \
    (true ? ::boost::for_each::rvalue_probe((COL),_foreach_rval) : (COL))

#else // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

# define BOOST_FE_RVALUE(COL)                                                                   \
    (::boost::mpl::bool_<(sizeof(::boost::for_each::is_rvalue((COL),0))                         \
                        ==sizeof(::boost::for_each::yes_type))>())

// Evaluate the container expression, and detect if it is an l-value or and r-value
# define BOOST_FE_EVAL(COL) (COL)

#endif // BOOST_FOREACH_NO_CONST_RVALUE_DETECTION

// A sneaky way to get the type of the collection without evaluating the expression
#define BOOST_FE_TYPEOF(COL)                                                                    \
    (true ? ::boost::for_each::convert() : ::boost::for_each::wrap(COL))

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
#define BOOST_FOREACH(VAR, COL)                                                                                                                 \
    if       (bool _foreach_rval = false) {}                                                                                                    \
    else if  (::boost::for_each::traits<sizeof(::boost::for_each::select((COL),0))> _foreach_tr = 0) {}                                         \
    else if  (::boost::for_each::static_any_t _foreach_col = _foreach_tr.contain(BOOST_FE_EVAL(COL), BOOST_FE_RVALUE(COL))) {}                  \
    else if  (::boost::for_each::static_any_t _foreach_cur = _foreach_tr.begin(_foreach_col, BOOST_FE_TYPEOF(COL), BOOST_FE_RVALUE(COL))) {}    \
    else if  (::boost::for_each::static_any_t _foreach_end = _foreach_tr.end(_foreach_col, BOOST_FE_TYPEOF(COL), BOOST_FE_RVALUE(COL))) {}      \
    else for (bool _foreach_continue = true;                                                                                                    \
               _foreach_continue && !_foreach_tr.done(_foreach_cur, _foreach_end, BOOST_FE_TYPEOF(COL));                                        \
               _foreach_continue ? _foreach_tr.next(_foreach_cur, BOOST_FE_TYPEOF(COL)) : (void)0)                                              \
         if       (::boost::for_each::set_false(_foreach_continue)) {}                                                                          \
         else for (VAR = _foreach_tr.deref(_foreach_cur, BOOST_FE_TYPEOF(COL));                                                                 \
                   !_foreach_continue; _foreach_continue = true)

#endif
