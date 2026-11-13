/*
 * omobd_history.h - Fixed-size history ring for dashboard graphs (pure logic)
 *
 * One ring per tracked channel, allocated once at boot (no heap churn in RUN).
 * NaN samples are kept: a gap in the graph is the honest picture of a sensor
 * that stopped reading.
 */
#ifndef OMOBD_HISTORY_H
#define OMOBD_HISTORY_H

#include <stdint.h>
#include <stddef.h>

template <size_t N>
class OmobdHistoryRing {
public:
    OmobdHistoryRing() : head_(0), count_(0) {}

    void clear() { head_ = 0; count_ = 0; }

    void push(float v) {
        buf_[head_] = v;
        head_ = (head_ + 1) % N;
        if (count_ < N) count_++;
    }

    size_t size() const { return count_; }
    static constexpr size_t capacity() { return N; }

    // i = 0 is the OLDEST retained sample, size()-1 the newest.
    float at(size_t i) const {
        size_t start = (head_ + N - count_) % N;
        return buf_[(start + i) % N];
    }

    float newest() const { return count_ ? at(count_ - 1) : __builtin_nanf(""); }

private:
    float  buf_[N];
    size_t head_;
    size_t count_;
};

#endif // OMOBD_HISTORY_H
