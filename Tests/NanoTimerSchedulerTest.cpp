#include "internal/NanoTimerScheduler.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace NanoOcp1;
using namespace std::chrono_literals;

namespace
{

/**
 * @brief Busy-waits (with sleeps) until @p pred holds or @p timeout elapses.
 * @details Repeatedly checks the predicate and sleeps for 1ms between checks until the predicate returns true or the timeout elapses.
 *
 * @tparam Pred A callable returning a bool.
 * @param pred The predicate to wait for.
 * @param timeout The maximum duration to wait.
 * @return true if the predicate held before the timeout, false otherwise.
 */
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

} // namespace

// A started timer fires at least once after its interval elapses.
TEST(NanoTimerScheduler, FiresAfterInterval)
{
    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&count]() { ++count; });
    scheduler.StartTimer(id, 20ms);

    EXPECT_TRUE(WaitUntil([&count]() { return count.load() >= 1; }, 1000ms)) << "Timer did not fire at least once within the expected interval.";
}

// A timer keeps firing periodically until stopped.
TEST(NanoTimerScheduler, RefiresPeriodically)
{
    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&count]() { ++count; });
    scheduler.StartTimer(id, 20ms);

    EXPECT_TRUE(WaitUntil([&count]() { return count.load() >= 3; }, 1000ms)) << "Timer did not fire multiple times within the expected interval.";
    scheduler.StopTimer(id);
}

// Restarting a running timer pushes its deadline out instead of firing.
TEST(NanoTimerScheduler, StartResetsDeadline)
{
    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&count]() { ++count; });

    // Re-arm a 200ms timer every 20ms; each restart must push the deadline out so it never fires while
    // being reset. The wide 10x margin (20ms re-arm vs 200ms interval) keeps this robust against the
    // scheduling jitter of loaded CI runners (macOS in particular), where a short sleep can overrun.
    scheduler.StartTimer(id, 200ms);
    for (int i = 0; i < 5; ++i)
    {
        std::this_thread::sleep_for(20ms);
        scheduler.StartTimer(id, 200ms);
    }
    EXPECT_EQ(count.load(), 0) << "Timer should not have fired while being repeatedly restarted.";

    EXPECT_TRUE(WaitUntil([&count]() { return count.load() >= 1; }, 2000ms)) << "Once we stop resetting, the timer should fire.";
    scheduler.StopTimer(id);
}

// Stopping a timer before its deadline prevents any fire.
TEST(NanoTimerScheduler, StopCancelsBeforeFire)
{
    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&count]() { ++count; });
    scheduler.StartTimer(id, 100ms);
    scheduler.StopTimer(id);

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 0) << "Timer should not have fired because it was stopped before its deadline.";
}

// Destroying a scheduled timer before it fires prevents any fire.
TEST(NanoTimerScheduler, DestroyBeforeFire)
{
    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&count]() { ++count; });
    scheduler.StartTimer(id, 100ms);
    scheduler.DestroyTimer(id);

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 0) << "Timer should not have fired because it was destroyed before its deadline.";
}

// A callback that stops its own timer fires exactly once (the one-shot gesture pattern).
TEST(NanoTimerScheduler, CallbackCanStopItself)
{
    // shared_ptr so the callback can be built before the id exists (it is filled in after CreateTimer);
    // atomic because the id is written here (test thread) and read in the callback (scheduler thread).
    auto idHolder = std::make_shared<std::atomic<NanoTimerScheduler::TimerId>>(0);

    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&scheduler, idHolder, &count]() {
        ++count;
        scheduler.StopTimer(idHolder->load()); // Stop the timer from within its own callback.
    });
    idHolder->store(id);
    scheduler.StartTimer(id, 20ms);

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 1) << "Timer should have fired exactly once when it stops itself from within the callback.";
}

// A callback that destroys its own timer fires once and does not crash or re-fire.
TEST(NanoTimerScheduler, CallbackCanDestroyItself)
{
    // shared_ptr so the callback can be built before the id exists (it is filled in after CreateTimer);
    // atomic because the id is written here (test thread) and read in the callback (scheduler thread).
    auto idHolder = std::make_shared<std::atomic<NanoTimerScheduler::TimerId>>(0);

    std::atomic<int> count{0};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&scheduler, idHolder, &count]() {
        ++count;
        scheduler.DestroyTimer(idHolder->load()); // Destroy the timer from within its own callback.
    });
    idHolder->store(id);
    scheduler.StartTimer(id, 20ms);

    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(count.load(), 1) << "Timer should have fired exactly once when it destroys itself from within the callback.";
}

// Stop() from another thread blocks until an in-flight callback has finished (no use-after-free).
TEST(NanoTimerScheduler, StopBlocksUntilCallbackFinishes)
{
    std::atomic<bool> inCallback{false};
    std::atomic<bool> callbackFinished{false};
    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&inCallback, &callbackFinished]() {
        inCallback = true;
        std::this_thread::sleep_for(120ms);
        callbackFinished = true;
    });
    scheduler.StartTimer(id, 20ms);

    ASSERT_TRUE(WaitUntil([&inCallback]() { return inCallback.load(); }, 1000ms)); // Wait until the callback has started.

    // The callback is mid-flight; Stop must not return until it completes (no use-after-free).
    scheduler.StopTimer(id);
    EXPECT_TRUE(callbackFinished.load()) << "Stop should block until the in-flight callback has finished.";
}

// Thousands of timers run on the single scheduler thread without crashing or leaking.
TEST(NanoTimerScheduler, ManyTimersShareOneThread)
{
    constexpr int timerCount = 5000;
    std::atomic<int> fireCount{0};
    NanoTimerScheduler scheduler;

    std::vector<NanoTimerScheduler::TimerId> ids;
    ids.reserve(timerCount);
    for (int i = 0; i < timerCount; ++i)
    {
        const auto id = scheduler.CreateTimer([&fireCount]() { ++fireCount; });
        ids.push_back(id);
        scheduler.StartTimer(id, std::chrono::milliseconds(5 + (i % 10))); // Stagger the timers slightly to simulate a more realistic load.
    }

    EXPECT_TRUE(WaitUntil([&fireCount, timerCount]() { return fireCount.load() >= timerCount; }, 5000ms)) << "All timers should have fired at least once within the timeout.";

    for (const auto id : ids)
        scheduler.DestroyTimer(id);
}

// Destroying the scheduler while timers are active is safe.
TEST(NanoTimerScheduler, DestructorStopsCleanly)
{
    std::atomic<int> count{0};
    {
        NanoTimerScheduler scheduler;
        for (int i = 0; i < 100; ++i)
        {
            const auto id = scheduler.CreateTimer([&count]() { ++count; });
            scheduler.StartTimer(id, 5ms);
        }
        std::this_thread::sleep_for(30ms);
        // scheduler destroyed here with timers still running.
    }
    SUCCEED() << "Scheduler destroyed cleanly even with active timers.";
}
