#pragma once
#include <cassert>
#include "lua/src/lua.hpp"
#include "MacroDefBase.h"

/* lua脚本使用的编码
如果使用utf8, 定义该宏; 未定义时使用 std::local{} 的编码, 可以使用 std::locale::global修改编码
*/
//#define LUA_CODE_UTF8

SHARELIB_BEGIN_NAMESPACE

//----lua栈保护的辅助类-------------------------------------------------------------

//检查栈元素数量是否变更的辅助类,不会自动弹出多出的元素，只有debug模式有效
class lua_stack_check
{
public:
    explicit lua_stack_check(lua_State * pL)
#ifndef NDEBUG
        : m_pL(pL)
        , m_nCount(lua_gettop(pL))
#endif
    {
        (void)pL;
    }
    ~lua_stack_check()
    {
#ifndef NDEBUG
        assert(m_nCount == lua_gettop(m_pL));
#endif
    }

#ifndef NDEBUG
private:
    lua_State * m_pL;
    int m_nCount;
#endif
};

//自动弹出栈元素,确保数量不变的辅助类
class lua_stack_guard
{
public:
    explicit lua_stack_guard(lua_State * pL)
        : m_pL(pL)
        , m_nCount(lua_gettop(pL))
    {
    }

    ~lua_stack_guard()
    {
        int nCount = lua_gettop(m_pL);
        assert(nCount >= m_nCount);
        if (nCount > m_nCount)
        {
            lua_pop(m_pL, (nCount - m_nCount));
        }
    }
private:
    lua_State * m_pL;
    int m_nCount;
};

SHARELIB_END_NAMESPACE
