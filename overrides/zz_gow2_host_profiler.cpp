// GOW2_HOST_PROFILE=<s>: sampling profiler of the host threads (Windows x64). A thread suspends every other thread of the
// process about once per millisecond, reads its instruction pointer and resumes it; every <s> seconds it prints, per
// thread with real work, the functions where the samples landed (self time, symbols from the runner's PDB through
// DbgHelp). Generated game functions show up by name (`<name>_0x<address>`), runtime functions as themselves.
// GOW2_HOST_PROFILE_TOP=<n> (default 25): functions shown per thread.
// GOW2_HOST_PROFILE_STACKS=1: also unwinds each sample's call stack (RtlVirtualUnwind, up to 24 frames, while the thread
// is suspended) and prints the inclusive time per function and the callers of the hottest self-time functions.
// GOW2_HOST_PROFILE_LINES=<text>: for the functions whose name contains <text>, the source lines their self time is on.
//
// Named zz_* so it sorts last in the runner's unity build (adding it only rebuilds the last batch). It does not include
// <windows.h>: in the unity build it would share a translation unit with raylib, whose names collide with the Win32
// ones. The few Win32 calls are declared here with the exact types of the SDK, so they stay compatible if the SDK
// headers are included anyway.
#if defined(_WIN32) && defined(_M_X64)

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct _CONTEXT;
struct tagTHREADENTRY32;
struct _SYMBOL_INFO;
struct _IMAGEHLP_LINE64;
struct _IMAGE_RUNTIME_FUNCTION_ENTRY;
struct _UNWIND_HISTORY_TABLE;
struct _KNONVOLATILE_CONTEXT_POINTERS;
extern "C"
{
    __declspec(dllimport) void *__stdcall GetCurrentProcess(void);
    __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);
    __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
    __declspec(dllimport) void *__stdcall OpenThread(unsigned long access, int inherit, unsigned long threadId);
    __declspec(dllimport) int __stdcall CloseHandle(void *handle);
    __declspec(dllimport) unsigned long __stdcall SuspendThread(void *thread);
    __declspec(dllimport) unsigned long __stdcall ResumeThread(void *thread);
    __declspec(dllimport) int __stdcall GetThreadContext(void *thread, _CONTEXT *context);
    __declspec(dllimport) void *__stdcall CreateToolhelp32Snapshot(unsigned long flags, unsigned long processId);
    __declspec(dllimport) int __stdcall Thread32First(void *snapshot, tagTHREADENTRY32 *entry);
    __declspec(dllimport) int __stdcall Thread32Next(void *snapshot, tagTHREADENTRY32 *entry);
    __declspec(dllimport) long __stdcall GetThreadDescription(void *thread, wchar_t **description);
    __declspec(dllimport) void *__stdcall LocalFree(void *mem);
    __declspec(dllimport) _IMAGE_RUNTIME_FUNCTION_ENTRY *__stdcall RtlLookupFunctionEntry(unsigned long long controlPc,
                                                                                          unsigned long long *imageBase,
                                                                                          _UNWIND_HISTORY_TABLE *history);
    __declspec(dllimport) void *__stdcall RtlVirtualUnwind(unsigned long handlerType, unsigned long long imageBase,
                                                           unsigned long long controlPc, _IMAGE_RUNTIME_FUNCTION_ENTRY *entry,
                                                           _CONTEXT *context, void **handlerData,
                                                           unsigned long long *establisherFrame,
                                                           _KNONVOLATILE_CONTEXT_POINTERS *contextPointers);
    __declspec(dllimport) unsigned long __stdcall SymSetOptions(unsigned long options);
    __declspec(dllimport) int __stdcall SymInitialize(void *process, const char *searchPath, int invadeProcess);
    __declspec(dllimport) int __stdcall SymFromAddr(void *process, unsigned long long address, unsigned long long *displacement,
                                                    _SYMBOL_INFO *symbol);
    __declspec(dllimport) int __stdcall SymGetLineFromAddr64(void *process, unsigned long long address, unsigned long *displacement,
                                                             _IMAGEHLP_LINE64 *line);
}
#pragma comment(lib, "dbghelp.lib")

