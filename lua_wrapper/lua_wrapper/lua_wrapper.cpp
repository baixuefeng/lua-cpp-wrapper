#include "lua_wrapper.h"
#include <string>

SHARELIB_BEGIN_NAMESPACE

static const char * LUA_CHUNK_FUNC_NAME = "COMPILED_LUA_FUNC";

//-------------------------------------------------------------

lua_state_wrapper::lua_state_wrapper()
	: m_pLuaState(nullptr)
{
}

lua_state_wrapper::lua_state_wrapper(lua_state_wrapper&& lua2)
{
    m_pLuaState = lua2.m_pLuaState;
    lua2.m_pLuaState = nullptr;
}

lua_state_wrapper& lua_state_wrapper::operator=(lua_state_wrapper&& lua2)
{
    if (this != &lua2)
    {
        lua_State* p = m_pLuaState;
        m_pLuaState = lua2.m_pLuaState;
        lua2.m_pLuaState = p;
    }
    return *this;
}

lua_state_wrapper::~lua_state_wrapper()
{
    close();
}

bool lua_state_wrapper::create()
{
    assert(!m_pLuaState);
    if (!m_pLuaState)
    {
        m_pLuaState = luaL_newstate();
        assert(m_pLuaState);
        if (m_pLuaState)
        {
            luaL_openlibs(m_pLuaState);
            return true;
        }
        lua_close(m_pLuaState);
        m_pLuaState = nullptr;
        return false;
    }
    return true;
}

void lua_state_wrapper::close()
{
    if (m_pLuaState)
    {
        lua_close(m_pLuaState);
        m_pLuaState = nullptr;
    }
}

void lua_state_wrapper::attach(lua_State * pState)
{
    assert(!m_pLuaState);
    m_pLuaState = pState;
}

lua_State * lua_state_wrapper::detach()
{
    auto p = m_pLuaState;
    m_pLuaState = nullptr;
    return p;
}

lua_State* lua_state_wrapper::get_raw_state()
{
	return m_pLuaState;
}

lua_state_wrapper::operator lua_State*()
{
	return m_pLuaState;
}

void * lua_state_wrapper::alloc_user_data(const char * pName, size_t size)
{
    assert(m_pLuaState);
    if (m_pLuaState && pName)
    {
        lua_stack_guard_checker check(m_pLuaState);
        void* p = lua_newuserdata(m_pLuaState, size);
        lua_setglobal(m_pLuaState, pName);
        return p;
    }
    return nullptr;
}

bool lua_state_wrapper::do_lua_file(const char * pFileName)
{
    assert(m_pLuaState);
    auto err = LUA_ERRERR;
    if (pFileName && m_pLuaState)
	{
		err = luaL_dofile(m_pLuaState, pFileName);
	}
    assert(err == LUA_OK);
	return (err == LUA_OK);
}

bool lua_state_wrapper::do_lua_file(const wchar_t * pFileName)
{
    try
    {
#ifdef LUA_CODE_UTF8
        std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
        auto & fct = std::use_facet<std::codecvt_utf16<wchar_t> >(std::locale{});
        std::wstring_convert<std::remove_reference_t<decltype(fct)> > cvt(&fct);
#endif
        return do_lua_file(cvt.to_bytes(pFileName).c_str());
    }
    catch (...)
    {
        assert(!"code convert failed!");
        return false;
    }
}

bool lua_state_wrapper::do_lua_string(const char * pString)
{
    assert(m_pLuaState);
    auto err = LUA_ERRERR;
    if (pString && m_pLuaState)
	{
		err = luaL_dostring(m_pLuaState, pString);
	}
    assert(err == LUA_OK);
    return (err == LUA_OK);
}

bool lua_state_wrapper::do_lua_string(const wchar_t * pString)
{
    try
    {
#ifdef LUA_CODE_UTF8
        std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
        auto & fct = std::use_facet<std::codecvt_utf16<wchar_t> >(std::locale{});
        std::wstring_convert<std::remove_reference_t<decltype(fct)> > cvt(&fct);
#endif
        return do_lua_file(cvt.to_bytes(pString).c_str());
    }
    catch (...)
    {
        assert(!"code convert failed!");
        return false;
    }
}

