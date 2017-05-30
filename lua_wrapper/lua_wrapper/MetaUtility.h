#pragma once

#include <tuple>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

//----把指针或引用统一转化为引用-------------------------------------------------------------------------------
namespace Internal
{
    template<class T, 
        bool isPointer = std::is_pointer<typename std::remove_reference<T>::type>::value, 
        bool = std::is_lvalue_reference<T>::value>
    struct ReferenceTypeHelper
    {
        static const bool is_pointer = isPointer;
        using type = typename std::remove_pointer<typename std::remove_reference<T>::type>::type &;
    };

    template<class T>
    struct ReferenceTypeHelper<T, false, false>
    {
        static const bool is_pointer = false;
        using type = typename std::remove_pointer<typename std::remove_reference<T>::type>::type &&;
    };

    template<class T, bool = ReferenceTypeHelper<T>::is_pointer>
    struct ToReferenceImpl
    {
        static typename ReferenceTypeHelper<T>::type ToReference(T && arg)
        {
            return static_cast<typename ReferenceTypeHelper<T>::type>(*arg);
        }
    };

    template<class T>
    struct ToReferenceImpl<T, false>
    {
        static typename ReferenceTypeHelper<T>::type ToReference(T && arg)
        {
            return static_cast<typename ReferenceTypeHelper<T>::type>(arg);
        }
    };
}

//把指针或引用统一转化为引用
template<class T>
typename Internal::ReferenceTypeHelper<T>::type to_reference(T && arg)
{
    return Internal::ToReferenceImpl<T>::ToReference(std::forward<T>(arg));
}

//----生成参数序列-----------------------------------------------------------------------

template<size_t ... Index>
struct IntegerSequence
{};

namespace Internal
{
    template<class T, size_t N>
    struct MakeSequenceHelper;

    template<size_t ... Index>
    struct MakeSequenceHelper<IntegerSequence<Index...>, 0>
    {
        using type = IntegerSequence<Index...>;
    };

    template<size_t ... Index, size_t N>
    struct MakeSequenceHelper<IntegerSequence<Index...>, N>
        : public MakeSequenceHelper<IntegerSequence<N - 1, Index...>, N - 1>
    {};
}

//根据可变参数生成参数序列,从0开始
template<size_t N>
struct MakeSequence
    : public Internal::MakeSequenceHelper<IntegerSequence<>, N>
{
};

//----判断类是否有成员类型------------------------------------------------------------------------

namespace Internal
{
    struct WrapInt
    {
        WrapInt(int){};
    };

    template<class T>
    struct Identity
    {
        using type = T;

        const T & operator()(const T & _Left) const
        {	// apply identity operator to operand
            return (_Left);
        }
    };
}

/* 判断类是否有成员
使用方法: 定义下面的类

template<class _ClassType>
struct HasMember...
HAS_MEMBER_TYPE_IMPL(membername)

之后就可用 HasMember...<className>::value 来判断一个类是否有某成员
实现细节: 两个静态函数必须用模板
1. 重载的函数,所有重载版本都会编译成二进制代码,遇到非法的就编译不过;
2. 模板是选择性编译,不是重载,只会选择一个合法的版本编译成二进制代码,非法的版本就不会编译,也就不会导致编译不过;
*/

//是否有成员(函数,静态变量), 有bug, 当类是模板时总是true,屏蔽
/* 
#define HAS_MEMBER_IMPL(memberName) \
{ \
private: \
    template<class T>static auto DeclFunc(int, decltype(typename T::memberName) * = 0)->std::true_type; \
    template<class T>static auto DeclFunc(shr::Internal::WrapInt)->std::false_type; \
public: \
    static const bool value = decltype(DeclFunc<_ClassType>(0))::value; \
};
*/

//是否有成员(类型)
#define HAS_MEMBER_TYPE_IMPL(memberType) \
{ \
private: \
    template<class T>static auto DeclFunc(int, typename T::memberType * = 0)->std::true_type; \
    template<class T>static auto DeclFunc(shr::Internal::WrapInt)->std::false_type; \
public: \
    static const bool value = decltype(DeclFunc<_ClassType>(0))::value; \
};

