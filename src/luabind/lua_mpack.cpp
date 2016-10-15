#include "lua_mpack.hpp"
#include "mpack/mpack.h"

static int test(lua_State* L)
{
    size_t sz;
    const char* str = luaL_checklstring(L, 1, &sz);

    mpack_tree_t tree;
    mpack_tree_init(&tree, str, sz);
    mpack_tree_parse(&tree);

    mpack_node_t root = mpack_tree_root(&tree);

    mpack_node_print(root);
    // clean up and check for errors
    if (mpack_tree_destroy(&tree) != mpack_ok) {
        fprintf(stderr, "An error occurred decoding the data!\n");
    }
    return 0;
}
static void toObject(lua_State *L, int idx, int arg,mpack_writer_t* writer);

static void toArray(lua_State *L, int idx, int arg,mpack_writer_t* writer)
{
    size_t len = lua_objlen(L, idx);
    //jobjectArray array = writeArray(len);
    
    mpack_start_array(writer, len);
    
    for (size_t i = 1; i <= len; ++i) {
        lua_rawgeti(L, idx, i);
        toObject(L, -1, arg,writer);
        lua_pop(L, 1);
    }
    mpack_finish_array(writer);
}

static int getLuaTableCount(lua_State *L, int idx) {
    int total = 0;
    lua_pushvalue(L, idx); // [table]
    lua_pushnil(L); // [table, nil]
    while (lua_next(L, -2))
    {
        // [table, key, value]
        lua_pushvalue(L, -2);
        // [table, key, value, key]
        lua_pop(L, 2);
        total = total+ 1;
    }
    // [table]
    lua_pop(L, 1);
    return total;
}

static void toMap(lua_State *L, int idx, int arg,mpack_writer_t* writer)
{
    //jobject map;
    //jmethodID put;
    //std::tie(map, put) = writeHashmap();
    
    mpack_start_map(writer, getLuaTableCount(L,idx));
    
    lua_pushvalue(L, idx); // [table]
    lua_pushnil(L); // [table, nil]
    while (lua_next(L, -2))
    {
        // [table, key, value]
        lua_pushvalue(L, -2);
        // [table, key, value, key]
        
        toObject(L, -1, arg,writer);
        toObject(L, -2, arg,writer);
        
        //LocalRef<jobject> k(env_, toObject(L, -1, arg));
        //LocalRef<jobject> v(env_, toObject(L, -2, arg));
        //env_->CallObjectMethod(map, put, k.get(), v.get());
        
        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        // [table, key]
    }
    // [table]
    lua_pop(L, 1);
    mpack_finish_map(writer);
    
    //return map;
}

static void toObject(lua_State *L, int idx, int arg,mpack_writer_t* writer)
{
    int t = lua_type(L, idx);
    switch (t) {
        case LUA_TNIL:
            return mpack_write_nil(writer);
        case LUA_TBOOLEAN:
            return mpack_write_bool(writer,lua_toboolean(L, idx));
        case LUA_TNUMBER:
            return mpack_write_double(writer,lua_tonumber(L, idx));
        case LUA_TSTRING:
            return mpack_write_cstr(writer,lua_tostring(L, idx));
        case LUA_TTABLE:
            return (lua_objlen(L, idx) > 0) ? toArray(L, idx, arg,writer) : toMap(L, idx, arg,writer);
        case LUA_TFUNCTION: // fall thought
        case LUA_TLIGHTUSERDATA: // fall thought
        case LUA_TUSERDATA: // fall thought
        case LUA_TTHREAD: // fall thought
        case LUA_TNONE: // fall thought
        default:
            arg = arg < 0 ? idx : arg;
            luaL_error(L, "bridge: unsupported type '%s' contained in table arg #%d",
                       luaL_typename(L, idx), arg);
            //return nullptr;
    }
}

static int pack(lua_State* L)
{
    char* data;
    size_t size;
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, &data, &size);
    
    toObject(L,1,1,&writer);
    lua_pushlstring(L, writer.buffer, writer.used);
    if (mpack_writer_destroy(&writer) != mpack_ok) {
        fprintf(stderr, "An error occurred encoding the data!\n");
    }
    
    if (data)
        MPACK_FREE(data);
    
    return 1;
}

static void unpack_to_lua_table(mpack_node_t node, size_t depth,lua_State* L) {
    mpack_node_data_t* data = node.data;
    switch (data->type) {

        case mpack_type_nil:
            lua_pushnil(L);
            break;
        case mpack_type_bool:
            lua_pushboolean(L, data->value.b);
            break;

        case mpack_type_float:
            lua_pushnumber(L, data->value.f);
            break;
        case mpack_type_double:
            lua_pushnumber(L, data->value.d);
            break;

        case mpack_type_int:
            lua_pushinteger(L, data->value.i);
            break;
        case mpack_type_uint:
            lua_pushinteger(L, data->value.u);
            break;

        case mpack_type_bin:
            lua_pushlstring(L, mpack_node_data(node), data->len);
            break;

        case mpack_type_ext:
            fprintf(stdout, "<ext data of type %i and length %u>",
                    mpack_node_exttype(node), data->len);
            break;

        case mpack_type_str:
        {
            const char* bytes = mpack_node_data(node);
            lua_pushlstring(L,bytes,data->len);
        }
            break;

        case mpack_type_array:
            lua_newtable(L);
            for (size_t i = 0; i < data->len; ++i) {
                unpack_to_lua_table(mpack_node_array_at(node, i), depth + 1,L);
                lua_rawseti(L, -2,i+1);
            }
            break;

        case mpack_type_map:
            lua_newtable(L);
            for (size_t i = 0; i < data->len; ++i) {
                unpack_to_lua_table(mpack_node_map_key_at(node, i), depth + 1,L);
                unpack_to_lua_table(mpack_node_map_value_at(node, i), depth + 1,L);
                lua_settable(L, -3);
            }
            break;
    }
}

static int unpack(lua_State* L)
{
    size_t sz;
    const char* str = luaL_checklstring(L, 1, &sz);

    mpack_tree_t tree;
    mpack_tree_init(&tree, str, sz);
    mpack_tree_parse(&tree);

    mpack_node_t root = mpack_tree_root(&tree);
    unpack_to_lua_table(root,1,L);
    // clean up and check for errors
    if (mpack_tree_destroy(&tree) != mpack_ok) {
        fprintf(stderr, "An error occurred decoding the data!\n");
    }
    return 1;
}

int luaopen_mpack(lua_State* L)
{
    lua_newtable(L);
    lua_pushliteral(L, "test");
    lua_pushcfunction(L, test);
    lua_settable(L, -3);

    lua_pushliteral(L, "pack");
    lua_pushcfunction(L, pack);
    lua_settable(L, -3);

    lua_pushliteral(L, "unpack");
    lua_pushcfunction(L, unpack);
    lua_settable(L, -3);
    return 1;
}
