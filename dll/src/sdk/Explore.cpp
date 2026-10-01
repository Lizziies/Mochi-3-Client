#include "Explore.hpp"
#include "core/Log.hpp"
#include "sig/Scanner.hpp"

#include <windows.h>

#include <capstone/capstone.h>

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace explore {

namespace {

std::atomic<bool> busy{false};

uintptr_t imageBase() { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

const IMAGE_NT_HEADERS* headers() {
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(imageBase());
    return reinterpret_cast<const IMAGE_NT_HEADERS*>(imageBase() + dos->e_lfanew);
}

struct Range {
    uintptr_t start = 0;
    size_t size = 0;
};

std::vector<Range> sections(bool code, bool data) {
    std::vector<Range> out;
    auto* nt = headers();
    auto* sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        bool exec = (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        if ((exec && code) || (!exec && data)) out.push_back({imageBase() + sec->VirtualAddress, sec->Misc.VirtualSize});
    }
    return out;
}

bool readMem(uintptr_t address, void* out, size_t n) {
    SIZE_T got = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), out, n, &got) && got == n;
}

// reads in chunks and calls fn(chunkAddress, bytes, count); chunks overlap by `overlap` bytes
template <class Fn>
void walk(uintptr_t start, size_t size, size_t overlap, Fn&& fn) {
    constexpr size_t chunk = 1u << 20;
    std::vector<uint8_t> buf(chunk + overlap);
    for (size_t off = 0; off < size; off += chunk) {
        size_t n = std::min(chunk + overlap, size - off);
        if (!readMem(start + off, buf.data(), n)) {
            n = 0;
            for (size_t page = 0; page < std::min(chunk + overlap, size - off); page += 0x1000) {
                size_t len = std::min<size_t>(0x1000, size - off - page);
                if (!readMem(start + off + page, buf.data() + page, len)) std::memset(buf.data() + page, 0, len);
                n = page + len;
            }
        }
        if (fn(start + off, buf.data(), n, std::min(chunk, size - off))) return;
    }
}

uintptr_t arg(lua_State* L, int i) { return static_cast<uintptr_t>(luaL_checkinteger(L, i)); }

void push(lua_State* L, uintptr_t v) { lua_pushinteger(L, static_cast<lua_Integer>(v)); }

int lBase(lua_State* L) {
    push(L, imageBase());
    return 1;
}

int lSections(lua_State* L) {
    auto* nt = headers();
    auto* sec = IMAGE_FIRST_SECTION(nt);
    lua_newtable(L);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        lua_newtable(L);
        lua_pushlstring(L, reinterpret_cast<const char*>(sec->Name), strnlen(reinterpret_cast<const char*>(sec->Name), 8));
        lua_setfield(L, -2, "name");
        push(L, imageBase() + sec->VirtualAddress);
        lua_setfield(L, -2, "start");
        lua_pushinteger(L, sec->Misc.VirtualSize);
        lua_setfield(L, -2, "size");
        lua_pushboolean(L, (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0);
        lua_setfield(L, -2, "exec");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

int lHex(lua_State* L) {
    uintptr_t at = arg(L, 1);
    size_t n = static_cast<size_t>(luaL_checkinteger(L, 2));
    std::vector<uint8_t> buf(n);
    if (!readMem(at, buf.data(), n)) return 0;
    std::string out;
    char tmp[4];
    for (auto b : buf) {
        std::snprintf(tmp, sizeof(tmp), "%02x", b);
        out += tmp;
    }
    lua_pushstring(L, out.c_str());
    return 1;
}

template <class T>
int readValue(lua_State* L) {
    T v{};
    if (!readMem(arg(L, 1), &v, sizeof(T))) return 0;
    if constexpr (std::is_floating_point_v<T>) lua_pushnumber(L, static_cast<lua_Number>(v));
    else lua_pushinteger(L, static_cast<lua_Integer>(v));
    return 1;
}

int lCstr(lua_State* L) {
    uintptr_t at = arg(L, 1);
    int max = static_cast<int>(luaL_optinteger(L, 2, 128));
    std::string s;
    for (int i = 0; i < max; i++) {
        char c = 0;
        if (!readMem(at + i, &c, 1) || !c) break;
        s += c;
    }
    lua_pushstring(L, s.c_str());
    return 1;
}

std::vector<uintptr_t> scan(const std::vector<Range>& ranges, const scanner::Pattern& p, size_t limit) {
    std::vector<uintptr_t> hits;
    size_t n = p.bytes.size();
    for (auto& r : ranges) {
        walk(r.start, r.size, n, [&](uintptr_t base, const uint8_t* data, size_t count, size_t span) {
            for (size_t i = 0; i < span && i + n <= count; i++) {
                if (data[i] != p.bytes[0]) continue;
                bool ok = true;
                for (size_t k = 1; k < n && ok; k++) ok = !p.mask[k] || data[i + k] == p.bytes[k];
                if (ok) hits.push_back(base + i);
                if (hits.size() >= limit) return true;
            }
            return false;
        });
        if (hits.size() >= limit) break;
    }
    return hits;
}

int pushList(lua_State* L, const std::vector<uintptr_t>& list) {
    lua_newtable(L);
    for (size_t i = 0; i < list.size(); i++) {
        push(L, list[i]);
        lua_rawseti(L, -2, static_cast<int>(i) + 1);
    }
    return 1;
}

int lFind(lua_State* L) {
    auto p = scanner::parse(luaL_checkstring(L, 1));
    if (!p) return luaL_error(L, "bad pattern");
    size_t limit = static_cast<size_t>(luaL_optinteger(L, 2, 16));
    return pushList(L, scan(sections(true, false), *p, limit));
}

int lBytes(lua_State* L) {
    size_t len = 0;
    const char* text = luaL_checklstring(L, 1, &len);
    bool wide = lua_toboolean(L, 2);
    size_t limit = static_cast<size_t>(luaL_optinteger(L, 3, 16));
    std::string hex;
    char tmp[8];
    for (size_t i = 0; i < len; i++) {
        std::snprintf(tmp, sizeof(tmp), "%02x ", static_cast<unsigned char>(text[i]));
        hex += tmp;
        if (wide) hex += "00 ";
    }
    auto p = scanner::parse(hex);
    if (!p) return luaL_error(L, "bad text");
    return pushList(L, scan(sections(false, true), *p, limit));
}

// rip-relative operands: the 4-byte displacement is preceded by a modrm byte with mod=00 and rm=101
int lXrefs(lua_State* L) {
    uintptr_t target = arg(L, 1);
    size_t limit = static_cast<size_t>(luaL_optinteger(L, 2, 64));
    std::vector<uintptr_t> hits;
    for (auto& r : sections(true, false)) {
        walk(r.start, r.size, 16, [&](uintptr_t base, const uint8_t* data, size_t count, size_t span) {
            for (size_t i = 1; i < span && i + 4 <= count; i++) {
                if ((data[i - 1] & 0xC7) != 0x05) continue;
                int32_t disp;
                std::memcpy(&disp, data + i, 4);
                uintptr_t after = base + i + 4;
                for (int imm : {0, 1, 2, 4}) {
                    if (static_cast<uintptr_t>(static_cast<int64_t>(after + imm) + disp) == target) {
                        hits.push_back(base + i);
                        break;
                    }
                }
                if (hits.size() >= limit) return true;
            }
            return false;
        });
        if (hits.size() >= limit) break;
    }
    return pushList(L, hits);
}

int lCallers(lua_State* L) {
    uintptr_t target = arg(L, 1);
    size_t limit = static_cast<size_t>(luaL_optinteger(L, 2, 64));
    std::vector<uintptr_t> hits;
    for (auto& r : sections(true, false)) {
        walk(r.start, r.size, 8, [&](uintptr_t base, const uint8_t* data, size_t count, size_t span) {
            for (size_t i = 0; i < span && i + 5 <= count; i++) {
                if (data[i] != 0xE8 && data[i] != 0xE9) continue;
                int32_t disp;
                std::memcpy(&disp, data + i + 1, 4);
                if (static_cast<uintptr_t>(static_cast<int64_t>(base + i + 5) + disp) == target) hits.push_back(base + i);
                if (hits.size() >= limit) return true;
            }
            return false;
        });
        if (hits.size() >= limit) break;
    }
    return pushList(L, hits);
}

int lFunc(lua_State* L) {
    uintptr_t at = arg(L, 1);
    DWORD64 base = 0;
    auto* entry = RtlLookupFunctionEntry(at, &base, nullptr);
    if (!entry) return 0;
    for (int depth = 0; depth < 8; depth++) {
        auto* info = reinterpret_cast<const uint8_t*>(base + entry->UnwindData);
        bool chained = (info[0] >> 3) & 4;
        if (!chained) break;
        int codes = info[2];
        entry = reinterpret_cast<PRUNTIME_FUNCTION>(const_cast<uint8_t*>(info) + 4 + ((codes + 1) & ~1) * 2);
    }
    push(L, static_cast<uintptr_t>(base + entry->BeginAddress));
    push(L, static_cast<uintptr_t>(base + entry->EndAddress));
    return 2;
}

int lRtti(lua_State* L) {
    std::string name = std::string(".?AV") + luaL_checkstring(L, 1) + "@@";
    auto strings = sections(false, true);
    auto p = scanner::parse([&] {
        std::string hex;
        char tmp[8];
        for (unsigned char c : name) {
            std::snprintf(tmp, sizeof(tmp), "%02x ", c);
            hex += tmp;
        }
        hex += "00";
        return hex;
    }());
    auto names = scan(strings, *p, 4);
    lua_newtable(L);
    int out = 1;
    for (uintptr_t nameAddr : names) {
        uint32_t tdRva = static_cast<uint32_t>(nameAddr - 16 - imageBase());
        std::string hex;
        char tmp[8];
        for (int i = 0; i < 4; i++) {
            std::snprintf(tmp, sizeof(tmp), "%02x ", (tdRva >> (i * 8)) & 0xFF);
            hex += tmp;
        }
        auto locators = scan(strings, *scanner::parse(hex), 16);
        for (uintptr_t at : locators) {
            uintptr_t col = at - 12;
            uint32_t sig = 0, self = 0;
            if (!readMem(col, &sig, 4) || sig != 1 || !readMem(col + 20, &self, 4) || self != col - imageBase()) continue;
            uint32_t offset = 0;
            readMem(col + 4, &offset, 4);

            std::string colHex;
            for (int i = 0; i < 8; i++) {
                std::snprintf(tmp, sizeof(tmp), "%02x ", (col >> (i * 8)) & 0xFF);
                colHex += tmp;
            }
            for (uintptr_t slot : scan(strings, *scanner::parse(colHex), 4)) {
                lua_newtable(L);
                push(L, slot + 8);
                lua_setfield(L, -2, "vtable");
                lua_pushinteger(L, offset);
                lua_setfield(L, -2, "offset");
                push(L, col);
                lua_setfield(L, -2, "col");
                lua_rawseti(L, -2, out++);
            }
        }
    }
    return 1;
}

int lVtable(lua_State* L) {
    uintptr_t at = arg(L, 1);
    int count = static_cast<int>(luaL_optinteger(L, 2, 32));
    auto code = sections(true, false);
    lua_newtable(L);
    int out = 1;
    for (int i = 0; i < count; i++) {
        uintptr_t fn = 0;
        if (!readMem(at + i * 8, &fn, 8)) break;
        bool inCode = false;
        for (auto& r : code) inCode |= fn >= r.start && fn < r.start + r.size;
        if (!inCode) break;
        push(L, fn);
        lua_rawseti(L, -2, out++);
    }
    return 1;
}

int lHeap(lua_State* L) {
    uint64_t value = static_cast<uint64_t>(luaL_checkinteger(L, 1));
    size_t limit = static_cast<size_t>(luaL_optinteger(L, 2, 16));
    std::vector<uintptr_t> hits;
    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t at = 0x10000;
    while (hits.size() < limit && VirtualQuery(reinterpret_cast<void*>(at), &mbi, sizeof(mbi))) {
        uintptr_t next = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        bool ok = mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                  (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY));
        if (ok) {
            walk(reinterpret_cast<uintptr_t>(mbi.BaseAddress), mbi.RegionSize, 8, [&](uintptr_t base, const uint8_t* data, size_t count, size_t span) {
                for (size_t i = 0; i + 8 <= count && i < span; i += 8) {
                    uint64_t v;
                    std::memcpy(&v, data + i, 8);
                    if (v == value) hits.push_back(base + i);
                    if (hits.size() >= limit) return true;
                }
                return false;
            });
        }
        if (next <= at) break;
        at = next;
    }
    return pushList(L, hits);
}

csh capstone() {
    static csh handle = 0;
    if (!handle) {
        cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
        cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    }
    return handle;
}

bool inCode(uintptr_t at) {
    for (auto& r : sections(true, false))
        if (at >= r.start && at < r.start + r.size) return true;
    return false;
}

std::string describe(uintptr_t target) {
    char buf[96]{};
    if (inCode(target)) {
        char out[48];
        std::snprintf(out, sizeof(out), " -> fn %08llx", static_cast<unsigned long long>(target - imageBase()));
        return out;
    }
    if (!readMem(target, buf, sizeof(buf) - 1)) return {};
    int n = 0;
    while (n < 80 && buf[n] >= 0x20 && buf[n] < 0x7f) n++;
    if (n >= 4 && buf[n] == 0) return std::string(" -> \"") + std::string(buf, n) + "\"";
    int w = 0;
    while (w < 40 && buf[w * 2] >= 0x20 && buf[w * 2] < 0x7f && buf[w * 2 + 1] == 0) w++;
    if (w >= 4 && buf[w * 2] == 0 && buf[w * 2 + 1] == 0) {
        std::string s;
        for (int i = 0; i < w; i++) s += buf[i * 2];
        return " -> L\"" + s + "\"";
    }
    char out[48];
    std::snprintf(out, sizeof(out), " -> data %08llx", static_cast<unsigned long long>(target - imageBase()));
    return out;
}

std::string listing(uintptr_t start, size_t bytes, int maxInsns) {
    std::vector<uint8_t> code(bytes);
    size_t got = bytes;
    while (got && !readMem(start, code.data(), got)) got /= 2;
    std::string out;
    cs_insn* insn = cs_malloc(capstone());
    const uint8_t* p = code.data();
    size_t left = got;
    uint64_t addr = start;
    for (int i = 0; i < maxInsns && cs_disasm_iter(capstone(), &p, &left, &addr, insn); i++) {
        char line[256];
        int n = std::snprintf(line, sizeof(line), "%08llx  ", static_cast<unsigned long long>(insn->address - imageBase()));
        for (int b = 0; b < insn->size && b < 10; b++) n += std::snprintf(line + n, sizeof(line) - n, "%02x", insn->bytes[b]);
        for (int b = insn->size; b < 10; b++) n += std::snprintf(line + n, sizeof(line) - n, "  ");
        n += std::snprintf(line + n, sizeof(line) - n, "  %s %s", insn->mnemonic, insn->op_str);
        out += line;

        auto& x86 = insn->detail->x86;
        for (int o = 0; o < x86.op_count; o++) {
            auto& op = x86.operands[o];
            uintptr_t target = 0;
            if (op.type == X86_OP_MEM && op.mem.base == X86_REG_RIP) target = insn->address + insn->size + op.mem.disp;
            else if (op.type == X86_OP_IMM && (cs_insn_group(capstone(), insn, X86_GRP_CALL) || cs_insn_group(capstone(), insn, X86_GRP_JUMP)))
                target = static_cast<uintptr_t>(op.imm);
            if (target) {
                out += describe(target);
                break;
            }
        }
        out += '\n';
    }
    cs_free(insn, 1);
    return out;
}

int lDisasm(lua_State* L) {
    std::string text = listing(arg(L, 1), 15u * static_cast<size_t>(luaL_optinteger(L, 2, 40)), static_cast<int>(luaL_optinteger(L, 2, 40)));
    lua_pushstring(L, text.c_str());
    return 1;
}

int lDisFunc(lua_State* L) {
    uintptr_t at = arg(L, 1);
    DWORD64 base = 0;
    auto* entry = RtlLookupFunctionEntry(at, &base, nullptr);
    if (!entry) return luaL_error(L, "no function entry");
    uintptr_t start = static_cast<uintptr_t>(base + entry->BeginAddress);
    size_t size = entry->EndAddress - entry->BeginAddress;
    std::string text = listing(start, std::min<size_t>(size, 24000), 6000);
    lua_pushstring(L, text.c_str());
    return 1;
}

int lSleep(lua_State* L) {
    Sleep(static_cast<DWORD>(luaL_checkinteger(L, 1)));
    return 0;
}

int lLog(lua_State* L) {
    int n = lua_gettop(L);
    std::string s;
    for (int i = 1; i <= n; i++) {
        size_t len = 0;
        const char* t = luaL_tolstring(L, i, &len);
        if (i > 1) s += ' ';
        s.append(t, len);
        lua_pop(L, 1);
    }
    logger::info("explore: {}", s);
    return 0;
}

void work(std::string path) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    static const luaL_Reg api[] = {
        {"base", lBase},       {"sections", lSections}, {"hex", lHex},         {"u8", readValue<uint8_t>},
        {"u16", readValue<uint16_t>}, {"u32", readValue<uint32_t>}, {"u64", readValue<uint64_t>},
        {"i32", readValue<int32_t>},  {"f32", readValue<float>},    {"f64", readValue<double>},
        {"cstr", lCstr},       {"find", lFind},         {"bytes", lBytes},     {"xrefs", lXrefs},
        {"callers", lCallers}, {"func", lFunc},         {"rtti", lRtti},       {"vtable", lVtable},
        {"heap", lHeap},       {"sleep", lSleep},       {"log", lLog},         {"disasm", lDisasm},
        {"disfunc", lDisFunc}, {nullptr, nullptr}};
    luaL_newlib(L, api);
    lua_setglobal(L, "rt");
    lua_pushcfunction(L, lLog);
    lua_setglobal(L, "print");

    if (luaL_dofile(L, path.c_str()) != LUA_OK) logger::error("explore: {}", lua_tostring(L, -1));
    lua_close(L);
    busy = false;
}

}

void run(const std::string& path) {
    if (busy.exchange(true)) {
        logger::warn("explore: a script is still running");
        return;
    }
    std::thread(work, path).detach();
}

}
