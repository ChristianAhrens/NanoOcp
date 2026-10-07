#include "internal/NanoTimerScheduler.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
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

// A posted task runs, and on the scheduler thread (not the caller's thread).
TEST(NanoTimerScheduler, PostTaskRunsOffCallerThread)
{
    // Declared before the scheduler so they outlive its shutdown drain (destroyed in reverse order).
    std::atomic<bool> ran{false};
    std::atomic<std::thread::id> taskThread{};
    const auto callerThread = std::this_thread::get_id();

    NanoTimerScheduler scheduler;

    scheduler.PostTask([&]() {
        taskThread.store(std::this_thread::get_id());
        ran.store(true);
    });

    EXPECT_TRUE(WaitUntil([&ran]() { return ran.load(); }, 1000ms)) << "Posted task did not run.";
    EXPECT_NE(taskThread.load(), callerThread) << "Posted task should run on the scheduler thread, not the caller's.";
}

// Posted tasks run in FIFO order.
TEST(NanoTimerScheduler, PostTaskRunsFifo)
{
    // Declared before the scheduler so they outlive its shutdown drain (destroyed in reverse order).
    std::vector<int> order; // only the scheduler thread writes this, until all tasks finish
    std::atomic<int> doneCount{0};
    constexpr int taskCount = 50;

    NanoTimerScheduler scheduler;

    for (int i = 0; i < taskCount; ++i)
    {
        scheduler.PostTask([i, &order, &doneCount]() {
            order.push_back(i);
            doneCount.fetch_add(1, std::memory_order_release); // increment doneCount and signal completion of this task
        });
    }

    ASSERT_TRUE(WaitUntil([&doneCount, taskCount]() { return doneCount.load(std::memory_order_acquire) == taskCount; }, 2000ms))
        << "Not all posted tasks ran.";

    // The acquire above synchronizes-with the last task's release, so every push_back (all on the one
    // scheduler thread) is visible here and nothing writes `order` concurrently — no mutex needed.
    ASSERT_EQ(order.size(), static_cast<std::size_t>(taskCount));

    // Check that the tasks ran in the order they were posted.
    for (int i = 0; i < taskCount; ++i)
        EXPECT_EQ(order[i], i) << "Posted tasks did not run in FIFO order at index " << i;
}

// A task can post another task; both run.
TEST(NanoTimerScheduler, PostTaskFromWithinTask)
{
    // Declared before the scheduler so it outlives the shutdown drain (destroyed in reverse order).
    std::atomic<int> count{0};

    NanoTimerScheduler scheduler;

    scheduler.PostTask([&scheduler, &count]() {
        ++count;
        scheduler.PostTask([&count]() { ++count; });
    });

    EXPECT_TRUE(WaitUntil([&count]() { return count.load() == 2; }, 1000ms)) << "Nested posted task did not run.";
}

// A timer callback can post a task (timers and tasks share the scheduler thread).
TEST(NanoTimerScheduler, PostTaskFromTimerCallback)
{
    // Declared before the scheduler so it outlives shutdown and any late tasks (StopTimer does not drain posted tasks).
    std::atomic<bool> taskRan{false};

    NanoTimerScheduler scheduler;

    const auto id = scheduler.CreateTimer([&scheduler, &taskRan]() {
        scheduler.PostTask([&taskRan]() { taskRan.store(true); });
    });
    scheduler.StartTimer(id, 10ms);

    EXPECT_TRUE(WaitUntil([&taskRan]() { return taskRan.load(); }, 1000ms)) << "Task posted from a timer callback did not run.";
    scheduler.StopTimer(id);
}

// A null task is ignored and does not break the scheduler.
TEST(NanoTimerScheduler, PostTaskIgnoresNull)
{
    // Declared before the scheduler so it outlives the shutdown drain (destroyed in reverse order).
    std::atomic<bool> ran{false};

    NanoTimerScheduler scheduler;

    scheduler.PostTask(nullptr); // no-op

    scheduler.PostTask([&ran]() { ran.store(true); });
    EXPECT_TRUE(WaitUntil([&ran]() { return ran.load(); }, 1000ms)) << "Scheduler should keep working after a null PostTask.";
}

