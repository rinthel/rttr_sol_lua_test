//
// Created by Rinthel on 07/12/2018.
//

#include "RttrSolBinder.h"


/*! \brief Allocates from global memory (NOTE: does not currently align memory) */
struct GlobalAllocator
{
    void* Allocate(size_t sizeBytes)
    {
        return ::operator new(sizeBytes);
    }

    void DeAllocate(void* ptr, size_t /*osize*/)
    {
        assert(ptr != nullptr);		//can't decallocate null!!!
        ::operator delete(ptr);
    }

    void* ReAllocate(void* ptr, size_t osize, size_t nsize)
    {
        size_t bytesToCopy = osize;
        if (nsize < bytesToCopy)
        {
            bytesToCopy = nsize;
        }
        void* newPtr = Allocate(nsize);
        memcpy(newPtr, ptr, bytesToCopy);
        DeAllocate(ptr, osize);
        return newPtr;
    }

    static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
        GlobalAllocator * pool = static_cast<GlobalAllocator *>(ud);
        if (nsize == 0)
        {
            if (ptr != nullptr)
            {
                pool->DeAllocate(ptr, osize);
            }
            return NULL;
        }
        else
        {
            if (ptr == nullptr)
            {
                return pool->Allocate(nsize);
            }
            else
            {
                return pool->ReAllocate(ptr, osize, nsize);
            }
        }
    }
};

/*! \brief Allocates from a fixed pool.
*	Aligns all memory to 8 bytes
*	Has a min allocation of 64 bytes
*	Puts all free'd blocks on a free list.
*	Falls back to GlobalAllocator when out of memory. */
struct ArenaAllocator
{
    void* m_begin;
    void* m_end;
    char* m_curr;

    static constexpr int ALIGNMENT = 8;
    static constexpr int MIN_BLOCK_SIZE = ALIGNMENT * 8;

    struct FreeList
    {
        FreeList* m_next;
    };

    FreeList* m_freeListHead;
    GlobalAllocator m_globalAllocator;

    ArenaAllocator(void* begin, void* end) :
        m_begin(begin),
        m_end(end)
    {
        Reset();
    }

    void Reset()
    {
        m_freeListHead = nullptr;
        m_curr = static_cast<char*>(m_begin);
    }

    size_t SizeToAllocate(size_t size)
    {
        size_t allocatedSize = size;
        if (allocatedSize < MIN_BLOCK_SIZE)
        {
            allocatedSize = MIN_BLOCK_SIZE;
        }
        return allocatedSize;
    }

    void* Allocate(size_t sizeBytes)
    {
        size_t allocatedBytes = SizeToAllocate(sizeBytes);
        if (allocatedBytes <= MIN_BLOCK_SIZE && m_freeListHead)
        {
            //printf("-- allocated from the freelist --\n");
            void* ptr = m_freeListHead;
            m_freeListHead = m_freeListHead->m_next;
            return ptr;
        }
        else
        {
            m_curr = (char*)((uintptr_t)m_curr + (ALIGNMENT - 1) & ~(ALIGNMENT - 1));
            if (m_curr + allocatedBytes <= m_end)
            {
                //printf("Allocated %d bytes\n", (int)allocatedBytes);
                void* ptr = m_curr;
                m_curr += allocatedBytes;
                return ptr;
            }
            else
            {
                return m_globalAllocator.Allocate(sizeBytes);
            }
        }
    }

    void DeAllocate(void* ptr, size_t osize)
    {
        assert(ptr != nullptr);		//can't decallocate null!!!
        if (ptr >= m_begin && ptr <= m_end)
        {
            size_t allocatedBytes = SizeToAllocate(osize);
            //printf("DeAllocated %d bytes\n", (int)allocatedBytes);
            if (allocatedBytes >= MIN_BLOCK_SIZE)
            {
                //printf("-- deallocated to the freelist --\n");
                FreeList* newHead = static_cast<FreeList*>(ptr);
                newHead->m_next = m_freeListHead;
                m_freeListHead = newHead;
            }
        }
        else
        {
            m_globalAllocator.DeAllocate(ptr, osize);
        }
    }

