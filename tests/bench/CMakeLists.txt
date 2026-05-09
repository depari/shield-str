cmake_minimum_required(VERSION 3.16)

# ── Benchmark: no masking (BM-01) ────────────────────────────────────────────
add_executable(bench_no_mask bench_no_mask.cpp)
target_link_libraries(bench_no_mask PRIVATE shield::shield-str benchmark::benchmark_main)

# ── Benchmark: with masking (BM-02) ──────────────────────────────────────────
add_executable(bench_with_mask bench_with_mask.cpp)
target_link_libraries(bench_with_mask PRIVATE shield::shield-str benchmark::benchmark_main)

# ── Benchmark: parallel (BM-04) ──────────────────────────────────────────────
add_executable(bench_parallel bench_parallel.cpp)
target_link_libraries(bench_parallel PRIVATE shield::shield-str benchmark::benchmark_main)
