#include <CLI/CLI.hpp> // WHY: External library for easy command-line argument parsing.
#include <atomic>
#include <iostream>
#include <optional>
#include <string>
#include <thread> // WHY: For processing multiple images concurrently.
#include <vector>

#include "lut.hh"
#include "process.hh"

int main(int argc, char **argv)
{
    // Define your application's version string (consider making this easily updatable)
    const std::string APP_VERSION{"1.0.0"};

    // Define command line options and flags using CLI11.
    // --- App Information ---
    CLI::App app_parser{"Lab chroma ratio detector"};
    app_parser.description("Detects color ratio based on LAB chroma"); // Optional: Add description

    std::vector<std::string> image_filenames;
    // WHY float? Chroma threshold can be non-integer. Default 5 is common for distinguishing gray.
    float chroma_threshold{5.f};
    bool use_sepia_preset{false};  // WHY bool? Simple flag for a specific preset.
    bool file_names_only{false};   // WHY bool? Flag to change output column order.
    bool output_max_chroma{false}; // WHY bool? Flag to output max chroma instead of ratio.
    int dump_lut_at_threshold{0};  // WHY int? Threshold for dumping is integer; 0 means disabled.
    std::optional<float> greater_than{std::nullopt};
    std::optional<float> less_than{std::nullopt};
    bool sort_results{false};

    // --- Positional Arguments ---
    app_parser.add_option("files", image_filenames, "Image files to process (required unless using --dump-lut)");

    // --- Options ---
    // Renamed from -c,--chroma for clarity, as it's the threshold value.
    app_parser.add_option("-t,--threshold", chroma_threshold, "Chroma threshold for color detection (default: 5.0)")
        ->check(CLI::PositiveNumber); // WHY check? Ensure threshold is physically meaningful.

    app_parser
        .add_option("-g,--greater-than", greater_than,
                    "List images with color ratio greater than this value (default: none)")
        ->check(CLI::PositiveNumber); // WHY check? Ensure threshold is physically meaningful.
    app_parser.add_option("-l,--less-than", less_than, "List images with color ratio less than this value (default: none)")
        ->check(CLI::PositiveNumber); // WHY check? Ensure threshold is physically meaningful.

    // Renamed from -t,--lookup-table to avoid conflict with --threshold and be more descriptive.
    app_parser.add_option("-d,--dump-lut", dump_lut_at_threshold, "Dump precomputed LUT for a threshold and exit (default: 0)");

    // --- Flags ---
    app_parser.add_flag("-s,--sepia", use_sepia_preset, "Use preset threshold=13 for sepia detection (overrides -t)");

    app_parser.add_flag("-f,--file-names-only", file_names_only, "Output only file names");

    app_parser.add_flag("-m,--max-chroma", output_max_chroma, "Output max chroma value instead of color ratio");

    app_parser.add_flag("-r,--reverse-sort", sort_results, "Sort results by value descending (stable sort)");

    // --- Standard Flags ---
    // WHY add_flag_function? Provides a way to execute code (print version, exit) when flag is detected.
    // The callback takes the count of the flag occurrences.
    app_parser.add_flag_function(
        "-v,--version",          // Flag names
        [&](std::size_t count) { // Lambda callback function
            // WHY check count? Ensure the callback action only happens if the flag was actually passed.
            if (count > 0) {
                // Print app name (if set) and version string.
                std::cout << (app_parser.get_name().empty() ? "App" : app_parser.get_name()) << " version " << APP_VERSION
                          << std::endl;
                // WHY exit(0)? Standard practice to exit cleanly after printing version info.
                exit(0);
            }
        },
        "Print version information and exit" // Help description
    );

    // --- Parse Arguments ---
    try {
        // Allow extras initially to prevent error if only -d is passed without files
        // app_parser.allow_extras(); // Alternative: handle error differently
        CLI11_PARSE(app_parser, argc, argv);
    } catch (const CLI::ParseError &e) {
        // If files are missing AND -d wasn't given, the custom check below will handle it.
        // Otherwise, let CLI11 handle other parsing errors.
        if (app_parser.count("files") == 0 && app_parser.count("--dump-lut") == 0) {
            // Let the custom check below handle the missing files error
            // We might reach here if other parse errors occurred first though
        }
        return app_parser.exit(e); // Exit with CLI11's error message
    }

    // --- Post-Parsing Logic ---

    bool dump_mode = app_parser.count("--dump-lut") > 0;

    // Handle default value for --dump-lut if flag present but value omitted
    // If -d was given, but the variable is still its initial value (0), set it to 5.
    // Note: This assumes the user doesn't explicitly pass "-d 0". If that's valid,
    // this logic needs adjustment (e.g., using ->expected(0,1) might be needed,
    // but let's stick to this simpler logic first).
    if (dump_mode && dump_lut_at_threshold == 0) {
        dump_lut_at_threshold = 5; // Set default dump threshold to 5
    }

    // If in dump mode, execute dump and exit
    if (dump_mode) {
        // Ensure a valid threshold (either default 5 or user-provided)
        if (dump_lut_at_threshold <= 0) {
            std::cerr << "ERROR: Invalid threshold specified for --dump-lut: " << dump_lut_at_threshold << std::endl;
            return 1; // Exit with error
        }
        dump_lookup_table(dump_lut_at_threshold);
        return 0; // Exit successfully after dumping LUT
    }

    // Enforce 'files' requirement *only* if not in dump mode
    if (!dump_mode && image_filenames.empty()) {
        std::cerr << "ERROR: Input files are required when not using --dump-lut." << std::endl;
        // Print help manually or exit (CLI11 might not print help here easily)
        std::cout << app_parser.help() << std::endl; // Try printing help
        return 1;                                    // Exit with error
    }

    // Override threshold for sepia preset (only if not dumping)
    if (use_sepia_preset) {
        chroma_threshold = 13.f;
    }

    // --- Proceed with Image Processing (only if not in dump mode) ---

    // WHY get LUT here? Precompute or retrieve the LUT once before starting threads.
    const auto &chroma_check_lut = get_chroma_lut(chroma_threshold);

    // WHY check size? Only print filenames if multiple files are processed, for clarity.
    const bool print_filenames = image_filenames.size() > 1;

    // --- Launch Processing Threads ---
    // WHY vector<result>? Pre-allocate space for results from each thread.
    std::vector<processing_result> results(image_filenames.size());
    std::vector<std::thread> processing_threads;
    // WHY reserve? Minor optimization to avoid potential reallocations if vector grows.
    processing_threads.reserve(image_filenames.size());

    for (size_t i = 0; i < image_filenames.size(); ++i) {
        // WHY emplace_back? Efficiently constructs thread in place.
        // WHY std::ref(results[i])? Pass result slot by reference to allow modification by thread.
        // WHY pass LUT by const ref? Avoid copying the large LUT for each thread.
        processing_threads.emplace_back(process_image_file, image_filenames[i], file_names_only, output_max_chroma, greater_than,
                                        less_than, print_filenames, std::ref(results[i]), std::cref(chroma_check_lut));
    }

    // --- Collect and Print Results ---
    // WHY loop through results? Ensure results are printed in the original file order.
    /* for (size_t i = 0; i < results.size(); ++i) {
        // WHY loop with yield? Simple busy-wait for thread completion. std::memory_order_acquire ensures
        // visibility of writes done by the processing thread before the release store.
        while (!results[i].is_ready.load(std::memory_order_acquire)) {
            std::this_thread::yield(); // WHY yield? Avoids pegging CPU core while waiting.
        }
        // Output is pre-formatted by the processing thread.
        std::cout << results[i].output;
    } */

    // --- Collect and Print Results ---
    for (auto &t : processing_threads) {
        if (t.joinable())
            t.join();
    }

    if (!sort_results) {
        for (auto &res : results) {
            while (!res.is_ready.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            std::cout << res.output;
        }
    } else {
        std::vector<std::reference_wrapper<const processing_result>> sorted_refs(results.begin(), results.end());

        std::stable_sort(sorted_refs.begin(), sorted_refs.end(),
                         [](const processing_result &a, const processing_result &b) { return a.value > b.value; });

        for (const auto &res_ref : sorted_refs) {
            std::cout << res_ref.get().output;
        }
    }

    // --- Cleanup ---
    // WHY join threads? Ensures all threads complete execution before main exits.
    for (auto &t : processing_threads) {
        if (t.joinable()) { // WHY check joinable? Avoids crash if thread wasn't started properly.
            t.join();
        }
    }

    return 0; // Success.
}
