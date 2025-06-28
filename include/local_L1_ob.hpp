#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace eddie
{

struct alignas(64) L1Snapshot
{
    std::uint64_t seq;
    double bid_px;
    double bid_sz;
    double ask_px;
    double ask_sz;
};

static_assert(sizeof(L1Snapshot) <= 64, "L1Snapshot must fit in one cache line");

class LocalL1Orderbook final
{
  public:
    LocalL1Orderbook() noexcept
    {
        reset();
    }

    inline void apply(const L1Snapshot& s) noexcept
    {
        const std::uint64_t cur = seq_.load(std::memory_order_relaxed);
        if (s.seq <= cur)
        {
            return;
        }

        bidPx_ = s.bid_px;
        bidSz_ = s.bid_sz;
        askPx_ = s.ask_px;
        askSz_ = s.ask_sz;

        seq_.store(s.seq, std::memory_order_release);
    }

    [[nodiscard]] inline L1Snapshot snapshot() const noexcept
    {
        L1Snapshot s{};
        for (;;)
        {
            const std::uint64_t before = seq_.load(std::memory_order_acquire);

            s.bid_px = bidPx_;
            s.bid_sz = bidSz_;
            s.ask_px = askPx_;
            s.ask_sz = askSz_;

            s.seq = seq_.load(std::memory_order_relaxed);
            if (before == s.seq) [[likely]]
            {
                return s;
            }
        }
    }

    void reset() noexcept
    {
        seq_.store(0, std::memory_order_relaxed);
        bidPx_ = bidSz_ = askPx_ = askSz_ = 0.0;
    }

    [[nodiscard]] inline std::uint64_t sequence() const noexcept
    {
        return seq_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] inline double bestBidPrice() const noexcept
    {
        return bidPx_;
    }

    [[nodiscard]] inline double bestBidSize() const noexcept
    {
        return bidSz_;
    }

    [[nodiscard]] inline double bestAskPrice() const noexcept
    {
        return askPx_;
    }

    [[nodiscard]] inline double bestAskSize() const noexcept
    {
        return askSz_;
    }

  private:
    alignas(64) mutable std::atomic<std::uint64_t> seq_{0};
    double bidPx_{0.0};
    double bidSz_{0.0};
    double askPx_{0.0};
    double askSz_{0.0};
};

} // namespace eddie