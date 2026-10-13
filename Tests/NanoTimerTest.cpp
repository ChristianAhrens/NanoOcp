#include "internal/NanoTimer.h"
#include "internal/NanoTimerScheduler.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace NanoOcp1;
using namespace std::chrono_literals;

namespace
{

template <typename Pred>
bool WaitUntil(Pred pred, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (pred())
            return true;
        std::this_thread::sleep_for(1ms);
    }
    return pred();
}

/**
 * Minimal NanoTimer subclass for exercising the scheduler-backed adapter.
 *
 * Note the destructor calls stopTimer(): a subclass that can be firing must stop its timer before its
 * own members are destroyed, otherwise an in-flight callback could dispatch into a destroyed override.
 */
class CountingTimer : public NanoTimer
{
public:
    CountingTimer(std::shared_ptr<NanoTimerScheduler> scheduler, std::atomic<int>& count, bool selfStop = false)
        : NanoTimer(std::move(scheduler)), m_count(count), m_selfStop(selfStop)
    {
    }

    ~CountingTimer() override { stopTimer(); }

    void timerCallback() override
    {
        ++m_count;
        if (m_selfStop)
            stopTimer();
    }

private:
    std::atomic<int>& m_count;
    bool              m_selfStop;
};

} // namespace

// A started timer fires on the shared scheduler thread.
TEST(NanoTimer, FiresAfterStart)
{
    std::atomic<int> count{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    CountingTimer timer(scheduler, count);

    timer.startTimer(20);
    EXPECT_TRUE(WaitUntil([&count]() { return count.load() >= 1; }, 1000ms)) << "Timer did not fire at least once within the timeout.";
    timer.stopTimer();
}

// A running timer keeps firing periodically until stopped.
TEST(NanoTimer, FiresPeriodically)
{
    std::atomic<int> count{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    CountingTimer timer(scheduler, count);

    timer.startTimer(20);
    EXPECT_TRUE(WaitUntil([&count]() { return count.load() >= 3; }, 1000ms)) << "Timer did not fire at least three times within the timeout.";
    timer.stopTimer();
}

// stopTimer() prevents any further callback.
TEST(NanoTimer, StopPreventsFiring)
{
    std::atomic<int> count{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    CountingTimer timer(scheduler, count);

    timer.startTimer(100);
    timer.stopTimer();

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 0) << "Timer fired despite being stopped.";
}

// A callback that stops its own timer fires exactly once (the reconnect/one-shot pattern).
TEST(NanoTimer, CallbackCanStopItself)
{
    std::atomic<int> count{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    CountingTimer timer(scheduler, count, /*selfStop*/ true);

    timer.startTimer(20);
    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 1) << "Timer did not fire exactly once despite self-stopping.";
}

// Several NanoTimers share one scheduler thread.
TEST(NanoTimer, MultipleTimersShareOneScheduler)
{
    std::atomic<int> countA{0};
    std::atomic<int> countB{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    CountingTimer a(scheduler, countA);
    CountingTimer b(scheduler, countB);

    a.startTimer(20);
    b.startTimer(20);
    EXPECT_TRUE(WaitUntil([&]() { return countA.load() >= 1 && countB.load() >= 1; }, 1000ms)) << "One or both timers did not fire at least once within the timeout.";
    a.stopTimer();
    b.stopTimer();
}

// Destroying a running timer is safe: the subclass dtor stops it before teardown.
TEST(NanoTimer, DestructorStopsCleanly)
{
    std::atomic<int> count{0};
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    {
        CountingTimer timer(scheduler, count);
        timer.startTimer(5);
        std::this_thread::sleep_for(30ms);
        // timer destroyed here while active.
    }
    SUCCEED() << "Timer destroyed cleanly while active.";
}