//----函数类型辅助----------------------------------------------------------------------------

namespace Internal
{
// 下面的宏用来定义函数调用类型, 是从STL源码中复制下来的

#ifdef _M_IX86

#ifdef _M_CEE
#ifndef NON_MEMBER_CALL_MACRO
#define NON_MEMBER_CALL_MACRO(FUNC, CONST_OPT) \
	FUNC(__cdecl, CONST_OPT) \
	FUNC(__stdcall, CONST_OPT) \
	FUNC(__clrcall, CONST_OPT)
#endif // !NON_MEMBER_CALL_MACRO
#ifndef MEMBER_CALL_MACRO
#define MEMBER_CALL_MACRO(FUNC, CONST_OPT, CV_OPT) \
	FUNC(__thiscall, CONST_OPT, CV_OPT) \
	FUNC(__cdecl, CONST_OPT, CV_OPT) \
	FUNC(__stdcall, CONST_OPT, CV_OPT) \
	FUNC(__clrcall, CONST_OPT, CV_OPT)
#endif // !MEMBER_CALL_MACRO

#else /* _M_CEE */
#ifndef NON_MEMBER_CALL_MACRO
#define NON_MEMBER_CALL_MACRO(FUNC, CONST_OPT) \
	FUNC(__cdecl, CONST_OPT) \
	FUNC(__stdcall, CONST_OPT) \
	FUNC(__fastcall, CONST_OPT)
#endif // !NON_MEMBER_CALL_MACRO
#ifndef MEMBER_CALL_MACRO
#define MEMBER_CALL_MACRO(FUNC, CONST_OPT, CV_OPT) \
	FUNC(__thiscall, CONST_OPT, CV_OPT) \
	FUNC(__cdecl, CONST_OPT, CV_OPT) \
	FUNC(__stdcall, CONST_OPT, CV_OPT) \
	FUNC(__fastcall, CONST_OPT, CV_OPT)
#endif // !MEMBER_CALL_MACRO
#endif /* _M_CEE */

#else /* _M_IX86 */

#ifdef _M_CEE
#ifndef NON_MEMBER_CALL_MACRO
#define NON_MEMBER_CALL_MACRO(FUNC, CONST_OPT) \
	FUNC(__cdecl, CONST_OPT) \
	FUNC(__clrcall, CONST_OPT)
#endif // !NON_MEMBER_CALL_MACRO
#ifndef MEMBER_CALL_MACRO
#define MEMBER_CALL_MACRO(FUNC, CONST_OPT, CV_OPT) \
	FUNC(__cdecl, CONST_OPT, CV_OPT) \
	FUNC(__clrcall, CONST_OPT, CV_OPT)
#endif // !MEMBER_CALL_MACRO

#else /* _M_CEE */
#ifndef NON_MEMBER_CALL_MACRO
#define NON_MEMBER_CALL_MACRO(FUNC, CONST_OPT) \
	FUNC(__cdecl, CONST_OPT)
#endif // !NON_MEMBER_CALL_MACRO
#ifndef MEMBER_CALL_MACRO
#define MEMBER_CALL_MACRO(FUNC, CONST_OPT, CV_OPT) \
	FUNC(__cdecl, CONST_OPT, CV_OPT)
#endif // !MEMBER_CALL_MACRO
#endif /* _M_CEE */
#endif /* _M_IX86 */


//下面定义的宏, 用来生成模板特化

/* 指针变量本身的const,volatile属性并不需要专门特化,只要对变量应用 std::remove_cv_t 即可.
   如果对它们专门特化, 生成的特化版本就太多了, 另外也会额外引入一些宏名字
*/

#ifndef CALLABLE_MEMBER_CALL_CV
#define CALLABLE_MEMBER_CALL_CV(FUNC, CONST_OPT) \
	MEMBER_CALL_MACRO(FUNC, CONST_OPT, ) \
	MEMBER_CALL_MACRO(FUNC, CONST_OPT, const) \
	MEMBER_CALL_MACRO(FUNC, CONST_OPT, volatile) \
	MEMBER_CALL_MACRO(FUNC, CONST_OPT, const volatile)
#endif // !CALLABLE_MEMBER_CALL_CV

}

