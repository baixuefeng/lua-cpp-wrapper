#include "lua_iostream.h"

SHARELIB_BEGIN_NAMESPACE

const lua_ostream::table_begin_t lua_ostream::table_begin;
const lua_ostream::table_end_t lua_ostream::table_end;

lua_ostream::lua_ostream(lua_State * pLua)
: m_pLua(pLua)
, m_tableIndex(0)
{
    assert(m_pLua);
}

lua_State * lua_ostream::get() const
{
    return m_pLua;
}

lua_ostream & lua_ostream::operator<<(bool value)
{
    lua_pushboolean(m_pLua, int(value));
    check_table_push();
    return *this;
}

lua_ostream & lua_ostream::operator<<(long long value)
{
    lua_pushinteger(m_pLua, lua_Integer(value));
    check_table_push();
    return *this;
}

lua_ostream & lua_ostream::operator<<(double value)
{
    lua_pushnumber(m_pLua, (lua_Number)value);
    check_table_push();
    return *this;
}

#define IMPLEMENT_LUA_OSTREAM_INTEGER(type) \
lua_ostream & lua_ostream::operator<<(type value) \
{ \
    return (*this) << (long long)value; \
}
IMPLEMENT_LUA_OSTREAM_INTEGER(char)
IMPLEMENT_LUA_OSTREAM_INTEGER(unsigned char)
IMPLEMENT_LUA_OSTREAM_INTEGER(wchar_t)
IMPLEMENT_LUA_OSTREAM_INTEGER(short)
IMPLEMENT_LUA_OSTREAM_INTEGER(unsigned short)
IMPLEMENT_LUA_OSTREAM_INTEGER(int)
IMPLEMENT_LUA_OSTREAM_INTEGER(unsigned int)
IMPLEMENT_LUA_OSTREAM_INTEGER(long)
IMPLEMENT_LUA_OSTREAM_INTEGER(unsigned long)
IMPLEMENT_LUA_OSTREAM_INTEGER(unsigned long long)
#undef IMPLEMENT_LUA_OSTREAM_INTEGER

#define IMPLEMENT_LUA_OSTREAM_FLOAT_POINT(type) \
lua_ostream & lua_ostream::operator<<(type value) \
{ \
    return (*this) << (double)value; \
}
IMPLEMENT_LUA_OSTREAM_FLOAT_POINT(float)
IMPLEMENT_LUA_OSTREAM_FLOAT_POINT(long double)
#undef IMPLEMENT_LUA_OSTREAM_FLOAT_POINT

lua_ostream & lua_ostream::operator<<(char * value)
{
    return (*this) << (const char *)value;
}

lua_ostream & lua_ostream::operator<<(const char * value)
{
    lua_pushstring(m_pLua, value);
    check_table_push();
    return *this;
}

lua_ostream & lua_ostream::operator<<(wchar_t * value)
{
    return (*this) << (const wchar_t*)value;
}

lua_ostream & lua_ostream::operator<<(const wchar_t * value)
{
    return (*this) << std::wstring(value);
}

lua_ostream & lua_ostream::operator<<(table_begin_t)
{
    assert(m_tableIndex == 0);
    lua_newtable(m_pLua);
    m_tableIndex = lua_gettop(m_pLua);
    return *this;
}

lua_ostream & lua_ostream::operator<<(table_end_t)
{
    assert(m_tableIndex > 0);
    assert(lua_type(m_pLua, m_tableIndex) == LUA_TTABLE);
    assert(lua_gettop(m_pLua) == m_tableIndex);
    m_tableIndex = 0;
    return *this;
}

void lua_ostream::check_table_push()
{
    if (m_tableIndex > 0)
    {
        assert(lua_type(m_pLua, m_tableIndex) == LUA_TTABLE);
        auto nOffset = lua_gettop(m_pLua) - m_tableIndex;
        if (nOffset == 1)
        {
            //没有key
            lua_rawseti(m_pLua, m_tableIndex, lua_rawlen(m_pLua, m_tableIndex) + 1);
        }
        else
        {
            assert(nOffset == 2);
            assert(lua_type(m_pLua, -2) == LUA_TSTRING);
            lua_rawset(m_pLua, m_tableIndex);
            assert(lua_type(m_pLua, -1) == LUA_TTABLE);
        }
    }
}

lua_ostream & lua_ostream::operator<<(lua_table_key_t key)
{
    assert(key.m_pKey);
    assert(m_tableIndex > 0);
    assert(lua_gettop(m_pLua) == m_tableIndex);
    lua_pushstring(m_pLua, key.m_pKey);
    return *this;
}

void lua_ostream::insert_subtable(lua_ostream & subTable)
{
    (void)subTable;
    assert(subTable.m_pLua == m_pLua);
    assert(m_tableIndex > 0);
    assert(lua_gettop(m_pLua) > m_tableIndex);
    assert(subTable.m_tableIndex == 0);
    assert(lua_type(m_pLua, -1) == LUA_TTABLE);
    check_table_push();
}

//----------------------------------------------------------------

lua_istream::lua_istream(lua_State * pLua, int stackIndex)
    : m_pLua(pLua)
    , m_stackIndex(lua_absindex(pLua, stackIndex))
    , m_top(lua_gettop(pLua))
    , m_isEof(false)
    , m_isOK(true)
    , m_isTableKey(false)
{
    assert(pLua);
    if (m_stackIndex != 0)
    {
        auto valueType = lua_type(m_pLua, m_stackIndex);
        if ((valueType == LUA_TNIL) || (valueType == LUA_TNONE))
        {
            m_isEof = true;
        }
        else if (valueType == LUA_TTABLE)
        {
            lua_pushnil(m_pLua);
            m_isEof = (lua_next(m_pLua, m_stackIndex) == 0);
        }
    }
}

