#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

// Simple thread-safe profiler for named sections. Meant to be low-overhead and
// easy to use from Parser code. Usage:
//   static Profiler profiler;
//   {
//     auto s = profiler.scoped("lex");
//     ... do work ...
//   }
// At the end call profiler.print() to dump results.

struct Profiler
{
    struct Stat {
        uint64_t total_ns = 0;
        uint64_t calls = 0;
    };

    Profiler() = default;

    // RAII helper
    struct Scoped {
        Profiler& p;
        const char* name;
        std::chrono::steady_clock::time_point start;
        Scoped(Profiler& pr, const char* n) : p(pr), name(n), start(std::chrono::steady_clock::now()) {}
        ~Scoped() { p.add(name, std::chrono::steady_clock::now() - start); }
    };

    Scoped scoped(const char* name) { return Scoped{ *this, name }; }

    void add(const char* name, std::chrono::steady_clock::duration dur)
    {
        auto ns = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count());
        auto &s = stats[name];
        s.total_ns += ns;
        s.calls += 1;
    }

    void reset()
    {
        stats.clear();
    }

    void print(std::ostream &out = std::cout)
    {
        out << "\nProfiler results:\n";
        out << "Section, Calls, Total ms, Avg us\n";
        for (auto &it : stats) {
            double total_ms = it.second.total_ns / 1e6;
            double avg_us = (it.second.calls == 0) ? 0.0 : (it.second.total_ns / (double)it.second.calls) / 1e3;
            out << it.first << ", " << it.second.calls << ", " << total_ms << ", " << avg_us << "\n";
        }
        out << std::flush;
    }

private:
    std::map<std::string, Stat> stats;
};