namespace
{
    // THREADENTRY32 and SYMBOL_INFO, member for member.
    struct HostProfThreadEntry
    {
        unsigned long dwSize, cntUsage, th32ThreadID, th32OwnerProcessID;
        long tpBasePri, tpDeltaPri;
        unsigned long dwFlags;
    };
    struct HostProfSymbolInfo
    {
        unsigned long SizeOfStruct, TypeIndex;
        unsigned long long Reserved[2];
        unsigned long Index, Size;
        unsigned long long ModBase;
        unsigned long Flags;
        unsigned long long Value, Address;
        unsigned long Register, Scope, Tag, NameLen, MaxNameLen;
        char Name[1];
    };
    struct HostProfLine // IMAGEHLP_LINE64
    {
        unsigned long SizeOfStruct;
        void *Key;
        unsigned long LineNumber;
        char *FileName;
        unsigned long long Address;
    };
    constexpr unsigned long kHostProfThreadAccess = 0x0002u | 0x0008u | 0x0800u; // SUSPEND_RESUME | GET_CONTEXT | QUERY_LIMITED
    constexpr unsigned long kHostProfContextControl = 0x00100001u;                 // CONTEXT_AMD64 | CONTEXT_CONTROL
    constexpr unsigned long kHostProfContextFull = 0x0010000Bu;                    // | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT
    constexpr size_t kHostProfContextSize = 0x4D0u, kHostProfContextFlags = 0x30u, kHostProfContextRsp = 0x98u,
                     kHostProfContextRip = 0xF8u;
    constexpr size_t kHostProfFrames = 24u;

    using HostProfStack = std::array<uint64_t, kHostProfFrames>; // frame 0 = rip; 0 ends the stack

    struct HostProfThread
    {
        unsigned long id = 0;
        void *handle = nullptr;
        std::string name;
        uint64_t samples = 0;
        std::unordered_map<uint64_t, uint32_t> rips;
        std::vector<HostProfStack> stacks;
    };

    std::string hostProfThreadName(void *handle, unsigned long id)
    {
        std::string name;
        wchar_t *desc = nullptr;
        if (GetThreadDescription(handle, &desc) >= 0 && desc != nullptr)
        {
            for (const wchar_t *p = desc; *p; ++p)
                name.push_back(*p < 128 ? static_cast<char>(*p) : '?');
            LocalFree(desc);
        }
        if (name.empty())
            name = "tid " + std::to_string(id);
        return name;
    }

    void hostProfRefreshThreads(std::vector<HostProfThread> &threads)
    {
        void *snap = CreateToolhelp32Snapshot(0x4u /* TH32CS_SNAPTHREAD */, 0u);
        if (snap == reinterpret_cast<void *>(-1))
            return;
        const unsigned long pid = GetCurrentProcessId(), self = GetCurrentThreadId();
        HostProfThreadEntry e{};
        e.dwSize = sizeof(e);
        for (int ok = Thread32First(snap, reinterpret_cast<tagTHREADENTRY32 *>(&e)); ok;
             ok = Thread32Next(snap, reinterpret_cast<tagTHREADENTRY32 *>(&e)))
        {
            if (e.th32OwnerProcessID != pid || e.th32ThreadID == self)
                continue;
            if (std::any_of(threads.begin(), threads.end(), [&](const HostProfThread &t) { return t.id == e.th32ThreadID; }))
                continue;
            if (void *h = OpenThread(kHostProfThreadAccess, 0, e.th32ThreadID))
            {
                HostProfThread t;
                t.id = e.th32ThreadID;
                t.handle = h;
                t.name = hostProfThreadName(h, t.id);
                threads.push_back(std::move(t));
            }
        }
        CloseHandle(snap);
    }

    // Walks the suspended thread's stack from its context. Only the unwinder runs here: no allocation, no printing.
    void hostProfUnwind(unsigned char *context, HostProfStack &stack)
    {
        stack.fill(0u);
        for (size_t i = 0; i < kHostProfFrames; ++i)
        {
            uint64_t rip, rsp;
            std::memcpy(&rip, context + kHostProfContextRip, 8);
            std::memcpy(&rsp, context + kHostProfContextRsp, 8);
            if (rip == 0u || rsp == 0u)
                break;
            stack[i] = rip;
            unsigned long long imageBase = 0;
            _IMAGE_RUNTIME_FUNCTION_ENTRY *entry = RtlLookupFunctionEntry(rip, &imageBase, nullptr);
            if (entry != nullptr)
            {
                void *handlerData = nullptr;
                unsigned long long establisher = 0;
                RtlVirtualUnwind(0u /* UNW_FLAG_NHANDLER */, imageBase, rip, entry, reinterpret_cast<_CONTEXT *>(context),
                                 &handlerData, &establisher, nullptr);
            }
            else if (i == 0u)
            {
                // leaf function without unwind data (only the innermost frame can be one): the return address is on
                // top of the stack
                uint64_t ret;
                std::memcpy(&ret, reinterpret_cast<const void *>(rsp), 8);
                rsp += 8u;
                std::memcpy(context + kHostProfContextRip, &ret, 8);
                std::memcpy(context + kHostProfContextRsp, &rsp, 8);
            }
            else
                break; // no unwind data above the innermost frame: the rest of the stack can't be trusted
        }
    }