    void* ReAllocate(void* ptr, size_t osize, size_t nsize)
    {
        //printf("ReAllocated %d bytes\n", (int)nsize);
        size_t bytesToCopy = osize;
        if (nsize < bytesToCopy)
        {
            bytesToCopy = nsize;
        }
        void* newPtr = Allocate(nsize);
        memcpy(newPtr, ptr, bytesToCopy);
        DeAllocate(ptr, osize);
        return newPtr;
    }

    static void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
        ArenaAllocator * pool = static_cast<ArenaAllocator *>(ud);
        if (nsize == 0)
        {
            if (ptr != nullptr)
            {
                pool->DeAllocate(ptr, osize);
            }
            return NULL;
        }
        else
        {
            if (ptr == nullptr)
            {
                return pool->Allocate(nsize);
            }
            else
            {
                return pool->ReAllocate(ptr, osize, nsize);
            }
        }
    }
};

constexpr int POOL_SIZE = 1024 * 10;
char memory[POOL_SIZE];
ArenaAllocator pool(memory, &memory[POOL_SIZE - 1]);


int CreateUserDatumFromVariant( lua_State* L, const rttr::variant& v );

int ToLua( lua_State* L, rttr::variant& result )
{
    int numberOfReturnValues = 0;
    if (result.is_valid() == false)
    {
        luaL_error(L, "unable to send to Lua type '%s'\n", result.get_type().get_name().to_string().c_str());
    }
    else if (result.is_type<void>() == false)
    {
        if (result.is_type<int>())
        {
            lua_pushnumber(L, result.get_value<int>());
            numberOfReturnValues++;
        }
        else if (result.is_type<short>())
        {
            lua_pushnumber(L, result.get_value<short>());
            numberOfReturnValues++;
        }
        else if ( result.get_type().is_class() || result.get_type().is_pointer() )
        {
            numberOfReturnValues += CreateUserDatumFromVariant( L, result );
        }
        else
        {
            luaL_error(L,
                "unhandled type '%s' being sent to Lua.\n",
                result.get_type().get_name().to_string().c_str());
        }
    }
    return numberOfReturnValues;
}

/*! \brief Invoke #methodToInvoke on #object, passing the arguments to the method from Lua and leave the result on the Lua stack.
*	- Assumes that the top of the stack downwards is filled with the parameters to the method we are invoking.
*	- To call a free function pass rttr::instance = {} as #object
* \return the number of values left on the Lua stack */
int InvokeMethod( lua_State* L, rttr::method& methodToInvoke, rttr::instance& object )
{
    rttr::array_range<rttr::parameter_info> nativeParams = methodToInvoke.get_parameter_infos();
    int luaParamsStackOffset = 0;
    int numNativeArgs = (int)nativeParams.size();
    int numLuaArgs = lua_gettop(L);
    if (numLuaArgs > numNativeArgs)
    {
        luaParamsStackOffset = numLuaArgs - numNativeArgs;
        numLuaArgs = numNativeArgs;
    }
    if (numLuaArgs != numNativeArgs)
    {
        printf("Error calling native function '%s', wrong number of args, expected %d, got %d\n",
            methodToInvoke.get_name().to_string().c_str(), numNativeArgs, numLuaArgs);
        assert(numLuaArgs == numNativeArgs);
    }
    union PassByValue
    {
        int intVal;
        short shortVal;
    };

    std::vector<PassByValue> pbv(numNativeArgs);
    std::vector<rttr::argument> nativeArgs(numNativeArgs);
    auto nativeParamsIt = nativeParams.begin();
    for (int i = 0; i < numLuaArgs; i++, nativeParamsIt++)
    {
        const rttr::type nativeParamType = nativeParamsIt->get_type();
        int luaArgIdx = i + 1 + luaParamsStackOffset;
        int luaType = lua_type(L, luaArgIdx);
        switch (luaType)
        {
            case LUA_TNUMBER:
                if (nativeParamType == rttr::type::get<int>())
                {
                    pbv[i].intVal = (int)lua_tonumber(L, luaArgIdx);
                    nativeArgs[i] = pbv[i].intVal;
                }
                else if (nativeParamType == rttr::type::get<short>())
                {
                    pbv[i].shortVal = (short)lua_tonumber(L, luaArgIdx);
                    nativeArgs[i] = pbv[i].shortVal;
                }
                else
                {
                    printf("unrecognised parameter type '%s'\n", nativeParamType.get_name().to_string().c_str());
                    assert(false);
                }
                break;
            default:
                luaL_error(L, "Don't know this lua type '%s', parameter %d when calling '%s'",
                    lua_typename(L, luaType),
                    i,
                    methodToInvoke.get_name().to_string().c_str());
                break;
        }
    }
    rttr::variant result = methodToInvoke.invoke_variadic(object, nativeArgs);
    return ToLua(L, result);
}