// Tasks queued before the scheduler is destroyed are still run (drained on shutdown, not dropped).
TEST(NanoTimerScheduler, PostTaskDrainedOnShutdown)
{
    std::atomic<int> count{0};
    constexpr int taskCount = 100;

    {
        NanoTimerScheduler scheduler;
        for (int i = 0; i < taskCount; ++i)
            scheduler.PostTask([&count]() { ++count; });
        // scheduler destroyed here; queued tasks must still run before the thread joins.
    }

    EXPECT_EQ(count.load(), taskCount) << "Tasks queued before shutdown should be drained, not dropped.";
}

// A continuously non-empty task queue must not starve timers: an already-due timer still fires
// even while tasks are posted back-to-back without pause (regression test for queue starvation).
TEST(NanoTimerScheduler, BusyTaskQueueDoesNotStarveTimer)
{
    // Declared before the scheduler so they outlive its shutdown drain (destroyed in reverse order).
    std::atomic<bool> keepPosting{true};
    std::atomic<int> taskRuns{0};
    std::atomic<int> timerFires{0};
    std::function<void()> busy;

    NanoTimerScheduler scheduler;

    // Self-reposting task keeps the immediate-task queue continuously non-empty, simulating the
    // sustained socket traffic that previously starved reconnect/timeout timers.
    busy = [&keepPosting, &taskRuns, &busy, &scheduler]() {
        taskRuns.fetch_add(1, std::memory_order_relaxed);
        if (keepPosting.load(std::memory_order_acquire))
            scheduler.PostTask(busy);
    };
    scheduler.PostTask(busy);

    const auto id = scheduler.CreateTimer([&timerFires]() { timerFires.fetch_add(1, std::memory_order_relaxed); });
    scheduler.StartTimer(id, 20ms);

    // Even though tasks never stop flowing, the timer must still fire repeatedly.
    const bool firedEnough = WaitUntil([&timerFires]() { return timerFires.load(std::memory_order_relaxed) >= 3; }, 2000ms);

    keepPosting.store(false, std::memory_order_release); // let the busy chain end for clean teardown
    scheduler.StopTimer(id);

    EXPECT_TRUE(firedEnough) << "A continuously non-empty task queue starved the timer.";
    EXPECT_GT(taskRuns.load(std::memory_order_relaxed), 0) << "Sanity: tasks should also have been running.";
}

// A backlog of perpetually-due timers must not starve posted tasks: a task posted while many short
// timers keep firing back-to-back must still run (regression for OnDeviceLost dispatched via PostTask
// being starved by reconnect timers when many devices drop at once).
TEST(NanoTimerScheduler, BusyDueTimersDoNotStarveTask)
{
    // Declared before the scheduler so they outlive its shutdown drain (destroyed in reverse order).
    constexpr int timerCount = 8;
    std::atomic<int> timerFires{0};
    std::atomic<bool> taskRan{false};

    NanoTimerScheduler scheduler;

    // Several 1ms timers, each doing a little work, keep the due-timer set continuously non-empty
    // (a full round of callbacks takes longer than the re-arm interval, so a timer is always due).
    std::vector<NanoTimerScheduler::TimerId> ids;
    ids.reserve(timerCount);
    for (int i = 0; i < timerCount; ++i)
    {
        const auto id = scheduler.CreateTimer([&timerFires]() {
            timerFires.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(1ms); // simulate work so the timer set stays perpetually due
        });
        scheduler.StartTimer(id, 1ms);
        ids.push_back(id);
    }

    // Wait until the timers are clearly saturating the scheduler thread.
    ASSERT_TRUE(WaitUntil([&timerFires, timerCount]() { return timerFires.load(std::memory_order_relaxed) >= timerCount; }, 2000ms))
        << "Timers did not start firing.";

    // Post a task into the busy scheduler; it must still get a turn despite the timer backlog.
    scheduler.PostTask([&taskRan]() { taskRan.store(true, std::memory_order_release); });

    const bool ran = WaitUntil([&taskRan]() { return taskRan.load(std::memory_order_acquire); }, 2000ms);

    for (auto id : ids)
        scheduler.StopTimer(id);

    EXPECT_TRUE(ran) << "A posted task was starved by continuously-due timers.";
}

