#ifndef MANDELBROT_COMMON_HPP
#define MANDELBROT_COMMON_HPP

#include <array>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace mandelbrot {

struct HelpRequested {};

struct Config {
    int threads = 1;
    std::string allocation = "static";
    std::size_t height = 720;
    std::size_t width = 960;
    int max_iterations = 2048;
    int print_level = 2;
    bool save_output = true;
    bool verify = false;
    std::string output_path = "mandelbrot.ppm";
};

inline void print_usage(std::ostream& output, const char* program) {
    output
        << "Usage: " << program << " [options]\n"
        << "\nOptions:\n"
        << "  --num-threads <integer>             Worker threads (default: 1)\n"
        << "  --work-allocation <static|dynamic>  Row scheduling (default: static)\n"
        << "  --height <integer>                  Image height (default: 720)\n"
        << "  --width <integer>                   Image width (default: 960)\n"
        << "  --max-iterations <integer>          Iteration limit per pixel (default: 2048)\n"
        << "  --print-level <0|1|2>               Output detail (default: 2)\n"
        << "  --output <path>                     Output PPM path\n"
        << "  --no-output                         Do not write an image\n"
        << "  --verify                            Compare parallel output with sequential output\n"
        << "  --help                              Show this help message\n";
}

inline int parse_positive_int(const std::string& value, const std::string& option) {
    std::size_t parsed_characters = 0;
    long parsed = 0;
    try {
        parsed = std::stol(value, &parsed_characters);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    if (parsed_characters != value.size() || parsed <= 0 ||
        parsed > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    return static_cast<int>(parsed);
}

inline std::size_t parse_size(const std::string& value, const std::string& option) {
    std::size_t parsed_characters = 0;
    unsigned long long parsed = 0;
    try {
        parsed = std::stoull(value, &parsed_characters);
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    if (parsed_characters != value.size() || parsed == 0 ||
        parsed > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("Invalid value for " + option + ": " + value);
    }
    return static_cast<std::size_t>(parsed);
}

inline Config parse_arguments(int argc, char** argv) {
    Config config;

    for (int argument = 1; argument < argc; ++argument) {
        const std::string option = argv[argument];
        auto next_value = [&]() -> std::string {
            if (argument + 1 >= argc) {
                throw std::invalid_argument("Missing value after " + option);
            }
            return argv[++argument];
        };

        if (option == "--num-threads") {
            config.threads = parse_positive_int(next_value(), option);
        } else if (option == "--work-allocation") {
            config.allocation = next_value();
        } else if (option == "--height") {
            config.height = parse_size(next_value(), option);
        } else if (option == "--width") {
            config.width = parse_size(next_value(), option);
        } else if (option == "--max-iterations") {
            config.max_iterations = parse_positive_int(next_value(), option);
        } else if (option == "--print-level") {
            const std::string value = next_value();
            std::size_t parsed_characters = 0;
            try {
                config.print_level = std::stoi(value, &parsed_characters);
            } catch (const std::exception&) {
                throw std::invalid_argument(
                    "Invalid value for " + option + ": " + value
                );
            }
            if (parsed_characters != value.size() ||
                config.print_level < 0 || config.print_level > 2) {
                throw std::invalid_argument(
                    "--print-level must be 0, 1, or 2"
                );
            }
        } else if (option == "--output") {
            config.output_path = next_value();
            if (config.output_path.empty()) {
                throw std::invalid_argument("--output cannot be empty");
            }
        } else if (option == "--no-output") {
            config.save_output = false;
        } else if (option == "--verify") {
            config.verify = true;
        } else if (option == "--help") {
            throw HelpRequested{};
        } else {
            throw std::invalid_argument("Unknown option: " + option);
        }
    }

    if (config.allocation != "static" && config.allocation != "dynamic") {
        throw std::invalid_argument(
            "--work-allocation must be 'static' or 'dynamic'"
        );
    }
    return config;
}

using Pixel = std::array<std::uint8_t, 3>;

class Image {
public:
    Image(std::size_t height, std::size_t width, Pixel value = {0, 0, 0})
        : height_(height),
          width_(width),
          pixels_(checked_pixel_count(height, width), value) {}

    Pixel& operator()(std::size_t row, std::size_t column) {
        return pixels_[row * width_ + column];
    }

    const Pixel& operator()(std::size_t row, std::size_t column) const {
        return pixels_[row * width_ + column];
    }

    std::size_t height() const {
        return height_;
    }

    std::size_t width() const {
        return width_;
    }

    bool operator==(const Image& other) const {
        return height_ == other.height_ &&
               width_ == other.width_ &&
               pixels_ == other.pixels_;
    }

    void save_ppm(const std::string& path) const {
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Could not open output file: " + path);
        }

        output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Pixel& pixel : pixels_) {
            output.write(
                reinterpret_cast<const char*>(pixel.data()),
                static_cast<std::streamsize>(pixel.size())
            );
        }
        if (!output) {
            throw std::runtime_error("Could not write output file: " + path);
        }
    }

private:
    static std::size_t checked_pixel_count(
        std::size_t height,
        std::size_t width
    ) {
        if (height != 0 &&
            width > std::numeric_limits<std::size_t>::max() / height) {
            throw std::length_error("Image dimensions are too large");
        }
        return height * width;
    }

    std::size_t height_;
    std::size_t width_;
    std::vector<Pixel> pixels_;
};

struct PixelResult {
    bool is_member;
    int iterations;
};

inline PixelResult evaluate(
    const std::complex<double>& point,
    int max_iterations
) {
    std::complex<double> value(0.0, 0.0);
    int iterations = 0;
    while (std::norm(value) <= 4.0 && iterations < max_iterations) {
        value = value * value + point;
        ++iterations;
    }
    return {iterations == max_iterations, iterations};
}

inline std::size_t render_row(
    Image& image,
    std::size_t row,
    int max_iterations
) {
    std::size_t member_count = 0;
    const double y =
        (static_cast<double>(row) / static_cast<double>(image.height()) - 0.5)
        * 2.0;

    for (std::size_t column = 0; column < image.width(); ++column) {
        const double x =
            (static_cast<double>(column) /
             static_cast<double>(image.width()) - 0.75) * 2.0;
        const PixelResult result =
            evaluate(std::complex<double>(x, y), max_iterations);

        if (result.is_member) {
            image(row, column) = {0, 0, 0};
            ++member_count;
        } else {
            image(row, column) = {255, 255, 255};
        }
    }
    return member_count;
}

inline std::size_t render_sequential(Image& image, int max_iterations) {
    std::size_t member_count = 0;
    for (std::size_t row = 0; row < image.height(); ++row) {
        member_count += render_row(image, row, max_iterations);
    }
    return member_count;
}

inline void print_result(
    const std::string& mode,
    const Config& config,
    std::size_t member_count,
    double elapsed_seconds
) {
    if (config.print_level >= 2) {
        std::cout << "Mode: " << mode << '\n';
    }
    if (config.print_level >= 1) {
        std::cout << "Mandelbrot-set pixels: " << member_count << '\n';
    }
    std::cout << "Elapsed time: " << elapsed_seconds << " seconds\n";
}

}  // namespace mandelbrot

#endif
