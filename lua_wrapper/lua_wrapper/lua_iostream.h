#pragma once

#include <string>
#include <codecvt>
#include <cstdlib>
#include <type_traits>
#include "MacroDefBase.h"
#include "lua_wrapper_base.h"

SHARELIB_BEGIN_NAMESPACE

//---------------------------------------------------
/* table元素的kay
1. 用于lua_ostream时, 先 <<lua_table_key_t, 再 <<table元素值到lua栈上, 该元素值的key便被指定了;
   不指定kay的情况下, 按顺序依次存入。建议指定key与不指定key的两种方式不要混用，lua内部要进行hash排序，
   混用时存入lua的顺序并不等于其内部的真实顺序，导致从lua中读取时顺序与存入的顺序不相同。
2. 用于lua_istream时, 先 >>lua_table_key_t, 再 >>变量, 便可以把指定的table元素读取变量中;
*/
struct lua_table_key_t
{
    explicit lua_table_key_t(const char * pKey)
        : m_pKey(pKey)
    {
    }
    const char * m_pKey;
};

//----往lua栈上push数据-----------------------------------

/* 重载的 << 运算符原型:
lua_ostream & operator << (lua_ostream & os, const T & value);
*/
class lua_ostream
{
    SHARELIB_DISABLE_COPY_CLASS(lua_ostream);
public:
    explicit lua_ostream(lua_State * pLua);

    lua_State * get() const;

//----数值类型--------------------------
    lua_ostream & operator << (bool value);
    lua_ostream & operator << (char value);
    lua_ostream & operator << (unsigned char value);
    lua_ostream & operator << (wchar_t value);
    lua_ostream & operator << (short value);
    lua_ostream & operator << (unsigned short value);
    lua_ostream & operator << (int value);
    lua_ostream & operator << (unsigned int value);
    lua_ostream & operator << (long value);
    lua_ostream & operator << (unsigned long value);
    lua_ostream & operator << (long long value);
    lua_ostream & operator << (unsigned long long value);
    lua_ostream & operator << (float value);
    lua_ostream & operator << (double value);
    lua_ostream & operator << (long double value);

//----char字符串----------------------------
    lua_ostream & operator << (char * value);
    lua_ostream & operator << (const char * value);
    template<class T1, class T2>
    lua_ostream & operator << (const std::basic_string<char, T1, T2> & value)
    {
        return (*this) << (const char *)value.c_str();
    }

//----wchar_t字符串----------------------------
    lua_ostream & operator << (wchar_t * value);
    lua_ostream & operator << (const wchar_t * value);
    template<class T1, class T2>
    lua_ostream & operator << (const std::basic_string<wchar_t, T1, T2> & value)
    {
        try
        {
#ifdef LUA_CODE_UTF8
            std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
            auto & fct = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t> >(std::locale{});
            std::wstring_convert<std::decay_t<decltype(fct)> > cvt(&fct);
#endif
            return (*this) << cvt.to_bytes(value).c_str();
        }
        catch (...)
        {
            assert(!"code convert failed!");
            return *this;
        }
    }

//----指针----------------------------
    template<class T>
    lua_ostream & operator << (T * value)
    {
        lua_pushlightuserdata(m_pLua, value);
        check_table_push();
        return *this;
    }

//----table----------------------------
    struct table_begin_t{};
    static const table_begin_t table_begin;
    struct table_end_t{};
    static const table_end_t table_end;

    /*必须配对使用, 表示table输出的起止, 不支持嵌套.
    一个配对的table操作之后, 栈顶便是这个新加入的table
    */
    lua_ostream & operator << (table_begin_t);
    lua_ostream & operator << (table_end_t);

    /* 先输出key到lua中, 再把值输出到lua栈上, 栈顶的的值便取该名字
    */
    lua_ostream & operator << (lua_table_key_t key);

    /** 用于存入嵌套的table时。外层lua_ostream << table_begin；
    而后重新构造一个lua_ostream，存入完整的内层table；
    之后把嵌套的子table插入到外层table中。
    @param[in] subTable 里面存入了一个完整的内存table
    */
    void insert_subtable(lua_ostream & subTable);

private:
    /* 写入数据实现：
    1. 写入数据到栈上;
    2. check_table_push, 把栈上的table元素压入table中;
    3. 当输入lua_table_key_t时, 字符串入栈, 下次输入数据时, lua_rawset
    */
    void check_table_push();

    lua_State * const m_pLua;
    int m_tableIndex;
};

//----从lua中指定的栈位置读取数据--------------------------