lua_istream::~lua_istream()
{
    if (lua_gettop(m_pLua) > m_top)
    {
        lua_settop(m_pLua, m_top);
    }
}

lua_State * lua_istream::get() const
{
    return m_pLua;
}

int lua_istream::index() const
{
    return m_stackIndex;
}

bool lua_istream::eof() const
{
    return m_isEof;
}

bool lua_istream::bad() const
{
    return !m_isOK;
}

lua_istream::operator void*() const
{
    return (void*)m_isOK;
}

bool lua_istream::isSubTable() const
{
    assert(lua_type(m_pLua, m_stackIndex) == LUA_TTABLE);
    if (lua_type(m_pLua, m_stackIndex) != LUA_TTABLE)
    {
        return false;
    }
    else if (m_isEof)
    {
        return false;
    }
    else 
    {
        return lua_type(m_pLua, -1) == LUA_TTABLE;
    }
}

lua_istream & lua_istream::operator>>(bool & value)
{
    if (!m_isEof)
    {
        m_isOK = (lua_type(m_pLua, get_value_index()) == LUA_TBOOLEAN);
        if (m_isOK)
        {
            value = !!lua_toboolean(m_pLua, get_value_index());
        }
        next();
    }
    return *this;
}

#define IMPLEMENT_LUA_ISTREAM_INTEGER(type) \
lua_istream & lua_istream::operator>>(type & value) \
{ \
    if (!m_isEof) \
    { \
        m_isOK = (lua_type(m_pLua, get_value_index()) == LUA_TNUMBER); \
        if (m_isOK) \
        { \
            value = (type)lua_tointeger(m_pLua, get_value_index()); \
        } \
        next(); \
    } \
    return *this; \
}
IMPLEMENT_LUA_ISTREAM_INTEGER(char)
IMPLEMENT_LUA_ISTREAM_INTEGER(unsigned char)
IMPLEMENT_LUA_ISTREAM_INTEGER(wchar_t)
IMPLEMENT_LUA_ISTREAM_INTEGER(short)
IMPLEMENT_LUA_ISTREAM_INTEGER(unsigned short)
IMPLEMENT_LUA_ISTREAM_INTEGER(int)
IMPLEMENT_LUA_ISTREAM_INTEGER(unsigned int)
IMPLEMENT_LUA_ISTREAM_INTEGER(long)
IMPLEMENT_LUA_ISTREAM_INTEGER(unsigned long)
IMPLEMENT_LUA_ISTREAM_INTEGER(long long)
IMPLEMENT_LUA_ISTREAM_INTEGER(unsigned long long)
#undef IMPLEMENT_LUA_ISTREAM_INTEGER

#define IMPLEMENT_LUA_ISTREAM_FLOAT_POINT(type) \
lua_istream & lua_istream::operator>>(type & value) \
{ \
    if (!m_isEof) \
    { \
        m_isOK = (lua_type(m_pLua, get_value_index()) == LUA_TNUMBER); \
        if (m_isOK) \
        { \
            value = (type)lua_tonumber(m_pLua, get_value_index()); \
        } \
        next(); \
    } \
    return *this; \
}
IMPLEMENT_LUA_ISTREAM_FLOAT_POINT(float)
IMPLEMENT_LUA_ISTREAM_FLOAT_POINT(double)
IMPLEMENT_LUA_ISTREAM_FLOAT_POINT(long double)
#undef IMPLEMENT_LUA_ISTREAM_FLOAT_POINT

lua_istream & lua_istream::operator>>(lua_table_key_t key)
{
    assert(!m_isEof);
    if (!m_isEof)
    {
        assert(lua_type(m_pLua, m_stackIndex) == LUA_TTABLE);
        assert(lua_gettop(m_pLua) == m_top + 2);
        lua_pushstring(m_pLua, key.m_pKey);
        lua_rawget(m_pLua, m_stackIndex);
        m_isTableKey = true;
    }
    return *this;
}

void shr::lua_istream::cleanup_subtable(lua_istream & subTable)
{
    (void)subTable;
    assert(!m_isEof);
    assert(lua_type(m_pLua, m_stackIndex) == LUA_TTABLE);
    assert(lua_type(subTable.m_pLua, subTable.m_stackIndex) == LUA_TTABLE);
    assert(m_pLua == subTable.m_pLua);
    assert(m_stackIndex < subTable.m_stackIndex);
    m_isOK = true;
    next();
}

int lua_istream::get_value_index()
{
    assert(!m_isEof);
    if (lua_type(m_pLua, m_stackIndex) == LUA_TTABLE)
    {
        return -1;
    }
    else
    {
        return m_stackIndex;
    }
}

void lua_istream::next()
{
    assert(!m_isEof);
    if (lua_type(m_pLua, m_stackIndex) == LUA_TTABLE)
    {
        assert(lua_gettop(m_pLua) >= m_top + 2);
        if (m_isTableKey)
        {
            lua_settop(m_pLua, m_top + 2);
            m_isEof = false;
            m_isTableKey = false;
        }
        else
        {
            lua_settop(m_pLua, m_top + 1);
            m_isEof = (lua_next(m_pLua, m_stackIndex) == 0);
        }
    }
    else
    {
        assert(lua_gettop(m_pLua) == m_top);
        m_isEof = true;
    }
}

//----------------------------------------------------------------

SHARELIB_END_NAMESPACE