int CallGlobalFromLua(lua_State* L)
{
    rttr::method* m = (rttr::method*)lua_touserdata(L, lua_upvalueindex(1));
    rttr::method& methodToInvoke(*m);
    rttr::instance object = {};
    return InvokeMethod(L, methodToInvoke, object);
}

/*! \return The meta table name for type t */
std::string MetaTableName( const rttr::type& t )
{
    std::string metaTableName;
    if ( t.is_pointer() )
    {
        metaTableName = t.get_raw_type().get_name().to_string();
    }
    else
    {
        metaTableName = t.get_name().to_string();
    }
    metaTableName.append("_MT_");
    return metaTableName;
}

int CreateUserDatumFromVariant( lua_State* L, const rttr::variant& v )
{
    void* ud = lua_newuserdata( L, sizeof( rttr::variant ) );
    int userDatumStackIndex = lua_gettop( L );
    new (ud) rttr::variant( v );

    luaL_getmetatable( L, MetaTableName( v.get_type() ).c_str() );
    lua_setmetatable( L, userDatumStackIndex );

    lua_newtable( L );
    lua_setuservalue( L, userDatumStackIndex );

    return 1;	//return the userdatum
}

int CreateUserDatum(lua_State* L)
{
    const char* typeName = (const char*)lua_tostring(L, lua_upvalueindex(1));
    rttr::type typeToCreate = rttr::type::get_by_name(typeName);

    void* ud = lua_newuserdata(L, sizeof(rttr::variant) );
    new (ud) rttr::variant(typeToCreate.create());
    //rttr::variant& variant = *(rttr::variant*)ud;

    luaL_getmetatable(L, MetaTableName(typeToCreate).c_str());
    lua_setmetatable(L, 1);

    lua_newtable(L);
    lua_setuservalue(L, 1);

    return 1;	//return the userdatum
}

int DestroyUserDatum(lua_State* L)
{
    rttr::variant* ud = (rttr::variant*)lua_touserdata(L, -1);
    ud->~variant();
    return 0;
}

int InvokeFuncOnUserDatum(lua_State* L)
{
    rttr::method& m = *(rttr::method*)lua_touserdata(L, lua_upvalueindex(1));
    if (lua_isuserdata(L, 1) == false)
    {
        luaL_error(L, "Expected a userdatum on the lua stack when invoking native method '%s'", m.get_name().to_string().c_str());
    }

    rttr::variant& ud = *(rttr::variant*)lua_touserdata(L, 1);
    rttr::instance object(ud);
    return InvokeMethod(L, m, object);
}

int IndexUserDatum(lua_State* L)
{
    const char* typeName = (const char*)lua_tostring(L, lua_upvalueindex(1));
    rttr::type typeInfo = rttr::type::get_by_name(typeName);
    if (lua_isuserdata(L, 1) == false)
    {
        luaL_error(L, "Expected a userdatum on the lua stack when indexing native type '%s'", typeName);
    }

    if (lua_isstring(L, 2) == false)
    {
        luaL_error(L, "Expected a name of a native property or method when indexing native type '%s'", typeName);
    }

    const char* fieldName = lua_tostring(L, 2);
    rttr::method m = typeInfo.get_method(fieldName);
    if (m.is_valid())
    {
        void* methodUD = lua_newuserdata(L, sizeof(rttr::method));
        new (methodUD) rttr::method(m);
        lua_pushcclosure(L, InvokeFuncOnUserDatum, 1);
        return 1;
    }

    rttr::property p = typeInfo.get_property(fieldName);
    if (p.is_valid())
    {
        rttr::variant& ud = *(rttr::variant*)lua_touserdata(L, 1);
        rttr::variant result = p.get_value(ud);
        if (result.is_valid())
        {
            return ToLua(L, result);
        }
    }

    //if it's not a method or property then return the uservalue
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
    return 1;
}