/* 从lua中指定的栈位置读取数据, 一个lua_istream对象在eof()为true之前
 可以读，之后便不能再读，除非重新构造一个对象。如果指定的栈位置是table，将依次
 读取table的各个元素，直到eof()为true

 重载的 >> 运算符原型:
lua_istream & operator >> (lua_istream & is, T & value); 
*/
class lua_istream
{
    SHARELIB_DISABLE_COPY_CLASS(lua_istream);
public:
    lua_istream(lua_State * pLua, int stackIndex);
    ~lua_istream();
    lua_State * get() const;
    int index() const;

    //数据是否已读完,同一个lua_istream对象,一旦读完就不能再读,如果要重复读,可以重新构造一个
    bool eof() const;

    //上次读取是否成功, 不会影响下次的读取, 继续读取的话, 上次失败的也会跳过
    bool bad() const;
    operator void*() const; //用于bool判断

    //栈顶的值是否是一个嵌套的子table。首先要求自身是一个table
    bool is_subtable() const;

//----数值类型--------------------------
    lua_istream & operator >> (bool & value);
    lua_istream & operator >> (char & value);
    lua_istream & operator >> (unsigned char & value);
    lua_istream & operator >> (wchar_t & value);
    lua_istream & operator >> (short & value);
    lua_istream & operator >> (unsigned short & value);
    lua_istream & operator >> (int & value);
    lua_istream & operator >> (unsigned int & value);
    lua_istream & operator >> (long & value);
    lua_istream & operator >> (unsigned long & value);
    lua_istream & operator >> (long long & value);
    lua_istream & operator >> (unsigned long long & value);
    lua_istream & operator >> (float & value);
    lua_istream & operator >> (double & value);
    lua_istream & operator >> (long double & value);

//----char字符串----------------------------
    template<class T1, class T2>
    lua_istream & operator >> (std::basic_string<char, T1, T2> & value)
    {
        if (!m_isEof)
        {
            m_isOK = (lua_type(m_pLua, get_value_index()) == LUA_TSTRING);
            if (m_isOK)
            {
                value = lua_tostring(m_pLua, get_value_index());
            }
            next();
        }
        return *this;
    }

//----wchar_t字符串----------------------------
    template<class T1, class T2>
    lua_istream & operator >> (std::basic_string<wchar_t, T1, T2> & value)
    {
        std::string temp;
        if ((*this) >> temp)
        {
            try
            {
#ifdef LUA_CODE_UTF8
                std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
                auto & fct = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t> >(std::locale{});
                std::wstring_convert<std::decay_t<decltype(fct)> > cvt(&fct);
#endif
                value = cvt.from_bytes(temp);
            }
            catch (...)
            {
                assert(!"code convert failed!");
                value.clear();
            }
        }
        return *this;
    }

//----指针----------------------------
    template<class T>
    lua_istream & operator >> (T * & value)
    {
        if (!m_isEof)
        {
            m_isOK = (lua_type(m_pLua, get_value_index()) == LUA_TLIGHTUSERDATA);
            if (m_isOK)
            {
                value = (T*)lua_touserdata(m_pLua, get_value_index());
            }
            next();
        }
        return *this;
    }

//----table----------------------------
    /* 先 >>key, 而后 >>变量, 就表示把table中指定名字的值读取到变量
     */
    lua_istream & operator >> (lua_table_key_t key);

    /* 如果值是table，可以依次连续读取table元素到变量。但不支持table嵌套的情况下连续读取。
    table嵌套时，前面的读完之后，栈顶就是子table(is_subtable为true)，这时构造一个新的lua_istream(pLua, -1); 
    用这个新的lua_istream就可以连续读取子table中的变量了。
    读取完毕子table后，传给这个函数，进行栈清理
    */
    void cleanup_subtable(lua_istream & subTable);

private:
    /* 读取数据实现:
    1. 判断是否到头
    1. 检查类型
    2. 从get_value_index取出合适的索引, 转换数据;
    3. 跳转到下一个，如果是table，上一个键值对出栈，下一个键值对入栈; 
    4. 当输入lua_table_key_t时, lua_rawget，值入栈，下次优先读取，读完出栈。
    */
    int get_value_index();
    void next();

    lua_State * const m_pLua;
    const int m_stackIndex;
    const int m_top;
    bool m_isEof;
    bool m_isOK;
    bool m_isTableKey;
};

//----lua调用C++函数时, 传参及返回值的转接类------------------------------------------------------

