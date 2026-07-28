#!/usr/bin/env bash
set -euo pipefail

height="${1:-720}"
width="${2:-960}"
max_iterations="${3:-2048}"
repetitions="${4:-3}"
output="${5:-results/mandelbrot.csv}"
thread_counts="${THREAD_COUNTS:-1 2 4 8 16 32}"
allocations="${ALLOCATIONS:-static dynamic}"

read -r -a launcher <<< "${LAUNCHER:-}"
mkdir -p "$(dirname "$output")"
printf "height,width,max_iterations,allocation,threads,repetition,elapsed_seconds,member_pixels\n" > "$output"

for allocation in $allocations; do
    for threads in $thread_counts; do
        for repetition in $(seq 1 "$repetitions"); do
            run_output="$(
                "${launcher[@]}" ./mandelbrot-parallel \
                    --height "$height" \
                    --width "$width" \
                    --max-iterations "$max_iterations" \
                    --work-allocation "$allocation" \
                    --num-threads "$threads" \
                    --print-level 1 \
                    --no-output
            )"
            elapsed="$(awk '/^Elapsed time:/ {print $3}' <<< "$run_output")"
            member_pixels="$(
                awk '/^Mandelbrot-set pixels:/ {print $3}' <<< "$run_output"
            )"
            printf "%s,%s,%s,%s,%s,%s,%s,%s\n" \
                "$height" "$width" "$max_iterations" "$allocation" \
                "$threads" "$repetition" "$elapsed" "$member_pixels" >> "$output"
        done
    done
done

echo "Wrote $output"
