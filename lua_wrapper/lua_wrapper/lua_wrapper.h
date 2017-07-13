#pragma once

/* 
version: 0.2
*/

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include "MacroDefBase.h"
#include "lua_iostream.h"
#include "MetaUtility.h"

SHARELIB_BEGIN_NAMESPACE

//----lua调用C++的转接辅助---------------------------------------------
namespace Internal
{
//----检查供lua调用的C函数参数是否合法-----------------------------
    template<class ...T>
    struct CheckCFuncArgValid;

    template<>
    struct CheckCFuncArgValid<>
    {
    };

    template<class T, class ...Rest>
    struct CheckCFuncArgValid<T, Rest...>
        : public CheckCFuncArgValid<Rest...>
    {
        //不能包含左值引用的参数
        static_assert(!std::is_lvalue_reference<T>::value 
        || std::is_const<std::remove_reference_t<T>>::value,
        "unsupport left value reference parameter");
    };

    template<class ...T>
    struct CheckCFuncArgValid<std::tuple<T...> >
        : public CheckCFuncArgValid<T...>
    {
    };

//----分普通函数, 成员函数, 成员变量 返回值是否为void, 区分调用------------------------------------------

    /** lua调用分发器
    @Tparam[in] callId: 常量值, 表示调用类型
    @Tparam[in] returnVoid: 常量值, 表示返回类型是否为void
    @Tparam[in] _CallableType: CallableTypeHelper类类型
    @Tparam[in] _IndexType: 参数序列号
    */
    template<CallableIdType callId, bool returnVoid, class _CallableType, class _IndexType>
    struct luaCFunctionDispatcher;

    //模板特化, 函数指针, 返回void
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::POINTER_TO_FUNCTION, true, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType pfn)
        {
            (void)pLua;//消除0参0返回值时的警告
            pfn(lua_io_dispatcher<
                std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type >
                >::from_lua(pLua, index + 1)...);
            return 0;
        }
    };

    //模板特化, 函数指针, 有返回值
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::POINTER_TO_FUNCTION, false, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType pfn)
        {
            using result_type = std::decay_t<typename _CallableType::result_t>;
            return lua_io_dispatcher<result_type>::to_lua(
                pLua,
                pfn(lua_io_dispatcher<
                    std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type >
                    >::from_lua(pLua, index + 1)...)
                );
        }
    };

    //模板特化, 成员函数指针, 返回void
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::POINTER_TO_MEMBER_FUNCTION, true, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType pmf)
        {
            using class_type = std::decay_t<typename _CallableType::class_t>;
            class_type * pThis = lua_io_dispatcher<class_type*>::from_lua(pLua, 1);
            assert(pThis);
            if (pThis)
            {
                (pThis->*pmf)(lua_io_dispatcher <
                    std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type >
                    >::from_lua(pLua, index + 2)...);
            }
            return 0;
        }
    };

    //模板特化, 成员函数指针, 有返回值
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::POINTER_TO_MEMBER_FUNCTION, false, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType pmf)
        {
            using result_type = std::decay_t<typename _CallableType::result_t>;
            using class_type = std::decay_t<typename _CallableType::class_t>;
            class_type * pThis = lua_io_dispatcher<class_type*>::from_lua(pLua, 1);
            assert(pThis);
            if (pThis)
            {
                return lua_io_dispatcher<result_type>::to_lua(
                    pLua,
                    (pThis->*pmf)(lua_io_dispatcher<
                        std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type>
                        >::from_lua(pLua, index + 2)...)
                    );
            }
            else
            {
                return 0;
            }
        }
    };

    //模板特化, 成员变量指针
    template<class _CallableType>
    struct luaCFunctionDispatcher<CallableIdType::POINTER_TO_MEMBER_DATA, false, _CallableType, IntegerSequence<> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType pmd)
        {
            using result_type = std::decay_t<typename _CallableType::result_t>;
            using class_type = std::decay_t<typename _CallableType::class_t>;
            class_type * pThis = lua_io_dispatcher<class_type*>::from_lua(pLua, 1);
            assert(pThis);
            if (pThis)
            {
                return lua_io_dispatcher<result_type>::to_lua(
                    pLua,
                    pThis->*pmd
                    );
            }
            else
            {
                return 0;
            }
        }
    };

    //模板特化, 函数对象, 返回void
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::FUNCTION_OBJECT, true, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType && fnObj)
        {
            fnObj(lua_io_dispatcher<
                std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type >
            >::from_lua(pLua, index + 1)...);
            return 0;
        }
    };

    //模板特化, 函数对象, 有返回值
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallableIdType::FUNCTION_OBJECT, false, _CallableType, IntegerSequence<index...> >
    {
        template<class _PfType>
        static int Apply(lua_State * pLua, _PfType && fnObj)
        {
            using result_type = std::decay_t<typename _CallableType::result_t>;
            return lua_io_dispatcher<result_type>::to_lua(
                pLua,
                fnObj(lua_io_dispatcher<
                    std::decay_t<typename std::tuple_element<index, typename _CallableType::arg_tuple_t>::type>
                >::from_lua(pLua, index + 1)...)
                );
        }
    };