//类型值
enum class CallableIdType
{
    FUNCTION,
    POINTER_TO_FUNCTION,
    POINTER_TO_MEMBER_FUNCTION,
    POINTER_TO_MEMBER_DATA,
    FUNCTION_OBJECT
};

/* 调用类型辅助
覆盖了函数,函数指针,成员函数指针,成员指针, 没有覆盖函数对象，定义了以下几种类型别名：
result_t, 返回值类型；
arg_tuple_t, 参数绑定成tuple的类型;
arg_index_t, 参数的序列号类型, IntegerSequence<...>
class_t, (只有成员函数指针和成员指针才定义), 类类型
callable_id, 值,CallableIdType中定义的类型值
 */
template<class T, bool= std::is_class<T>::value>
struct CallableTypeHelper;

//函数类型特化
#define FUNCTION_HELPER(CALL_OPT, NO_USE) \
template<class _RetType, class... _ArgType> \
struct CallableTypeHelper<_RetType CALL_OPT (_ArgType...), false> \
{ \
    using result_t = _RetType; \
    using arg_tuple_t = std::tuple<_ArgType...>; \
    using arg_index_t = typename MakeSequence<sizeof...(_ArgType)>::type; \
    static const CallableIdType callable_id = CallableIdType::FUNCTION; \
};
NON_MEMBER_CALL_MACRO(FUNCTION_HELPER, )
#undef FUNCTION_HELPER

//函数指针类型特化
#define POINTER_TO_FUNCTION_HELPER(CALL_OPT, NO_USE) \
template<class _RetType, class... _ArgType> \
struct CallableTypeHelper<_RetType(CALL_OPT * )(_ArgType...), false> \
{ \
    using result_t = _RetType; \
    using arg_tuple_t = std::tuple<_ArgType...>; \
    using arg_index_t = typename MakeSequence<sizeof...(_ArgType)>::type; \
    static const CallableIdType callable_id = CallableIdType::POINTER_TO_FUNCTION; \
};
NON_MEMBER_CALL_MACRO(POINTER_TO_FUNCTION_HELPER, )
#undef POINTER_TO_FUNCTION_HELPER

//成员函数指针类型特化
#define POINTER_TO_MEMBER_FUNCTION_HELPER(CALL_OPT, NO_USE, CV_OPT) \
template<class _RetType, class _ClassType, class... _ArgType> \
struct CallableTypeHelper<_RetType(CALL_OPT _ClassType::* )(_ArgType...) CV_OPT, false> \
{ \
    using class_t = _ClassType; \
    using result_t = _RetType; \
    using arg_tuple_t = std::tuple<_ArgType...>; \
    using arg_index_t = typename MakeSequence<sizeof...(_ArgType)>::type; \
    static const CallableIdType callable_id = CallableIdType::POINTER_TO_MEMBER_FUNCTION; \
};
CALLABLE_MEMBER_CALL_CV(POINTER_TO_MEMBER_FUNCTION_HELPER, )
#undef POINTER_TO_MEMBER_FUNCTION_HELPER

//成员指针类型特化
template<class _RetType, class _ClassType>
struct CallableTypeHelper<_RetType _ClassType::*, false>
{
    using class_t = _ClassType;
    using result_t = _RetType;
    using arg_tuple_t = std::tuple<>;
    using arg_index_t = typename MakeSequence<0>::type;
    static const CallableIdType callable_id = CallableIdType::POINTER_TO_MEMBER_DATA;
};

//函数对象、Lambda表达式类型特化
template<class T>
struct CallableTypeHelper<T, true>
{
    using class_t = std::decay_t<T>;
    using result_t = typename CallableTypeHelper<decltype(&class_t::operator()), false>::result_t;
    using arg_tuple_t = typename CallableTypeHelper<decltype(&class_t::operator()), false>::arg_tuple_t;
    using arg_index_t = typename CallableTypeHelper<decltype(&class_t::operator()), false>::arg_index_t;
    static const CallableIdType callable_id = CallableIdType::FUNCTION_OBJECT;
};

#undef NON_MEMBER_CALL_MACRO
#undef MEMBER_CALL_MACRO

SHARELIB_END_NAMESPACE
