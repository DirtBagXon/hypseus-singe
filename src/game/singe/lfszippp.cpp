/*
 * $Id$
 * lfs within zip (libzippp)
 *
 * ____ HYPSEUS COPYRIGHT NOTICE ____
 *
 * Copyright (C) 2025 DirtBagXon
 *
 * This file is part of HYPSEUS SINGE, a laserdisc arcade game emulator
 *
 * HYPSEUS SINGE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HYPSEUS SINGE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "../../io/zippp.h"
#include <cstring>
#include <set>

using namespace libzippp;

extern "C" {

extern const char* g_zipFile;
extern ZipArchive* g_zf;
extern bool* g_zlfs;

struct zip_dir_data {
    std::vector<std::string> ent;
    size_t index;

    ~zip_dir_data() = default;
};

void zip_noentry (lua_State *L) {

    lua_Debug ar;
    char caller[64] = "unknown";

    if (lua_getstack(L, 0, &ar)) {
        if (lua_getinfo(L, "n", &ar)) {
            if (ar.name) {
                strncpy(caller, ar.name, sizeof(caller) - 1);
                caller[sizeof(caller) - 1] = '\0';
            }
        }
    }
    luaL_error(L, "The lfs `%s` function is not available within a zip file", caller);
}

static int zip_iter_gc(lua_State* L) {
    zip_dir_data* d = (zip_dir_data*)lua_touserdata(L, 1);
    if (d) {
        d->~zip_dir_data();
    }
    return 0;
}

static int zip_iter(lua_State* L) {
    zip_dir_data* d = (zip_dir_data*)lua_touserdata(L, lua_upvalueindex(1));
    if (d->index < d->ent.size()) {
        lua_pushstring(L, d->ent[d->index].c_str());
        d->index++;
        return 1;
    }
    return 0;
}

int zip_iter_factory(lua_State* L) {

    const char* path = luaL_checkstring(L, 1);

    if (!path || !*path) {
        return luaL_error(L, "invalid path argument");
    }

    if (!g_zf->isOpen()) g_zf->open(ZipArchive::ReadOnly);

    if (!g_zf->isOpen()) {
        return luaL_error(L, "cannot open %s via lfs", g_zipFile);
    }

    std::string prefix(path);
    if (!prefix.empty() && prefix.back() != '/') {
        prefix += '/';
    }

    *g_zlfs = true;
    ZipEntry folder = g_zf->getEntry(prefix.c_str());
    if (!folder.isDirectory()) {
        return luaL_error(L, "cannot open %s: No such directory in zip", prefix.c_str());
    }

    std::set<std::string> child;
    std::vector<ZipEntry> entries = g_zf->getEntries();

    for (const ZipEntry& entry : entries) {
        const std::string& name = entry.getName();
        if (name.compare(0, prefix.size(), prefix) == 0) {
            std::string rest = name.substr(prefix.size());
            size_t slash_pos = rest.find('/');
            std::string child_name;
            if (slash_pos == std::string::npos) {
                child_name = rest;
            } else {
                child_name = rest.substr(0, slash_pos);
            }
            if (!child_name.empty()) {
                child.insert(child_name);
            }
        }
    }

    void* ud = lua_newuserdata(L, sizeof(zip_dir_data));
    zip_dir_data* d = new(ud) zip_dir_data();
    d->ent.assign(child.begin(), child.end());
    d->index = 0;

    if (luaL_newmetatable(L, "ZIP_ITER_MT")) {
        lua_pushcfunction(L, zip_iter_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    lua_pushcclosure(L, zip_iter, 1);
    return 1;
}

static void push_mode(lua_State* L, const ZipEntry& entry) {

    if (entry.isDirectory()) lua_pushstring(L, "directory");
    else if (!entry.isNull()) lua_pushstring(L, "file");
    else lua_pushstring(L, "invalid");
}

static void push_mtime(lua_State* L, const ZipEntry& entry) {
    time_t mtime = entry.getDate();
    lua_pushinteger(L, (lua_Integer)mtime);
}

int zip_file_info(lua_State* L) {

    const char* file = luaL_checkstring(L, 1);

    if (!g_zf->isOpen()) g_zf->open(ZipArchive::ReadOnly);

    if (!g_zf->isOpen()) {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot open %s via lfs", g_zipFile);
        return 2;
    }

    *g_zlfs = true;
    ZipEntry entry = g_zf->getEntry(file);

    if (entry.getSize() == 0) {
        std::string suffix(file);
        if (!suffix.empty() && suffix.back() != '/')
            suffix += '/';

        ZipEntry trail = g_zf->getEntry(suffix.c_str());
        if (trail.isDirectory()) entry = trail;
    }

    if (lua_isstring(L, 2)) {

        const char* member = lua_tostring(L, 2);

        if (strcmp(member, "name") == 0) {
            lua_pushstring(L, entry.getName().c_str());
            return 1;
        }
        if (strcmp(member, "mode") == 0) {
            push_mode(L, entry);
            return 1;
        }
        if (strcmp(member, "modification") == 0) {
            push_mtime(L, entry);
            return 1;
        }
        if (strcmp(member, "size") == 0) {
            lua_pushinteger(L, (lua_Integer)entry.getSize());
            return 1;
        }

        lua_pushnil(L);
        lua_pushfstring(L, "unknown member `%s` requested", member);
        return 2;
    }

    lua_newtable(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, entry.getName().c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "mode");
    push_mode(L, entry);
    lua_settable(L, -3);

    lua_pushstring(L, "modification");
    push_mtime(L, entry);
    lua_settable(L, -3);

    lua_pushstring(L, "size");
    lua_pushinteger(L, (lua_Integer)entry.getSize());
    lua_settable(L, -3);

    return 1;
}

}
