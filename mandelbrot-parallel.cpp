#include "mandelbrot-common.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace {

struct ParallelResult {
    std::size_t member_count;
    int threads;
};

ParallelResult render_static(
    mandelbrot::Image& image,
    int requested_threads,
    int max_iterations
) {
    const int thread_count = std::min(
        requested_threads,
        static_cast<int>(
            std::min<std::size_t>(
                image.height(),
                static_cast<std::size_t>(std::numeric_limits<int>::max())
            )
        )
    );
    const std::size_t base_rows =
        image.height() / static_cast<std::size_t>(thread_count);
    const std::size_t extra_rows =
        image.height() % static_cast<std::size_t>(thread_count);

    std::vector<std::size_t> counts(
        static_cast<std::size_t>(thread_count),
        0
    );
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(thread_count));

    std::size_t first_row = 0;
    for (int thread = 0; thread < thread_count; ++thread) {
        const std::size_t rows =
            base_rows + (static_cast<std::size_t>(thread) < extra_rows ? 1 : 0);
        const std::size_t begin = first_row;
        const std::size_t end = begin + rows;
        first_row = end;

        workers.emplace_back([&, thread, begin, end]() {
            std::size_t local_count = 0;
            for (std::size_t row = begin; row < end; ++row) {
                local_count +=
                    mandelbrot::render_row(image, row, max_iterations);
            }
            counts[static_cast<std::size_t>(thread)] = local_count;
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }
    return {
        std::accumulate(counts.begin(), counts.end(), std::size_t{0}),
        thread_count
    };
}

ParallelResult render_dynamic(
    mandelbrot::Image& image,
    int requested_threads,
    int max_iterations
) {
    const int thread_count = std::min(
        requested_threads,
        static_cast<int>(
            std::min<std::size_t>(
                image.height(),
                static_cast<std::size_t>(std::numeric_limits<int>::max())
            )
        )
    );

    std::atomic<std::size_t> next_row{0};
    std::vector<std::size_t> counts(
        static_cast<std::size_t>(thread_count),
        0
    );
    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(thread_count));

    for (int thread = 0; thread < thread_count; ++thread) {
        workers.emplace_back([&, thread]() {
            std::size_t local_count = 0;
            while (true) {
                const std::size_t row =
                    next_row.fetch_add(1, std::memory_order_relaxed);
                if (row >= image.height()) {
                    break;
                }
                local_count +=
                    mandelbrot::render_row(image, row, max_iterations);
            }
            counts[static_cast<std::size_t>(thread)] = local_count;
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }
    return {
        std::accumulate(counts.begin(), counts.end(), std::size_t{0}),
        thread_count
    };
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const mandelbrot::Config config =
            mandelbrot::parse_arguments(argc, argv);
        mandelbrot::Image image(config.height, config.width);

        const auto start = std::chrono::steady_clock::now();
        const ParallelResult result =
            config.allocation == "static"
                ? render_static(image, config.threads, config.max_iterations)
                : render_dynamic(image, config.threads, config.max_iterations);
        const auto stop = std::chrono::steady_clock::now();

        mandelbrot::print_result(
            config.allocation,
            config,
            result.member_count,
            std::chrono::duration<double>(stop - start).count()
        );
        if (config.print_level >= 2) {
            std::cout << "Threads: " << result.threads << '\n';
        }

        if (config.verify) {
            mandelbrot::Image reference(config.height, config.width);
            const std::size_t reference_count =
                mandelbrot::render_sequential(
                    reference,
                    config.max_iterations
                );
            const bool verified =
                result.member_count == reference_count &&
                image == reference;
            std::cout << "Verification: "
                      << (verified ? "OK" : "NOT OK") << '\n';
            if (!verified) {
                return 2;
            }
        }

        if (config.save_output) {
            image.save_ppm(config.output_path);
            if (config.print_level >= 2) {
                std::cout << "Output: " << config.output_path << '\n';
            }
        }
        return 0;
    } catch (const mandelbrot::HelpRequested&) {
        mandelbrot::print_usage(std::cout, argv[0]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        mandelbrot::print_usage(std::cerr, argv[0]);
        return 1;
    }
}
