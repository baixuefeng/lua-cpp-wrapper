// lua_wrapper.cpp : 定义控制台应用程序的入口点。
//
#include "lua_wrapper/lua_wrapper.h"
#include <iostream>
#include <cassert>
#include <string>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

//重载从lua中读取RECT数据
shr::lua_istream & operator >> (shr::lua_istream & is, RECT & rc)
{
    is >> rc.left >> rc.top >> rc.right >> rc.bottom;
    return is;
}

static void TestFunc1(int a, double b, bool c, const std::wstring & d, const RECT & e)
{
    auto pre = std::wcout.imbue(std::locale{ "" });
    std::wcout << a << L" "
        << b << L" "
        << std::boolalpha << c << L" "
        << d << L" "
        << L"{ " << e.left << ", " << e.top << ", " << e.right << ", " << e.bottom << L" }"
        << std::endl;
    std::wcout.imbue(pre);
}

struct WrapCompoment
{
    std::string m_a;
    RECT m_b;
    double m_c;
};

//重载把WrapCompoment写入lua的输出运算符
static shr::lua_ostream & operator << (shr::lua_ostream & os, const WrapCompoment& compoment)
{
    os << shr::lua_ostream::table_begin
        << compoment.m_a;

    shr::lua_ostream sub(os.get());
    sub << shr::lua_ostream::table_begin
        << compoment.m_b.left << compoment.m_b.top << compoment.m_b.right << compoment.m_b.bottom
        << shr::lua_ostream::table_end;
    os.insert_subtable(sub);

    os << compoment.m_c
        << shr::lua_ostream::table_end;
    return os;
}

//重载从lua中读取WrapCompoment的输入运算符
static shr::lua_istream & operator >> (shr::lua_istream & is, WrapCompoment& compoment)
{
    is >> compoment.m_a;
    if (is.is_subtable())
    {
        shr::lua_istream sub(is.get(), -1);
        sub >> compoment.m_b.left >> compoment.m_b.top >> compoment.m_b.right >> compoment.m_b.bottom;
        is.cleanup_subtable(sub);
    }
    is >> compoment.m_c;
    return is;
}

struct LuaTestClass
{
    void SetStr(const std::wstring & str)
    {
        m_str = str;
    }

    wchar_t * GetStr()
    {
        return (wchar_t*)m_str.c_str();
    }

    void SetCompoment(const WrapCompoment & compoment)
    {
        m_wrapCompoment = compoment;
    }

    WrapCompoment m_wrapCompoment;
    std::wstring m_str;
};

struct StrCmp
{
    SHARELIB_DISABLE_COPY_CLASS(StrCmp);

    StrCmp()
    {
    }
    ~StrCmp()
    {
    }

    StrCmp(StrCmp&&)
    {
    }
    StrCmp& operator=(StrCmp &&)
    {
    }

    bool operator()(const std::string str1, const std::string str2)
    {
        return std::locale("")(str1, str2);
    }
};

BEGIN_LUA_CPP_MAP_IMPLEMENT(RegisterTestLuaFunc, "LibTest")
ENTRY_LUA_CPP_MAP_IMPLEMENT("TestFunc1", TestFunc1)
ENTRY_LUA_CPP_MAP_IMPLEMENT("SetStr", &LuaTestClass::SetStr)
ENTRY_LUA_CPP_MAP_IMPLEMENT("GetStr", &LuaTestClass::GetStr)
ENTRY_LUA_CPP_MAP_IMPLEMENT("SetCompoment", &LuaTestClass::SetCompoment)
ENTRY_LUA_CPP_MAP_IMPLEMENT("MemberStr", &LuaTestClass::m_str)
ENTRY_LUA_CPP_MAP_IMPLEMENT("MemberCompoment", &LuaTestClass::m_wrapCompoment)
ENTRY_LUA_CPP_MAP_IMPLEMENT("MessageBox", &::MessageBox)
ENTRY_LUA_CPP_MAP_IMPLEMENT("FuncObjAdd", [](int a, int b)->int{ return a + b; })
ENTRY_LUA_CPP_MAP_IMPLEMENT("FuncObjSub", [](int a, int b)->int{ return a - b; })
ENTRY_LUA_CPP_MAP_IMPLEMENT("StrComp", StrCmp{})
END_LUA_CPP_MAP_IMPLEMENT()

void TestLuaCpp()
{
    shr::lua_state_wrapper lua;
    bool bOk = lua.create();
    assert(bOk);
    RegisterTestLuaFunc(lua);

    LuaTestClass a, b;
    a.m_str = L"LuaTestClass_a";
    b.m_str = L"LuaTestClass_b";
    lua.set_variable("pA", &a);
    lua.set_variable("pB", &b);

    bOk = lua.load_lua_file("test.lua");
    assert(bOk);
    bOk = lua.run();
    assert(bOk);
}

int _tmain(int argc, _TCHAR* argv[])
{
    std::locale::global(std::locale{ "" });
    TestLuaCpp();
	return 0;
}


