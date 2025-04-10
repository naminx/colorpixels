#include <CLI/CLI.hpp>
#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "lut.hh"
#include "process.hh"

int main(int argc, char **argv)
{
  CLI::App app{"Lab chroma ratio detector"};

  std::vector<std::string> files;
  float chroma_thr{5.f};
  bool sepia{false};
  bool rev{false};
  bool maxc{false};
  int dumpThresh{0};
  app.add_option("files", files, "Image files");
  app.add_option("-c,--chroma", chroma_thr, "Chroma threshold")
      ->check(CLI::PositiveNumber);
  app.add_flag("-s,--sepia", sepia, "Sepia preset threshold");
  app.add_flag("-r,--reverse-column", rev, "Ratio first");
  app.add_flag("-m,--max-chroma", maxc, "Output max chroma");
  app.add_option("-t,--lookup-table", dumpThresh,
                 "Dump LUT at threshold (exits)");
  CLI11_PARSE(app, argc, argv);

  if (sepia)
    chroma_thr = 13.f;
  if (dumpThresh) {
    dump_lookup_table(dumpThresh);
    return 0;
  }

  auto gray_range_LUT{get_lookup_table(chroma_thr)};

  bool print_names{files.size() > 1};

  std::vector<Result> res(files.size());
  std::vector<std::thread> threads;
  for (size_t idx = 0; idx < files.size(); ++idx) {
    threads.emplace_back(process_one, files[idx], rev, maxc, print_names,
                         std::ref(res[idx]), gray_range_LUT);
  }

  for (size_t i = 0; i < res.size(); ++i) {
    while (!res[i].ready.load(std::memory_order_acquire))
      std::this_thread::yield();
    std::cout << res[i].output;
  }

  for (auto &t : threads)
    t.join();
  return 0;
}