int NewIndexUserDatum(lua_State* L)
{
    const char* typeName = (const char*)lua_tostring(L, lua_upvalueindex(1));
    rttr::type typeInfo = rttr::type::get_by_name(typeName);
    if (lua_isuserdata(L, 1) == false)
    {
        luaL_error(L, "Expected a userdatum on the lua stack when indexing native type '%s'", typeName);
    }

    if (lua_isstring(L, 2) == false)
    {
        luaL_error(L, "Expected a name of a native property or method when indexing native type '%s'", typeName);
    }

    // 3 - the value we are writing to the object

    const char* fieldName = lua_tostring(L, 2);
    rttr::property p = typeInfo.get_property(fieldName);
    if (p.is_valid())
    {
        rttr::variant& ud = *(rttr::variant*)lua_touserdata(L, 1);
        int luaType = lua_type(L, 3);
        switch (luaType)
        {
            case LUA_TNUMBER:
                if (p.get_type() == rttr::type::get<int>())
                {
                    int val = (int)lua_tonumber(L, 3);
                    assert( p.set_value(ud, val) );
                }
                else if (p.get_type() == rttr::type::get<short>())
                {
                    short val = (short)lua_tonumber(L, 3);
                    assert(p.set_value(ud, val));
                }
                else
                {
                    luaL_error(L,
                        "Cannot set the value '%s' on this type '%s', we didn't recognise the native type '%s'",
                        fieldName, typeName, p.get_type().get_name().to_string().c_str() );
                }
                break;
            default:
                luaL_error(L,
                    "Cannot set the value '%s' on this type '%s', we didnt recognise the lua type '%s'",
                    fieldName, typeName, lua_typename(L, luaType) );
                break;
        }

        return 0;
    }

    //if it wasn't a property then set it as a uservalue
    lua_getuservalue(L, 1);
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_settable(L, -3);
    return 0;
}

bool BindRttrToSol(sol::state& state)
{
    lua_State* L = luaL_newstate();
//    lua_State* L = state;

    lua_newtable( L );
    lua_pushvalue( L, -1 );
    lua_setglobal( L, "Global" );

    //binding global methods
    lua_pushvalue( L, -1 );											//1
    for ( auto& method : rttr::type::get_global_methods() )
    {
        lua_pushstring( L, method.get_name().to_string().c_str() );	//2
        lua_pushlightuserdata( L, ( void* )&method );
        lua_pushcclosure( L, CallGlobalFromLua, 1 );					//3
        lua_settable( L, -3 );										//1[2] = 3
    }

    //binding classes to Lua
    int count = 0;
    for ( auto& classToRegister : rttr::type::get_types() )
    {
        if ( classToRegister.is_class() )
        {
            const std::string s = classToRegister.get_name().to_string();
//            if (s.find("rttr::") != std::string::npos ||
//                s.find("std::") != std::string::npos)
//                continue;
            const char* typeName = s.c_str();
            printf("%4d: bind %s...\n", count++, typeName);

            lua_newtable( L );
            lua_pushvalue( L, -1 );
            lua_setglobal( L, classToRegister.get_name().to_string().c_str() );

            lua_pushvalue( L, -1 );
            lua_pushstring( L, typeName );
            lua_pushcclosure( L, CreateUserDatum, 1 );
            lua_setfield( L, -2, "new" );

            //create the metatable & metamethods for this type
            luaL_newmetatable( L, MetaTableName( classToRegister ).c_str() );
            lua_pushstring( L, "__gc" );
            lua_pushcfunction( L, DestroyUserDatum );
            lua_settable( L, -3 );

            lua_pushstring( L, "__index" );
            lua_pushstring( L, typeName );
            lua_pushcclosure( L, IndexUserDatum, 1 );
            lua_settable( L, -3 );

            lua_pushstring( L, "__newindex" );
            lua_pushstring( L, typeName );
            lua_pushcclosure( L, NewIndexUserDatum, 1 );
            lua_settable( L, -3 );
        }
    }

    return L;
}