bool lua_state_wrapper::load_lua_file(const char * pFileName)
{
    assert(m_pLuaState);
    auto err = LUA_ERRERR;
    if (pFileName && *pFileName && m_pLuaState)
    {
        std::string luaStr = std::string(LUA_CHUNK_FUNC_NAME) + R"*( = loadfile([=====[)*" + pFileName + R"*(]=====]))*";
        if (do_lua_string(luaStr.c_str()))
        {
            lua_stack_guard stateGuard(m_pLuaState);
            if (LUA_TFUNCTION == lua_getglobal(m_pLuaState, LUA_CHUNK_FUNC_NAME))
            {
                err = LUA_OK;
            }
        }
    }
    assert(err == LUA_OK);
    return (err == LUA_OK);
}

bool lua_state_wrapper::load_lua_file(const wchar_t * pFileName)
{
    try
    {
#ifdef LUA_CODE_UTF8
        std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
        auto & fct = std::use_facet<std::codecvt_utf16<wchar_t> >(std::locale{});
        std::wstring_convert<std::remove_reference_t<decltype(fct)> > cvt(&fct);
#endif
        return load_lua_file(cvt.to_bytes(pFileName).c_str());
    }
    catch (...)
    {
        assert(!"code convert failed!");
        return false;
    }
}

bool lua_state_wrapper::load_lua_string(const char * pString)
{
    assert(m_pLuaState);
    auto err = LUA_ERRERR;
    if (pString && *pString && m_pLuaState)
    {
        std::string luaStr = std::string(LUA_CHUNK_FUNC_NAME) + R"*( = load([=====[)*" + pString + R"*(]=====]))*";
        if (do_lua_string(luaStr.c_str()))
        {
            lua_stack_guard stateGuard(m_pLuaState);
            if (LUA_TFUNCTION == lua_getglobal(m_pLuaState, LUA_CHUNK_FUNC_NAME))
            {
                err = LUA_OK;
            }
        }
    }
    assert(err == LUA_OK);
    return (err == LUA_OK);
}

bool lua_state_wrapper::load_lua_string(const wchar_t * pString)
{
    try
    {
#ifdef LUA_CODE_UTF8
        std::wstring_convert < std::codecvt_utf8_utf16<wchar_t> > cvt;
#else
        auto & fct = std::use_facet<std::codecvt_utf16<wchar_t> >(std::locale{});
        std::wstring_convert<std::remove_reference_t<decltype(fct)> > cvt(&fct);
#endif
        return load_lua_string(cvt.to_bytes(pString).c_str());
    }
    catch (...)
    {
        assert(!"code convert failed!");
        return false;
    }
}

bool lua_state_wrapper::run()
{
    assert(m_pLuaState);
    if (m_pLuaState)
    {
        lua_stack_guard stateGuard(m_pLuaState);
        if (LUA_TFUNCTION == lua_getglobal(m_pLuaState, LUA_CHUNK_FUNC_NAME))
        {
            return (0 == lua_pcall(m_pLuaState, 0, LUA_MULTRET, 0));
        }
    }
    return false;
}

std::string lua_state_wrapper::get_error_msg()
{
    if (!m_pLuaState)
	{
		return "未成功创建lua_State";
	}
	size_t nLenth = 0;
	const char *p = lua_tolstring(m_pLuaState, -1, &nLenth);
	if (p && nLenth > 0)
	{
		std::string err(p, nLenth);
		lua_pop(m_pLuaState, 1);
		return err;
	}
	return "";
}

int lua_state_wrapper::get_stack_count()
{
    assert(m_pLuaState);
    if (m_pLuaState)
    {
        return lua_gettop(m_pLuaState);
    }
    return 0;
}

size_t lua_state_wrapper::get_size(int index)
{
    assert(m_pLuaState);
    if (m_pLuaState)
    {
        return lua_rawlen(m_pLuaState, index);
    }
    return 0;
}

SHARELIB_END_NAMESPACE
