#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>

import mo_yanxi.concurrent.atomic_double_buffer;
import std;

using namespace mo_yanxi::ccur;

static void BM_AtomicDoubleBuffer_Store(benchmark::State& state) {
    atomic_double_buffer<int> buffer;
    int val = 0;
    for (auto _ : state) {
        buffer.store(++val);
    }
}
BENCHMARK(BM_AtomicDoubleBuffer_Store);

static void BM_AtomicDoubleBuffer_Load(benchmark::State& state) {
    atomic_double_buffer<int> buffer;
    buffer.store(42);
    for (auto _ : state) {
        buffer.load([&](int& val) {
            benchmark::DoNotOptimize(val);
        });
    }
}
BENCHMARK(BM_AtomicDoubleBuffer_Load);

static void BM_AtomicDoubleBuffer_LoadLatest(benchmark::State& state) {
    atomic_double_buffer<int> buffer;
    buffer.store(42);
    for (auto _ : state) {
        buffer.load_latest([&](int& val) {
            benchmark::DoNotOptimize(val);
        });
    }
}
BENCHMARK(BM_AtomicDoubleBuffer_LoadLatest);

static atomic_double_buffer<int> spsc_buffer;

static void BM_AtomicDoubleBuffer_SPSC(benchmark::State& state) {
    if (state.thread_index() == 0) {
        int val = 0;
        for (auto _ : state) {
            spsc_buffer.store(++val);
        }
    } else {
        for (auto _ : state) {
            spsc_buffer.load([&](int& val) {
                benchmark::DoNotOptimize(val);
            });
        }
    }
}
BENCHMARK(BM_AtomicDoubleBuffer_SPSC)->Threads(2);