    const std::string &hostProfSymbol(uint64_t rip, std::unordered_map<uint64_t, std::string> &cache)
    {
        auto hit = cache.find(rip);
        if (hit != cache.end())
            return hit->second;
        alignas(8) char buffer[sizeof(HostProfSymbolInfo) + 512];
        std::memset(buffer, 0, sizeof(buffer));
        auto *info = reinterpret_cast<HostProfSymbolInfo *>(buffer);
        info->SizeOfStruct = sizeof(HostProfSymbolInfo);
        info->MaxNameLen = 511;
        unsigned long long displacement = 0;
        std::string name;
        if (SymFromAddr(GetCurrentProcess(), rip, &displacement, reinterpret_cast<_SYMBOL_INFO *>(info)))
            name = info->Name;
        else
        {
            char hex[32];
            std::snprintf(hex, sizeof(hex), "?%llx", static_cast<unsigned long long>(rip & ~0xFFFull));
            name = hex;
        }
        return cache.emplace(rip, std::move(name)).first->second;
    }

    bool hostProfIsWait(const std::string &name) { return name.rfind("Nt", 0) == 0 || name.rfind("Zw", 0) == 0; }

    void hostProfPrintTop(const char *what, std::unordered_map<std::string, uint64_t> &counts, uint64_t samples, size_t top)
    {
        std::vector<std::pair<uint64_t, std::string>> sorted;
        for (auto &[name, n] : counts)
            sorted.emplace_back(n, name);
        std::sort(sorted.rbegin(), sorted.rend());
        std::fprintf(stderr, "[host-prof]   -- %s\n", what);
        for (size_t i = 0; i < sorted.size() && i < top; ++i)
            std::fprintf(stderr, "[host-prof]   %5.1f%%  %s\n", 100.0 * static_cast<double>(sorted[i].first) / static_cast<double>(samples),
                         sorted[i].second.c_str());
    }

    std::string hostProfLine(uint64_t rip)
    {
        HostProfLine line{};
        line.SizeOfStruct = sizeof(line);
        unsigned long displacement = 0;
        if (!SymGetLineFromAddr64(GetCurrentProcess(), rip, &displacement, reinterpret_cast<_IMAGEHLP_LINE64 *>(&line)) ||
            line.FileName == nullptr)
            return "?";
        const char *file = line.FileName;
        for (const char *p = line.FileName; *p; ++p)
            if (*p == '\\' || *p == '/')
                file = p + 1;
        return std::string(file) + ":" + std::to_string(line.LineNumber);
    }

