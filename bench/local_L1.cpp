#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "../include/csv.h"
#include "../include/local_L1_ob.hpp"

struct Update
{
    std::uint64_t seq;
    double bid_px;
    double bid_sz;
    double ask_px;
    double ask_sz;
};

inline std::vector<Update> load_csv(const std::filesystem::path &csvPath)
{
    constexpr std::size_t BYTES_PER_ROW = 5 * sizeof(double) + 8;
    const std::uintmax_t bytes = std::filesystem::file_size(csvPath);
    const std::size_t approxRows = bytes / BYTES_PER_ROW;
    std::vector<Update> rows;
    rows.reserve(approxRows);

    io::CSVReader<5, io::trim_chars<>, io::no_quote_escape<','>, io::throw_on_overflow, io::single_and_empty_line_comment<'#'>> csv(csvPath.string());

    csv.read_header(io::ignore_no_column, "seq", "bid_px", "bid_sz", "ask_px", "ask_sz");

    std::uint64_t seq;
    double bid_px, bid_sz, ask_px, ask_sz;

    while (csv.read_row(seq, bid_px, bid_sz, ask_px, ask_sz))
    {
        rows.emplace_back(Update{seq, bid_px, bid_sz, ask_px, ask_sz});
    }
    rows.shrink_to_fit();
    return rows;
}

int main(int argc, char *argv[])
{
    std::filesystem::path exePath = std::filesystem::canonical(argv[0]).parent_path();
    std::filesystem::path csv_path = exePath / ".." / "data" / "local_L1_data.csv";

    const auto updates = load_csv(csv_path);

    eddie::LocalL1Orderbook book;
    std::size_t stale = 0;

    const auto t0 = std::chrono::steady_clock::now();

    for (const auto &u : updates)
    {
        const auto before = book.sequence();
        book.apply({u.seq, u.bid_px, u.bid_sz, u.ask_px, u.ask_sz});
        if (book.sequence() == before)
        {
            ++stale;
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    const double dur_s = std::chrono::duration<double>(t1 - t0).count();
    const double mps = updates.size() / dur_s;

    const auto snap = book.snapshot();

    std::cout << std::fixed << std::setprecision(6) << "\nRESULTS\n"
              << "--------------------------------------------------\n"
              << "Total updates applied : " << updates.size() << '\n'
              << "Stale / out-of-order  : " << stale << "  (" << 100.0 * stale / updates.size() << "%)\n"
              << "Elapsed time          : " << dur_s << " s\n"
              << "Throughput (msgs/s)   : " << mps << '\n'
              << "--------------------------------------------------\n"
              << "Final snapshot\n"
              << "  seq     : " << snap.seq << '\n'
              << "  bid px  : " << snap.bid_px << "  sz: " << snap.bid_sz << '\n'
              << "  ask px  : " << snap.ask_px << "  sz: " << snap.ask_sz << '\n'
              << "--------------------------------------------------\n";

    return 0;
}
