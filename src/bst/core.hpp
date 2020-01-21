#pragma once

#include <windows.h>
#include "config.hpp"

#define BST_INLINE       __forceinline

#define BST_FORCEINLINE  __forceinline

#define BST_NOINLINE     __declspec(noinline)

#define BST_NOTHROW      __declspec(nothrow)

#if (_MSC_VER < 1900) && !defined(__MINGW32__)
#if !defined(__func__)
#define __func__   __FUNCTION__
#endif
#endif

#if (_MSC_VER < 1600) && !defined(__MINGW32__)
#if !defined(nullptr)
#define nullptr  NULL
#endif
#endif

#if (_MSC_VER < 1800) && !defined(__MINGW32__)
#define BST_DEFAULT 
#define BST_DELETED 
#else
#define BST_DEFAULT  =default
#define BST_DELETED  =delete
#endif 

/* noexcept added since vs2013 CTP */
#if (_MSC_FULL_VER < 180021114) && !defined(__MINGW32__)
#if !defined(noexcept)
#define noexcept throw()
#endif
#define BST_NOEXCEPT throw()
#else
#define BST_NOEXCEPT noexcept
#endif


#define BST_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define BST_MIN(a,b)    (((a) < (b)) ? (a) : (b))


#if (_MSC_VER < 1600) && !defined(__MINGW32__)

namespace bst {
  namespace detail {

    template <bool>
    struct StaticAssertion;

    template <>
    struct StaticAssertion<true>
    {
    };

    template<int>
    struct StaticAssertionTest
    {
    };
  }
}

#define BST_CONCATENATE(arg1, arg2)   BST_CONCATENATE1(arg1, arg2)
#define BST_CONCATENATE1(arg1, arg2)  BST_CONCATENATE2(arg1, arg2)
#define BST_CONCATENATE2(arg1, arg2)  arg1##arg2

#define BST_STATIC_ASSERT(expression, message)\
struct BST_CONCATENATE(__static_assertion_at_line_, __LINE__)\
{\
  bst::detail::StaticAssertion<static_cast<bool>((expression))> \
    BST_CONCATENATE(BST_CONCATENATE(BST_CONCATENATE(STATIC_ASSERTION_FAILED_AT_LINE_, __LINE__), _WITH__), message);\
};\
typedef bst::detail::StaticAssertionTest<sizeof(BST_CONCATENATE(__static_assertion_at_line_, __LINE__))> \
  BST_CONCATENATE(__static_assertion_test_at_line_, __LINE__)

#define static_assert(expression, message) BST_STATIC_ASSERT((expression), _Error_MessageAAA_)

#endif /* _MSC_VER < 1600 */



namespace bst {

template <class T1, class T2>
struct is_same;

template <class T>
struct is_same<T,T>
{
  static const bool value = true;
};

template <class T1, class T2>
struct is_same
{
  static const bool value = false;
};

} /* namespace */
