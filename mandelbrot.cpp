#include "mandelbrot-common.hpp"

#include <chrono>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const mandelbrot::Config config =
            mandelbrot::parse_arguments(argc, argv);
        mandelbrot::Image image(config.height, config.width);

        const auto start = std::chrono::steady_clock::now();
        const std::size_t member_count =
            mandelbrot::render_sequential(image, config.max_iterations);
        const auto stop = std::chrono::steady_clock::now();

        mandelbrot::print_result(
            "sequential",
            config,
            member_count,
            std::chrono::duration<double>(stop - start).count()
        );

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
