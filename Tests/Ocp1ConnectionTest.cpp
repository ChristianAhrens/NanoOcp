#include "NanoOcp1.h"
#include "internal/NanoTimerScheduler.h"

#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

using namespace NanoOcp1;

// A null scheduler is rejected at construction in release too, not just by the old debug-only assert.
// Otherwise callbacksOnMessageThread=true would silently fall back to delivering callbacks on the socket
// read thread (dispatchOrCall), defeating the opt-in with no diagnostic.
TEST(Ocp1Connection, NullSchedulerThrows)
{
    EXPECT_THROW({ NanoOcp1Client client(nullptr, std::string{}, 0, /*callbacksOnMessageThread=*/true); },
                 std::invalid_argument);
}

// A valid scheduler constructs (and tears down) without throwing.
TEST(Ocp1Connection, ValidSchedulerConstructs)
{
    auto scheduler = std::make_shared<NanoTimerScheduler>();
    EXPECT_NO_THROW({ NanoOcp1Client client(scheduler, std::string{}, 0, /*callbacksOnMessageThread=*/true); });
}