//----lua C函数的适配函数入口----------------------------------------

    //lua调用C的主函数,所有的C++调用都从这里转发出去
    template<class _CallType, bool bTrivialDestructor>
    int MainLuaCFunctionCall(lua_State * pLua)
    {
        //upvalue中第一个值固定为真实执行的调用值
        void * ppf = lua_touserdata(pLua, lua_upvalueindex(1));
        assert(ppf);
        using _Call_Helper = CallableTypeHelper<_CallType>;
        CheckCFuncArgValid<typename _Call_Helper::arg_tuple_t> checkParam;
        (void)checkParam;
        bool bTrivial = bTrivialDestructor;//avoid warning
        if (bTrivial)
        {
            return luaCFunctionDispatcher<
                _Call_Helper::callable_id,
                std::is_void<typename _Call_Helper::result_t>::value,
                _Call_Helper,
                typename _Call_Helper::arg_index_t>::Apply(pLua, *(_CallType*)ppf);
        }
        else
        {
            using _FuncObj_Store_t = std::shared_ptr<void>;
            _FuncObj_Store_t * pSpObj = (_FuncObj_Store_t*)ppf;
            return luaCFunctionDispatcher<
                _Call_Helper::callable_id,
                std::is_void<typename _Call_Helper::result_t>::value,
                _Call_Helper,
                typename _Call_Helper::arg_index_t>::Apply(pLua, *(_CallType*)(pSpObj->get()));
        }
    }

//----分情况注册回调函数--------------------------------------------------

    //destructor
    template<class T>
    int FunctionObjectGcHelper(lua_State * pLua)
    {
        T * p = (T*)lua_touserdata(pLua, 1);
        p->~T();
        return 0;
    }

    template<bool bTrivialDestructor = true>
    struct PushCppCallableDispatcher
    {
        //trivial destructor
        template<class _CallType>
        static void PushImpl(lua_State * pLua, _CallType && pf)
        {
            using _Call_t = std::decay_t<_CallType>;
            _Call_t * ppf = (_Call_t *)lua_newuserdata(pLua, sizeof(_Call_t));
            assert(ppf);
            ::new (ppf)_Call_t(std::forward<_CallType>(pf));
            ::lua_pushcclosure(pLua, MainLuaCFunctionCall<_Call_t, true>, 1);
        }
    };

    template<>
    struct PushCppCallableDispatcher<false>
    {
        //no trivial destructor
        template<class _CallType>
        static void PushImpl(lua_State * pLua, _CallType && pf)
        {
            using _Call_t = std::decay_t<_CallType>;
            using _FuncObj_Store_t = std::shared_ptr<void>;
            _FuncObj_Store_t * pObj = (_FuncObj_Store_t *)lua_newuserdata(pLua, sizeof(_FuncObj_Store_t));
            ::new (pObj)_FuncObj_Store_t();
            *pObj = std::make_shared<_Call_t>(std::forward<_CallType>(pf));
            if (luaL_newmetatable(pLua, "{BEB170D0-0C27-4AE7-9891-6487B608C7C5}") > 0)
            {
                //创建元表进行垃圾回收,一个元表只有一个垃圾回收实现,但元表中关联的userdata有多种,
                //因此使用shared_ptr<void>封装,进行统一析构
                lua_pushcfunction(pLua, FunctionObjectGcHelper<_FuncObj_Store_t>);
                lua_setfield(pLua, -2, "__gc");
            }
            //关联userdata到垃圾回收元表中
            lua_setmetatable(pLua, -2);
            assert(LUA_TUSERDATA == lua_type(pLua, -1));
            ::lua_pushcclosure(pLua, MainLuaCFunctionCall<_Call_t, false>, 1);
        }
    };
}

/** 向栈上压入C++调用
@param[in,out] pLua lua状态指针
@param[in] pf C++调用, _CallType 传值, 自动把函数转成指向函数的指针, 并且隐含保证了函数对象是可复制的.
             目前不支持参数中的左值引用的调用类型,
*/
template<class _CallType>
void push_cpp_callable_to_lua(lua_State * pLua, _CallType && pf)
{
    ::luaL_checkstack(pLua, 1, "too many upvalues");
    Internal::PushCppCallableDispatcher<
        std::is_trivially_destructible<std::decay_t<_CallType> >::value
        >::PushImpl(pLua, std::forward<_CallType>(pf));
}


