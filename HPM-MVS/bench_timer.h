// bench_timer.h -- phase-boundary trace for the MVS benchmarking harness.
//
// Deviation class: instrumentation. This file is vendored byte-identically into
// every campaign fork; change it in one fork and copy it to the others, never
// let the copies diverge.
//
//   compile out   -DBENCH_PHASES=0      every macro expands to nothing
//   enable        MVS_BENCH_PHASES=1    otherwise the scopes take one
//                                       always-false branch and emit nothing
//   sink          MVS_BENCH_FILE=<path> falls back to stderr
//
// Line format, one per boundary:
//
//   PHASE <name> BEGIN|END <CLOCK_MONOTONIC ns> [key=value ...]
//
// The sink is line buffered so that the trace of a run that segfaults, is
// killed by the timeout, or runs out of memory survives up to the last
// completed boundary.
//
// BENCH_PHASE(name, ...) opens a span that closes when its block does. Where a
// phase ends part-way through a scope whose locals are still live -- HPM-MVS's
// per-level prior blocks -- BENCH_PHASE_BEGIN/BENCH_PHASE_END name the scope
// and close it explicitly, so the span boundaries stay where the phase survey
// puts them instead of where C++ scoping happens to allow.
//
// BENCH_SYNC() is for CUMVS, the only implementation whose kernel launches are
// not all followed by a device synchronisation; everywhere else a host
// timestamp is already GPU-truthful and adding a barrier would change what is
// measured. It expands to a synchronise only when MVS_BENCH_SYNC is set, and it
// is deliberately undefined unless <cuda_runtime.h> was included first, so a
// translation unit that cannot synchronise fails to compile instead of
// silently timestamping an asynchronous launch.

#pragma once

#if !defined(BENCH_PHASES) || BENCH_PHASES

#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace bench {

inline bool enabled()
{
    static const bool on = [] {
        const char *value = getenv("MVS_BENCH_PHASES");
        return value && *value && *value != '0';
    }();
    return on;
}

inline bool sync_requested()
{
    static const bool on = [] {
        const char *value = getenv("MVS_BENCH_SYNC");
        return value && *value && *value != '0';
    }();
    return on;
}

inline FILE *sink()
{
    static FILE *const file = [] {
        const char *path = getenv("MVS_BENCH_FILE");
        FILE *handle = (path && *path) ? fopen(path, "w") : nullptr;
        if (!handle) {
            handle = stderr;
        }
        setvbuf(handle, nullptr, _IOLBF, BUFSIZ);
        return handle;
    }();
    return file;
}

inline long long monotonic_ns()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
}

inline void emit(const char *name, const char *edge, const char *attributes)
{
    fprintf(sink(), "PHASE %s %s %lld%s%s\n", name, edge, monotonic_ns(),
            (attributes && *attributes) ? " " : "", attributes ? attributes : "");
}

// The attribute string is a member array, so the only allocation the timer ever
// makes is the sink's, on the first event.
class Scope
{
public:
    explicit Scope(const char *name) : name_(name)
    {
        attributes_[0] = '\0';
        if (enabled()) {
            emit(name_, "BEGIN", attributes_);
        }
    }

    template <typename... Args>
    Scope(const char *name, const char *format, Args... args) : name_(name)
    {
        attributes_[0] = '\0';
        if (!enabled()) {
            return;
        }
        snprintf(attributes_, sizeof(attributes_), format, args...);
        emit(name_, "BEGIN", attributes_);
    }

    // Closes the span before the enclosing scope does. Idempotent.
    void end()
    {
        if (open_ && enabled()) {
            emit(name_, "END", attributes_);
        }
        open_ = false;
    }

    ~Scope() { end(); }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;

private:
    const char *name_;
    bool open_ = true;
    char attributes_[192];
};

} // namespace bench

#define BENCH_JOIN_(a, b) a##b
#define BENCH_JOIN(a, b) BENCH_JOIN_(a, b)
#define BENCH_PHASE(name, ...) \
    ::bench::Scope BENCH_JOIN(bench_scope_, __LINE__)(name, ##__VA_ARGS__)
#define BENCH_PHASE_BEGIN(scope, name, ...) ::bench::Scope scope(name, ##__VA_ARGS__)
#define BENCH_PHASE_END(scope) (scope).end()

#ifdef CUDART_VERSION
#define BENCH_SYNC()                        \
    do {                                    \
        if (::bench::sync_requested()) {    \
            cudaDeviceSynchronize();        \
        }                                   \
    } while (0)
#endif

#else

#define BENCH_PHASE(name, ...) ((void)0)
#define BENCH_PHASE_BEGIN(scope, name, ...) ((void)0)
#define BENCH_PHASE_END(scope) ((void)0)
#define BENCH_SYNC() ((void)0)

#endif