/* lua调用C++时, 传参及返回值的转接类
1. 调用C++函数时的传参: 调用from_lua从lua中读取数据传参, from_lua 转接到 lua_istream , 用它实现读取数据
2. 调用C++函数时的返回值: 调用to_lua把数据写入到lua中, to_lua 转接到 lua_ostream , 用它实现数据输出
3. 支持自定义类型的做法(自定义类型要有默认构造函数, 可以复制或移动)：
    用于参数的自定义类型, 必须重载 lua_istream 的 >> 运算符;
    用于返回值的自定义类型, 必须重载 lua_ostream 的 << 运算符;
*/
template<class T, 
    bool isEnum = std::is_enum<std::decay_t<T>>::value>
struct lua_io_dispatcher
{
    /** 数据push到lua栈上
    @param[in] pL 
    @param[in] value 数据
    @return lua栈上增加的个数
    */
    static int to_lua(lua_State * pL, const T & value)
    {
        auto n = lua_gettop(pL);
        lua_ostream os(pL);
        os << value;
        assert(lua_gettop(pL) >= n);
        return lua_gettop(pL) - n;
    }

    /** 从lua栈上读取数据
    @param[in] pL 
    @param[in] index lua栈上的索引位置
    @return 转换后的C++数据
    */
    static T from_lua(lua_State * pL, int index, T defaultValue = T{})
    {
        lua_stack_check checker(pL);
        T temp{};
        lua_istream is(pL, index);
        if (is >> temp)
        {
            return std::move(temp);
        }
        else
        {
            return std::move(defaultValue);
        }
    }
};

//枚举类型特化
template<class T>
struct lua_io_dispatcher<T, true>
{
    using _UType = typename std::underlying_type_t<T>;
    static int to_lua(lua_State * pL, T value)
    {
        return lua_io_dispatcher<_UType>::to_lua(pL, static_cast<_UType>(value));
    }

    static T from_lua(lua_State * pL, int index, T defaultValue = T{})
    {
        lua_stack_check checker(pL);
        return static_cast<T>(lua_io_dispatcher<_UType>::from_lua(pL, index, defaultValue));
    }
};

//const char*类型特化
template<>
struct lua_io_dispatcher<const char*, false>
{
    static int to_lua(lua_State * pL, const char* value)
    {
        lua_ostream os(pL);
        os << value;
        return 1;
    }

    static const char* from_lua(lua_State * pL, int index, const char * defaultValue = "")
    {
        lua_stack_check checker(pL);
        if (lua_type(pL, index) == LUA_TSTRING)
        {
            return lua_tostring(pL, index);
        }
        return defaultValue;
    }
};

//char*类型特化
template<>
struct lua_io_dispatcher<char*, false>
    : public lua_io_dispatcher<const char*, false>
{
};

namespace Internal
{
    /* 为了解决wchar_t* 类型的模板特化.
    如果像其它类型一样返回const wchar_t*的话, 函数内部的临时变量销毁之后，返回的指针就成了野指针。
    const char* 不存在这个问题，因为指针所指的内容在lua内部保存着，没有被销毁。
    因此在需要返回const wchar_t*的地方，改为返回StdWstringWrapper，而它可以隐式转化为const wchar_t*，
    这样内部的临时变量经StdWstringWrapper转接之后，生命期延长，避免了野指针的情况
    */
    struct StdWstringWrapper
    {
        StdWstringWrapper(std::wstring && stdstr)
        {
            m_str.swap(stdstr);
        }

        operator const wchar_t *() const
        {
            return m_str.c_str();
        }

        operator wchar_t *() const
        {
            return (wchar_t *)m_str.c_str();
        }

        std::wstring m_str;
    };
}

//const wchar_t*类型特化
template<>
struct lua_io_dispatcher<const wchar_t*, false>
{
    static int to_lua(lua_State * pL, const wchar_t* value)
    {
        lua_ostream os(pL);
        os << value;
        return 1;
    }

    static Internal::StdWstringWrapper from_lua(lua_State * pL, int index, const wchar_t * defaultValue = L"")
    {
        lua_stack_check checker(pL);
        std::wstring temp;
        lua_istream is(pL, index);
        if (is >> temp)
        {
            return std::move(temp);
        }
        else
        {
            return std::wstring(defaultValue);
        }
    }
};

//wchar_t*类型特化
template<>
struct lua_io_dispatcher<wchar_t*, false>
    : public lua_io_dispatcher<const wchar_t*, false>
{
};

SHARELIB_END_NAMESPACE