// Releasing the last owner from within a callback destroys the scheduler on its own thread. The scheduler
// must detach there instead of self-joining (which throws -> std::terminate under a noexcept destructor).
// Regression for the disconnect-teardown pattern where a callback drops the last reference to its scheduler.
TEST(NanoTimerScheduler, DestroyedFromOwnCallbackDetachesInsteadOfTerminating)
{
    std::atomic<bool> callerDroppedRef{false};
    std::atomic<bool> survivedDestruction{false};

    auto scheduler = std::make_shared<NanoTimerScheduler>();

    // Gate the self-destruction until the caller has dropped its reference, so owner.reset() below is
    // guaranteed to release the LAST reference and run ~NanoTimerScheduler on the scheduler thread.
    scheduler->PostTask([owner = scheduler, &callerDroppedRef, &survivedDestruction]() mutable {
        WaitUntil([&callerDroppedRef]() { return callerDroppedRef.load(std::memory_order_acquire); }, 2000ms);
        owner.reset();
        survivedDestruction.store(true, std::memory_order_release);
    });

    scheduler.reset();                                       // caller no longer owns the scheduler
    callerDroppedRef.store(true, std::memory_order_release); // let the task drop the final reference

    EXPECT_TRUE(WaitUntil([&survivedDestruction]() { return survivedDestruction.load(std::memory_order_acquire); }, 2000ms))
        << "Scheduler destroyed from within its own callback did not complete cleanly (self-join/terminate?).";
}

// A captured owner whose LAST reference is released by the callable's own destruction (rather than reset
// inside the body) must be destroyed while the scheduler mutex is unlocked. Here the owner's destructor
// re-enters the scheduler via DestroyTimer(); if InvokeUnlocked re-locked m_mutex before destroying the
// callable, that re-entry would deadlock. (DestroyedFromOwnCallback... resets the owner inside the body,
// exercising the unlocked path, so it misses this one.)
TEST(NanoTimerScheduler, CapturedOwnerReleasedByCallableDestructionDoesNotDeadlock)
{
    struct ReentrantOwner
    {
        ReentrantOwner(NanoTimerScheduler& s, NanoTimerScheduler::TimerId i, std::atomic<bool>& f)
            : sched(s), id(i), finished(f) {}
        ~ReentrantOwner()
        {
            sched.DestroyTimer(id);     // re-enters the scheduler; deadlocks if run while m_mutex is held
            finished.store(true, std::memory_order_release);
        }
        NanoTimerScheduler& sched;
        NanoTimerScheduler::TimerId id;
        std::atomic<bool>& finished;
    };

    std::atomic<bool> gate{false};
    std::atomic<bool> destructorFinished{false};

    auto scheduler = std::make_unique<NanoTimerScheduler>();
    const auto id = scheduler->CreateTimer([]() {});
    auto owner = std::make_shared<ReentrantOwner>(*scheduler, id, destructorFinished);

    // The lambda holds the owner but never resets it; its last reference drops when the callable is
    // destroyed after the callback returns. Gate the return until the caller has dropped its own reference.
    scheduler->PostTask([owner, &gate]() {
        WaitUntil([&gate]() { return gate.load(std::memory_order_acquire); }, 2000ms);
    });

    owner.reset();                               // caller drops its reference
    gate.store(true, std::memory_order_release); // let the task return so its callable is destroyed

    const bool finished = WaitUntil([&destructorFinished]() { return destructorFinished.load(std::memory_order_acquire); }, 2000ms);
    EXPECT_TRUE(finished) << "Captured owner's destructor did not finish — it likely deadlocked reacquiring the scheduler mutex.";

    if (!finished)
        (void)scheduler.release(); // Worker is deadlocked holding m_mutex; leak rather than hang on ~NanoTimerScheduler.
}