    void hostProfReport(std::vector<HostProfThread> &threads, double seconds, size_t top,
                        std::unordered_map<uint64_t, std::string> &cache, const char *linesOf)
    {
        std::fprintf(stderr, "[host-prof] window %.1f s\n", seconds);
        for (HostProfThread &t : threads)
        {
            if (t.samples == 0)
                continue;
            std::unordered_map<std::string, uint64_t> self;
            for (const auto &[rip, n] : t.rips)
                self[hostProfSymbol(rip, cache)] += n;
            uint64_t waiting = 0;
            for (auto &[name, n] : self)
                if (hostProfIsWait(name)) // kernel waits and sleeps: the thread had nothing to do
                    waiting += n;
            const double busy = 100.0 * static_cast<double>(t.samples - waiting) / static_cast<double>(t.samples);
            if (busy >= 3.0)
            {
                std::fprintf(stderr, "[host-prof] thread %s: %llu samples, %.1f%% busy\n", t.name.c_str(),
                             static_cast<unsigned long long>(t.samples), busy);
                hostProfPrintTop("self", self, t.samples, top);
                if (linesOf != nullptr)
                {
                    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> lines;
                    for (const auto &[rip, n] : t.rips)
                    {
                        const std::string &name = hostProfSymbol(rip, cache);
                        if (name.find(linesOf) != std::string::npos)
                            lines[name][hostProfLine(rip)] += n;
                    }
                    for (auto &[name, byLine] : lines)
                    {
                        std::string what = "lines of " + name.substr(0, 80);
                        hostProfPrintTop(what.c_str(), byLine, t.samples, 20u);
                    }
                }
                if (!t.stacks.empty())
                {
                    // inclusive: every function on the stack, once per sample
                    std::unordered_map<std::string, uint64_t> inclusive;
                    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> callers;
                    std::unordered_set<const std::string *> seen;
                    for (const HostProfStack &s : t.stacks)
                    {
                        seen.clear();
                        const std::string *leaf = nullptr;
                        for (size_t i = 0; i < kHostProfFrames && s[i] != 0u; ++i)
                        {
                            // return addresses point after the call: look the symbol up one byte back
                            const std::string &name = hostProfSymbol(i == 0 ? s[i] : s[i] - 1u, cache);
                            if (seen.insert(&name).second)
                                ++inclusive[name];
                            if (i == 0)
                                leaf = &name;
                            else if (leaf != nullptr && name != *leaf)
                            {
                                ++callers[*leaf][name];
                                leaf = nullptr;
                            }
                        }
                    }
                    hostProfPrintTop("inclusive", inclusive, t.samples, top);
                    std::vector<std::pair<uint64_t, std::string>> hot;
                    for (auto &[name, n] : self)
                        if (!hostProfIsWait(name))
                            hot.emplace_back(n, name);
                    std::sort(hot.rbegin(), hot.rend());
                    for (size_t i = 0; i < hot.size() && i < 10u; ++i)
                    {
                        std::string what = "callers of " + hot[i].second.substr(0, 80);
                        hostProfPrintTop(what.c_str(), callers[hot[i].second], t.samples, 4u);
                    }
                }
            }
            t.samples = 0;
            t.rips.clear();
            t.stacks.clear();
        }
    }

    void hostProfMain(double interval, size_t top, bool stacks, const char *linesOf)
    {
        SymSetOptions(0x2u | 0x4u | 0x10u); // SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES
        SymInitialize(GetCurrentProcess(), nullptr, 1);
        std::vector<HostProfThread> threads;
        std::unordered_map<uint64_t, std::string> cache;
        alignas(16) unsigned char context[kHostProfContextSize];
        HostProfStack stack;
        auto windowStart = std::chrono::steady_clock::now(), lastRefresh = windowStart - std::chrono::seconds(1);
        for (;;)
        {
            const auto now = std::chrono::steady_clock::now();
            if (now - lastRefresh >= std::chrono::seconds(1))
            {
                hostProfRefreshThreads(threads);
                lastRefresh = now;
            }
            for (HostProfThread &t : threads)
            {
                // Nothing that can take a lock (allocation, printing) while the thread is suspended.
                if (SuspendThread(t.handle) == static_cast<unsigned long>(-1))
                    continue;
                std::memset(context, 0, sizeof(context));
                *reinterpret_cast<unsigned long *>(context + kHostProfContextFlags) =
                    stacks ? kHostProfContextFull : kHostProfContextControl;
                const int ok = GetThreadContext(t.handle, reinterpret_cast<_CONTEXT *>(context));
                uint64_t rip = 0;
                if (ok)
                {
                    std::memcpy(&rip, context + kHostProfContextRip, sizeof(rip));
                    if (stacks)
                        hostProfUnwind(context, stack);
                }
                ResumeThread(t.handle);
                if (!ok)
                    continue;
                ++t.rips[rip];
                ++t.samples;
                if (stacks)
                    t.stacks.push_back(stack);
            }
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - windowStart).count();
            if (elapsed >= interval)
            {
                hostProfReport(threads, elapsed, top, cache, linesOf);
                windowStart = std::chrono::steady_clock::now();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    const bool g_hostProfStarted = []
    {
        const char *env = std::getenv("GOW2_HOST_PROFILE");
        if (env == nullptr || std::atof(env) <= 0.0)
            return false;
        const char *topEnv = std::getenv("GOW2_HOST_PROFILE_TOP");
        const size_t top = topEnv ? static_cast<size_t>(std::atoi(topEnv)) : 25u;
        const char *stacksEnv = std::getenv("GOW2_HOST_PROFILE_STACKS");
        const bool stacks = stacksEnv != nullptr && stacksEnv[0] == '1';
        const char *linesOf = std::getenv("GOW2_HOST_PROFILE_LINES");
        std::thread(hostProfMain, std::atof(env), top, stacks, linesOf).detach();
        std::fprintf(stderr, "[host-prof] sampling every ~1 ms, report every %s s%s\n", env, stacks ? ", with call stacks" : "");
        return true;
    }();
}

#endif