//----lua调用C++的映射实现宏---------------------------------------------
/* 这三个宏实现了一个函数, 用来完成lua调用C++的映射, 创建lua_State后, 调用该函数,
就把这些C++调用与lua关联起来了.
特性总结:
1. 支持函数指针,成员函数指针,成员变量指针,函数对象,Lambda表达式;
2. lua脚本中调用时,参数个数要和C++调用时的个数相同, 且一一对应, 自定义类型如有必要, 需重载<<和>>运算符;
3. lua调用C++的成员函数或成员变量时, 第一个参数必须传C++对象指针, 剩下的参数与该成员函数的参数相同, 
   成员变量调用则没有其它参数; 函数对象调用时不需要对象指针, 但需要保证lua回调时一些对象的生命期,比如Lambda
   捕获列表中的引用等.
4. 如果C++调用的参数列表中有默认值, 默认值不会生效.
5. 不支持C语言风格的可变数量参数
*/

/** 定义一个注册函数
@param[in] registerFuncName: 注册函数的名字, 不加引号
@param[in] pLibName: 库名字, const char *
*/
#define BEGIN_LUA_CPP_MAP_IMPLEMENT(registerFuncName, pLibName) \
void registerFuncName(lua_State * pLua) \
{ \
    const char * pLib = pLibName; \
    assert(pLua); \
    assert(pLib); \
    shr::lua_stack_check stackChecker(pLua); \
    ::lua_newtable(pLua); 

/** 添加一个C++调用到lua, 并且与一个字符串关联起来
@param[in] luaFuncName: lua脚本中调用时所用的名字, const char *
@param[in] cppCallable: C++调用, 由push_cpp_callable_to_lua实现,具体限制参照该函数注释
*/
#define ENTRY_LUA_CPP_MAP_IMPLEMENT(luaFuncName, cppCallable) \
    shr::push_cpp_callable_to_lua(pLua, cppCallable); \
    ::lua_setfield(pLua, -2, luaFuncName);

/** 结束注册函数
*/
#define END_LUA_CPP_MAP_IMPLEMENT() \
    ::lua_setglobal(pLua, pLib); \
}

//----lua_State的封装类---------------------------------------------------------

class lua_state_wrapper
{
    SHARELIB_DISABLE_COPY_CLASS(lua_state_wrapper);

    lua_State * m_pLuaState;
public:

    lua_state_wrapper();
    ~lua_state_wrapper();
    bool create();
    void close();
    void attach(lua_State * pState);
    lua_State * detach();

	lua_State* get_raw_state();
	operator lua_State*();

/* 一般的操作流程：
先设置变量、C函数等，再执行脚本，最后获取变量
*/

//----执行脚本前的操作：---------------------------------

    //往lua中写入全局变量
    template<class T>
    void set_variable(const char * pName, T value)
    {
        assert(m_pLuaState);
        if (m_pLuaState && pName)
        {
            lua_stack_check check(m_pLuaState);
            lua_io_dispatcher<std::decay_t<T>>::to_lua(m_pLuaState, value);
            lua_setglobal(m_pLuaState, pName);
        }
    }

    //从lua中分配一块命名内存, 返回内存地址, 栈保持不变
    void * alloc_user_data(const char * pName, size_t size);

//----执行脚本----------------------------

    //下面两个,加载并执行, 加载的时间代价: 毫秒量级
    bool do_lua_file(const char * pFileName);
    bool do_lua_string(const char * pString);

    //下面两个,只加载不执行,而后可以多次执行，run(),两种类型的执行脚本方法不可混用.
    bool load_lua_file(const char * pFileName);
    bool load_lua_string(const char * pString);

    /* 执行.
    本质上是把加载的lua脚本转变成lua函数,因此多次执行的lua上下文是相同的. 比如, 一个全局变量初始为0，
    第一次执行把它加1，那么第二次执行时它就是1，而不是初始值0.
    */
    bool run();

    // 获取编译失败的错误信息,注意：当失败的时候才调用
    std::string get_error_msg();

//----执行脚本后的操作-----------------------------

    //获取栈中数据的个数
    int get_stack_count();

    // 获取栈上元素的大小，字符串：长度； table：个数；userdata：size
    size_t get_size(int index);

    //从lua中读取全局变量
    template<class T>
    T get_variable(const char * pName, T defaultValue = T{})
    {
        assert(m_pLuaState);
        if (m_pLuaState && pName)
        {
            lua_stack_guard guard(m_pLuaState);
            lua_getglobal(m_pLuaState, pName);
            return lua_io_dispatcher<std::decay_t<T>>::from_lua(m_pLuaState, -1, defaultValue);
        }
        return defaultValue;
    }
};

SHARELIB_END_NAMESPACE

