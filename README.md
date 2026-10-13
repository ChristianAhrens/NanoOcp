# NanoOcp

NanoOcp is a **C++17** library that provides a minimal **AES70 / OCP.1** TCP client and server, plus the message structures and device-specific object definitions needed to control AES70-compatible audio devices over a plain TCP connection.

No third-party dependencies — only the C++ standard library (C++17) and platform sockets (POSIX / Winsock2).

Full API documentation is auto-generated from source and published at:
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/NanoOcp/)

[![Latest Release](https://img.shields.io/github/v/release/ChristianAhrens/NanoOcp)](https://github.com/ChristianAhrens/NanoOcp/releases/latest)
Pushing a tag (`X.Y.Z`, matching the version in the root `CMakeLists.txt`) publishes a GitHub Release with prebuilt `NanoOcp1Demo` binaries and the `NanoOcp1` library + headers for macOS, Linux, and Windows attached — see [Releases](https://github.com/ChristianAhrens/NanoOcp/releases).

|Platform|Status|
|:---|:---|
| macOS   | [![CI macOS](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml)   |
| Windows | [![CI Windows](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml) |
| Linux   | [![CI Linux](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml)   |
| Unit Tests | [![Unit Tests](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml) |

---

## Background: AES70 and OCP.1

[AES70](https://www.aes.org/publications/standards/search.cfm?docID=91) (also known as OCA — *Open Control Architecture*) is an open standard for controlling professional audio equipment over IP networks.  It defines a rich class hierarchy of controllable objects (gains, mutes, delays, routing matrices, …) and two wire protocols:

| Protocol | Transport | Port |
|---|---|---|
| **OCP.1** | TCP (framed) | device-specific; DS100 default: **50014** |
| **OCP.3** | WebSocket | not supported by NanoOcp |

NanoOcp implements **OCP.1 only** and is intentionally minimal — no AES70 object database, no root-block enumeration, no full standards compliance.  It provides just enough to:

- Open and maintain a TCP connection to an OCA device (or accept an incoming one).
- Serialize and deserialize the five OCP.1 message types (Command, CommandResponseRequired, Response, Notification, KeepAlive).
- Send `AddSubscription` commands so the device pushes property-change notifications.
- Use pre-built object definitions for d&b audiotechnik amplifiers and the DS100 signal engine.

---

## Repository layout

```
NanoOcp/
├── Source/                         # Library source — include these in your project
│   ├── NanoOcp1.h / .cpp           # NanoOcp1Client, NanoOcp1Server, NanoOcp1Base
│   ├── Ocp1Connection.h / .cpp     # Abstract TCP socket management
│   ├── Ocp1ConnectionServer.h/.cpp # TCP accept-loop server
│   ├── Ocp1Message.h / .cpp        # OCP.1 message structs and factory
│   ├── Ocp1DataTypes.h / .cpp      # ByteVector, Ocp1DataType, marshal helpers
│   ├── Variant.h / .cpp            # Type-erased OCA value (marshal/unmarshal)
│   ├── Ocp1ObjectDefinitions.h     # Generic d&b amp object definitions
│   ├── Ocp1DS100ObjectDefinitions.h# DS100-specific object definitions
│   ├── Ocp1Controller.h / .cpp     # Generic OCP.1 session controller (base class)
│   ├── AmpController.h / .cpp      # d&b amplifier controller (Dx / Dy / 5D)
│   ├── SoundscapeController.h / .cpp    # d&b DS100 signal engine controller
│   └── internal/                   # Platform helpers (no external deps)
│       ├── NanoSocket.h / .cpp     # Cross-platform TCP socket (POSIX / Winsock2)
│       ├── NanoThread.h            # std::thread wrapper (replaces juce::Thread)
│       ├── NanoTimer.h / .cpp      # Periodic timer (replaces juce::Timer), backed by NanoTimerScheduler
│       └── NanoTimerScheduler.h / .cpp  # Shared one-thread scheduler: periodic timers + one-shot posted tasks (PostTask)
├── NanoOcp1Demo/                   # CLI demo application
│   ├── CMakeLists.txt
│   ├── Terminal.h                  # Platform terminal setup / size query
│   ├── Ansi.h                      # ANSI escape-sequence constants
│   ├── AppState.h                  # Shared UI state + logging helpers + per-mode panel-size/terminal-fit checks
│   ├── Format.h                    # Small text-formatting helpers (bars, state/type strings)
│   ├── FocusParams.h               # Soundscape-focus parameter table, lookup, Variant format/parse
│   ├── Panels.h                    # Panel renderers (incl. ranged/column layouts), canvas mgmt, redraw thread
│   ├── Demo.h                      # Demo controller class (wraps AmpController / SoundscapeController)
│   └── main.cpp                    # CLI help/argument parsing + entry point (three-mode terminal UI)
├── Tests/                          # GoogleTest unit tests (NanoOcp1Tests target)
├── CMakeLists.txt                  # Root CMake build (library + optional demo/tests)
├── submodules/
│   └── doxygen-awesome-css/        # Doxygen HTML theme (docs only)
├── Doxyfile                        # Doxygen configuration
└── .github/workflows/
    ├── ci.yml                      # GitHub Actions: macOS / Windows / Linux
    └── docs.yml                    # GitHub Actions: Doxygen → gh-pages
```

---

## Architecture

NanoOcp is structured in four layers.  The controller layer sits on top of the existing low-level stack and is the recommended entry point for application code:

```
┌─────────────────────────────────────────────────────┐
│                  Your application                   │
│  onStateChanged / onPower / onChannelGain / …       │
└──────────────────────┬──────────────────────────────┘
                       │  typed callbacks (scheduler thread by default)
┌──────────────────────▼──────────────────────────────┐
│              Ocp1Controller  (base)                  │  Layer 1 – Controllers
│  Connection lifecycle, auto-reconnect, sub/query     │
│  ├── AmpController  (d&b Dx / Dy / 5D amplifiers)   │
│  │     typed: onPower, onChannelGain, onChannelMute  │
│  │           onChannelISP/GR/OVL, onChannelHeadroom  │
│  └── SoundscapeController  (d&b DS100 signal engine)      │
│        GUID handshake, RemoteObject vocabulary,      │
│        setActiveRemoteObjects / onRemoteObjectReceived│
└──────────────────────┬──────────────────────────────┘
                       │  callbacks (scheduler thread by default)
┌──────────────────────▼──────────────────────────────┐
│          NanoOcp1Client  /  NanoOcp1Server           │  Layer 2 – Connection
│  (NanoOcp1Base + Ocp1Connection + NanoTimer)         │
└──────────────────────┬──────────────────────────────┘
                       │  ByteVector (raw OCP.1 frame)
┌──────────────────────▼──────────────────────────────┐
│   Ocp1Message  (Command / Response / Notification /  │  Layer 3 – Protocol
│                 KeepAlive)  +  Ocp1Header             │
│   Ocp1CommandResponseRequired  ←  Ocp1CommandDefinition│
└──────────────────────┬──────────────────────────────┘
                       │  Ocp1CommandDefinition subclasses
┌──────────────────────▼──────────────────────────────┐
│  Ocp1ObjectDefinitions  /  Ocp1DS100ObjectDefinitions│  Layer 4 – Device objects
│  dbOcaObjectDef_*  structs  (per parameter, per ONo) │
└─────────────────────────────────────────────────────┘
```

### Layer 1 — Controllers (`Ocp1Controller.h`, `AmpController.h`, `SoundscapeController.h`)

The controller layer handles the complete session lifecycle so application code never has to manage subscribe/query sequencing, handle maps, or reconnection timers.

**`Ocp1Controller`** — generic base class.  Call `trackObject()` to register parameters of interest, then `connect(host, port)`.  The controller transitions automatically through:

```
Disconnected → Connecting → Subscribing → Subscribed → GetValues → Connected
```

On connection loss the underlying client retries automatically and the controller re-subscribes on the next successful connect.  Override `afterConnected()` to insert a device-specific handshake before the standard subscribe/query sequence.

**`AmpController`** — targets d&b Dx, Dy, and 5D amplifiers.  Call `setAmpType(type, channelCount)` before `connect()`.  Fires typed callbacks:

| Callback | Payload |
|---|---|
| `onPower` | `bool` — amp is on / off |
| `onChannelGain` | `uint16_t ch, float dB` |
| `onChannelMute` | `uint16_t ch, bool muted` |
| `onChannelISP` | `uint16_t ch, bool active` |
| `onChannelGR` | `uint16_t ch, bool active` |
| `onChannelOVL` | `uint16_t ch, bool active` |
| `onChannelHeadroom` | `uint16_t ch, float dB` |

Write commands: `setPower(bool)`, `setChannelGain(ch, dB)`, `setChannelMute(ch, bool)`.

**`SoundscapeController`** — targets d&b DS100 signal engines (DS100, DS110, DS100M, vCore).  Performs a GUID read on first connect to determine the OCA revision before subscribing.  The full `RemoteObject` vocabulary (75 parameter identifiers) is expressed as `RemoteObject::RemObjIdent` enumerators.  Set the parameters to monitor via `setActiveRemoteObjects()` and receive value updates through `onRemoteObjectReceived`.  Write values via `setObjectValue()`.

### Layer 2 — Connection (`NanoOcp1.h`)

`NanoOcp1Base` is the abstract base class that holds the target address/port and exposes three `std::function` callbacks:

| Callback | When fired |
|---|---|
| `onConnectionEstablished` | TCP connect succeeded |
| `onConnectionLost` | TCP connection dropped or failed |
| `onDataReceived(ByteVector)` | A complete OCP.1 frame arrived |

**`NanoOcp1Client`** — inherits `NanoOcp1Base`, `Ocp1Connection` (raw socket via `NanoSocket`), and `NanoTimer`.  `start()` starts a periodic timer that retries `connectToSocket()` until it succeeds.  Reconnects automatically after a disconnect.

**`NanoOcp1Server`** — inherits `NanoOcp1Base` and `Ocp1ConnectionServer` (accept loop).  `start()` binds a port and waits for an incoming connection.  Only one simultaneous peer is supported.

### Layer 3 — Protocol (`Ocp1Message.h`)

`Ocp1Message` is the abstract base for all five OCP.1 message types.  Use the static factory `Ocp1Message::UnmarshalOcp1Message(bytes)` to parse incoming data, then dispatch on `GetMessageType()`:

| `MessageType` | Class | Direction |
|---|---|---|
| `Command` (0) | `Ocp1Message` | Client → Device |
| `CommandResponseRequired` (1) | `Ocp1CommandResponseRequired` | Client → Device |
| `Notification` (2) | `Ocp1Notification` | Device → Client |
| `Response` (3) | `Ocp1Response` | Device → Client |
| `KeepAlive` (4) | `Ocp1KeepAlive` | Both |

`Ocp1CommandDefinition` is a plain struct that bundles the five fields needed to address any OCA property: target ONo, property data type, def-level, property index, and optional parameter bytes.  Its four virtual factory methods produce ready-to-send command definitions:

- `AddSubscriptionCommand()` — register for property-change notifications
- `RemoveSubscriptionCommand()` — unregister
- `GetValueCommand()` — read the current value
- `SetValueCommand(Variant)` — write a new value

### Layer 4 — Device objects (`Ocp1ObjectDefinitions.h`, `Ocp1DS100ObjectDefinitions.h`)

Concrete `dbOcaObjectDef_*` structs subclass `Ocp1CommandDefinition`.  Each struct represents one controllable parameter on one class of device.  Constructors accept the channel/record/object numbers and compute the correct **ONo** internally — callers never compose ONos manually.

**Generic d&b amplifier objects** (`Ocp1ObjectDefinitions.h`):
covers AmpGeneric, Dx, Dy, 5D — power, gain, mute, delay, EQ bands, input select, …

**DS100 signal engine objects** (`Ocp1DS100ObjectDefinitions.h`, namespace `NanoOcp1::DS100`):
covers all DS100 parameter boxes (MatrixInput, MatrixOutput, Positioning, CoordinateMapping, ReverbInput, Scene, …).

---

## Key concepts

| Concept | Description |
|---|---|
| **ONo** (Object Number) | 32-bit identifier encoding device type, record, channel and box/object number.  Computed by `GetONo()` / `GetONoTy2()`. |
| **Def-level** | Inheritance depth in the AES70 class hierarchy at which a property is defined (e.g. `DefLevel_OcaGain = 4`). |
| **Command handle** | Auto-incrementing 32-bit token assigned by `Ocp1CommandResponseRequired`.  The device echoes it back in the matching `Ocp1Response` so responses can be correlated to commands. |
| **AddSubscription** | Command that asks the device to push a `Notification` every time a property changes.  Must be sent once per property before notifications arrive. |
| **KeepAlive** | Heartbeat frame (carries a heartbeat interval).  Both sides send it; absence triggers reconnection. |

---

## Threading model

`NanoOcp1Client` runs all socket I/O on a dedicated `Ocp1Connection::ConnectionThread` (a thin `std::thread` wrapper).

The thread on which callbacks fire is selected by the `callbacksOnMessageThread` constructor parameter, which **defaults to `true`** at every layer (`Ocp1Connection`, `NanoOcp1Client`, `NanoOcp1Server`, `Ocp1Controller`, `AmpController`, `SoundscapeController`):

- **`true` (default)** — the low-level callbacks (`onDataReceived`, `onConnectionEstablished`, `onConnectionLost`) are posted to the shared `NanoTimerScheduler` thread, decoupling callback execution from socket I/O so slow or blocking callback code cannot stall the read loop.
- **`false`** — the low-level callbacks run synchronously on the **socket thread**.

Controller callbacks (`onStateChanged`, `onPower`, `onChannelGain`, `onRemoteObjectReceived`, …) are invoked from within those low-level callbacks, so they fire on whichever thread the parameter selects — the shared `NanoTimerScheduler` thread by default, or the socket thread when `callbacksOnMessageThread = false`.  `onStateChanged` triggered by the GetValues response-timeout always fires on the `NanoTimerScheduler` thread (see **Timers** below) — the same thread the other callbacks use under the default.

Whichever mode is used, callbacks never run on a GUI/framework thread automatically.  If you need to update GUI elements or call framework APIs that require a specific thread (e.g. the JUCE message thread), marshal inside the callback — for example via `juce::MessageManager::callAsync` or by posting a message to a `juce::MessageListener`.

**Timers.** `NanoOcp1Client` (reconnect) and `Ocp1Controller` (GetValues response-timeout) own no timer thread of their own.  Instead, they take a caller-supplied `std::shared_ptr<NanoTimerScheduler>` and register their timers on it.  The scheduler is a **required constructor argument** — NanoOcp provides no default or singleton; the owning application creates it and shares one instance across many clients and controllers.

A single `NanoTimerScheduler` runs one background thread that services every timer registered on it.  Callbacks it triggers (e.g. `onStateChanged` from a response-timeout) therefore fire on that **scheduler thread**, not the socket thread.  That same thread also runs the low-level callbacks posted under the default `callbacksOnMessageThread = true`, so one scheduler thread handles both the timers and the dispatched callbacks.

**Lifetime.** It is safe to release the last `shared_ptr<NanoTimerScheduler>` from within one of its own callbacks: the worker thread holds its own reference to the scheduler's internal state, so the scheduler detaches its thread and unwinds cleanly instead of self-joining.  This applies to the scheduler object only.  Do **not** destroy a client or controller (or other timer owner) from inside its own callback — teardown re-enters the connection's `SafeAction` guard (`disconnect()` → `setSafe()`) on the same thread and self-deadlocks.  Defer that teardown to outside the callback.

---

## Integration

NanoOcp uses **CMake ≥ 3.15** as its build system.  The library requires only a C++17-capable compiler and platform sockets — no third-party dependencies.

### As a CMake subdirectory

```cmake
add_subdirectory(path/to/NanoOcp)
target_link_libraries(YourTarget PRIVATE NanoOcp1)
```

### Building the demo manually

```bash
cmake -B build -S . -DNANOOCP1_BUILD_DEMO=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Building and running the unit tests

```bash
cmake -B build -S . -DNANOOCP1_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

`NANOOCP1_BUILD_TESTS` is `ON` by default; the tests build into the `NanoOcp1Tests` target and are registered with CTest via `gtest_discover_tests`.

### Adding source files directly

1. Add `Source/` to your project's include paths.
2. Add all `.cpp` files from `Source/` and `Source/internal/` to your build target.
3. Require C++17 (`-std=c++17` / `cxx_std_17`).
4. On Windows, link `ws2_32`.

### Prebuilt releases

Every `X.Y.Z` tag is built for macOS, Linux, and Windows and attached to the
corresponding [GitHub Release](https://github.com/ChristianAhrens/NanoOcp/releases)
as `NanoOcp1-X.Y.Z-<platform>.{tar.gz,zip}`, each containing:

```
bin/NanoOcp1Demo[.exe]   # ready-to-run demo executable
lib/libNanoOcp1.a        # (NanoOcp1.lib on Windows)
include/*.h               # public headers, incl. the generated NanoOcp1Version.h
```

### Versioning

The library and demo share a single version, defined once in the root
`CMakeLists.txt`'s `project(NanoOcp1 VERSION X.Y.Z ...)` call. It's exposed to
code via the generated `NanoOcp1Version.h` (`NANOOCP1_VERSION_STRING` /
`NanoOcp1::GetVersionString()`), and `NanoOcp1Demo -v` / `--version` prints it
on the command line. On Windows, it's also embedded as the `.exe`'s
VERSIONINFO resource (visible under file Properties → Details).

---

## Usage examples

### AmpController — typed amplifier control

```cpp
#include "AmpController.h"

auto scheduler = std::make_shared<NanoOcp1::NanoTimerScheduler>();
auto amp = std::make_unique<NanoOcp1::AmpController>(scheduler);

// Configure before connect
amp->setAmpType(NanoOcp1::AmpController::AmpType::Dy, 4 /*channels*/);

// Wire typed callbacks (fired on the shared NanoTimerScheduler thread by default)
amp->onStateChanged = [](NanoOcp1::Ocp1Controller::State s) {
    // Disconnected / Connecting / Subscribing / Subscribed / GetValues / Connected
};
amp->onPower = [](bool on) {
    // power state changed
};
amp->onChannelGain = [](std::uint16_t ch, float dB) {
    // ch is 1-based; dB range: -57.5 to +6.0
};
amp->onChannelMute = [](std::uint16_t ch, bool muted) {};
amp->onChannelISP  = [](std::uint16_t ch, bool active) {};
amp->onChannelGR   = [](std::uint16_t ch, bool active) {};
amp->onChannelOVL  = [](std::uint16_t ch, bool active) {};
amp->onChannelHeadroom = [](std::uint16_t ch, float dB) {};

// Connect — auto-reconnect and re-subscribe on loss
amp->connect("192.168.1.100", 50014);

// Send commands (no-op unless Connected)
amp->setPower(true);
amp->setChannelGain(1, -12.0f);
amp->setChannelMute(2, true);

// Stop
amp->disconnect();
```

### SoundscapeController — DS100 remote object control

```cpp
#include "SoundscapeController.h"

auto scheduler = std::make_shared<NanoOcp1::NanoTimerScheduler>();
auto ds100 = std::make_unique<NanoOcp1::SoundscapeController>(scheduler);

using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
using RO  = NanoOcp1::SoundscapeController::RemoteObject;

// Declare which parameters to subscribe and query on every connect
ds100->setActiveRemoteObjects({
    RO{ ROI::MatrixInput_LevelMeterIn,     ROA{5, 0} },  // level meter SO 5
    RO{ ROI::Positioning_SourcePosition,   ROA{5, 0} },  // XYZ position SO 5
    RO{ ROI::Positioning_SourceSpread,     ROA{5, 0} },  // spread SO 5
    RO{ ROI::ReverbInput_Gain,             ROA{1, 5} },  // En-Space send gain SO 5
});

// Value-change callback (fired on the shared NanoTimerScheduler thread by default)
ds100->onRemoteObjectReceived = [](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool {
    bool ok = false;
    switch (ro.Id)
    {
    case ROI::Positioning_SourcePosition:
    {
        auto xyz = ro.Var.ToPosition(&ok);
        if (ok)
        {
            // xyz[0]=X, xyz[1]=Y, xyz[2]=Z  (all 0.0–1.0)
        }
        break;
    }
    case ROI::MatrixInput_LevelMeterIn:
    {
        float dBFS = ro.Var.ToFloat(&ok);
        break;
    }
    default:
        break;
    }
    return ok;
};

ds100->onStateChanged = [&](NanoOcp1::Ocp1Controller::State s) {
    if (s == NanoOcp1::Ocp1Controller::State::Connected)
    {
        // Device model identified after GUID handshake
        auto model = ds100->getConnectedDeviceModel(); // DS100 / DS110 / DS100M / vCore
    }
};

// Connect — GUID handshake runs automatically, then subscribe+query
ds100->connect("192.168.1.100", 50014);

// Write a value
ds100->setObjectValue(RO{
    ROI::Positioning_SourcePosition,
    ROA{5, 0},
    NanoOcp1::Variant{0.5f, 0.3f, 0.0f}
});

ds100->disconnect();
```

### Low-level client — connect, subscribe, get, set

```cpp
#include "NanoOcp1.h"
#include "Ocp1Message.h"
#include "Ocp1DS100ObjectDefinitions.h"

// 1. Create the shared scheduler and the client (callbacks fire on the socket thread with false)
auto scheduler = std::make_shared<NanoOcp1::NanoTimerScheduler>();
auto client = std::make_unique<NanoOcp1::NanoOcp1Client>(
    scheduler, "192.168.1.100", 50014, /*callbacksOnMessageThread=*/false);

// 2. Wire callbacks before start()
client->onConnectionEstablished = [&]() {
    // Send first commands here (e.g. read GUID, send subscriptions)
};
client->onConnectionLost = [&]() {
    // Clear pending handles, update UI, etc.
};
client->onDataReceived = [&](const NanoOcp1::ByteVector& data) -> bool {
    auto msg = NanoOcp1::Ocp1Message::UnmarshalOcp1Message(data);
    if (!msg) return false;

    switch (msg->GetMessageType())
    {
        case NanoOcp1::Ocp1Message::Notification:
        {
            auto* n = static_cast<NanoOcp1::Ocp1Notification*>(msg.get());
            // match n->GetEmitterOno() against your subscription table
            break;
        }
        case NanoOcp1::Ocp1Message::Response:
        {
            auto* r = static_cast<NanoOcp1::Ocp1Response*>(msg.get());
            // match r->GetResponseHandle() against your pending-command map
            break;
        }
        default: break;
    }
    return true;
};

// 3. Start — begins reconnect timer; first successful connect fires onConnectionEstablished
client->start();

// 4. Subscribe to sound-object 5 position on a DS100
NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position posDef(5);
std::uint32_t subHandle;
auto subCmd = NanoOcp1::Ocp1CommandResponseRequired(
    posDef.AddSubscriptionCommand(), subHandle);
client->sendData(subCmd.GetSerializedData());

// 5. Read the current position
std::uint32_t getHandle;
auto getCmd = NanoOcp1::Ocp1CommandResponseRequired(
    posDef.GetValueCommand(), getHandle);
client->sendData(getCmd.GetSerializedData());

// 6. Write a new position (x=0.5, y=0.5, z=0.0)
NanoOcp1::Variant newPos(0.5f, 0.5f, 0.0f);
std::uint32_t setHandle;
auto setCmd = NanoOcp1::Ocp1CommandResponseRequired(
    posDef.SetValueCommand(newPos), setHandle);
client->sendData(setCmd.GetSerializedData());
```

### Server — accept an incoming OCA controller

```cpp
// The server binds port 50014 and waits for a controller to connect.
auto scheduler = std::make_shared<NanoOcp1::NanoTimerScheduler>();
auto server = std::make_unique<NanoOcp1::NanoOcp1Server>(
    scheduler, "", 50014, /*callbacksOnMessageThread=*/false);

server->onConnectionEstablished = [&]() { /* controller connected */ };
server->onConnectionLost        = [&]() { /* controller disconnected */ };
server->onDataReceived = [&](const NanoOcp1::ByteVector& data) -> bool {
    auto msg = NanoOcp1::Ocp1Message::UnmarshalOcp1Message(data);
    // handle incoming commands from the controller …
    return true;
};

server->start();
```

### Message flow diagram

```
Client                                     Device
  │──CommandResponseRequired(AddSub)──────►│  subscribe to a property
  │◄──────────────Response(OK)─────────────│
  │──CommandResponseRequired(GetValue)─────►│  read current value
  │◄──────────────Response(value)───────────│
  │◄──────────────Notification──────────────│  value changed (unsolicited)
  │──CommandResponseRequired(SetValue)─────►│  write new value
  │◄──────────────Response(OK)─────────────│
  │──KeepAlive──────────────────────────────►│
  │◄──────────────KeepAlive─────────────────│
```

---

## Demo application — NanoOcp1Demo

`NanoOcp1Demo/` is a **CLI application** (entry point `main.cpp`, split into a handful of single-header modules — see [Repository layout](#repository-layout)) with a live terminal panel (ANSI colours, macOS / Linux / Windows) that demonstrates both high-level controllers.  It operates in three modes selected at startup — Amp, Soundscape overview, and Soundscape focus — plus a Soundobject-Routing sub-mode reached at runtime from Soundscape overview.  Every mode checks the current terminal size against what it needs and shows a short warning instead of a garbled panel if it doesn't fit (see [Terminal-too-small warning](#terminal-too-small-warning)).

### Amp mode (`--amp`, default)

Connects to a d&b amplifier via `AmpController`.  Shows per-channel gain bars, mute state, and protection indicators (ISP / GR / OVL / headroom), in a fixed 19-row panel.

```
a <ip>         set host address          p <n>       set port
c              connect (or reconnect)    d           disconnect
1 / 0          power on / off
g <ch> <dB>    set channel gain  (ch: 1-4,  dB: -57.5 to +6.0)
m <ch> <1|0>   mute / unmute channel
q              quit
```

### Soundscape overview mode (`--soundscape <N>` or `--soundscape <lo>-<hi>`)

Connects to a d&b Soundscape signal engine (DS100, DS110, DS100M, or vCore) via `SoundscapeController`.  Monitors and controls every sound object in the given range — a single number is a range of one, e.g. `--soundscape 8-32` for 25 objects at once: level meter, gain, mute, XYZ position, spread, delay mode, En-Space send gain.  The exact device model is identified automatically via the GUID handshake.

Each sound object is laid out as its own **channel-strip column** (not a row), wrapping onto additional stacked bands when the range is wider than the terminal — so a 128-object range on a narrow terminal just grows taller, not off-screen to the right.  The level meter is a full-width bar over the device's actual **-120..0 dB** metering range, with the exact dB value printed beneath it, fed from `MatrixInput_LevelMeterIn`.

Per-parameter commands take an optional leading sound-object number to target just that one object within the active range; with no target they **broadcast** to every object in it — handy for batch-configuring a block of objects identically, then fine-tuning one:

```
a <ip>              set host address          p <n>       set port
c                   connect (or reconnect)    d           disconnect
x/y/z [so] <m>      set position XYZ (meters)              -- [so]: target one object; omit to broadcast
sp [so] <0-1>       set spread
dm [so] <0|1|2>     set delay mode  (0=off  1=compensate  2=reflect)
ig [so] <dB>        set matrix input gain  (-120.0 to +24.0)
mm [so] <1|0>       mute / unmute matrix input
es [so] <dB>        set En-Space send gain  (-120.0 to +24.0)
r <so> | r <lo>-<hi>  enter the Soundobject-Routing sub-mode (see below)
q                   quit
```

### Soundobject-Routing sub-mode (`r <so>` or `r <lo>-<hi>`, from Soundscape overview)

Switches to controlling `SoundObjectRouting_Gain` / `SoundObjectRouting_Mute` for one sound object, or any sub-range of the active overview range, against all 32 Function Groups — laid out the same way as the overview (one column per selected sound object, wrapping onto further bands), with one row per Function Group.  Subscribes to `FunctionGroup_Mode` (is the group in use?) and `FunctionGroup_Name` (display label) for all 32 groups up front, so groups the device reports as unused are shown dimmed rather than with meaningless values.  `rg`/`rm` broadcast to every selected sound object by default, or can target one within the routing range — the same convention as the overview commands above.

```
a <ip>              set host address          p <n>       set port
c                   connect (or reconnect)    d           disconnect
rg <fg> [so] <dB>   set routing gain to Function Group <fg>  (-120.0 to +24.0)
rm <fg> [so] <1|0>  mute / unmute routing to Function Group <fg>
b / back            return to Soundscape overview mode
q                   quit
```

### Soundscape focus mode (`--soundscape <N> --param <name> [--addr2 <n>]`)

Same connection as overview mode, but subscribes to exactly **one** OCP1 remote object instead of the fixed overview set, and monitors it in a scrolling, timestamped log that fills the whole terminal height — the panel adapts live as the terminal is resized, keeping only a handful of fixed rows (title, host/status, current value, commands) pinned above the log.  Use this to watch or drive any parameter in `RemoteObject::RemObjIdent`, not just the ones the overview panel hardcodes.

- `<N>` (from `--soundscape`) is the parameter's primary address (sound-object / matrix-input channel, matrix-output channel, function group, reverb zone, or mapping area, depending on the parameter — see `--list-params` below).
- `--addr2 <n>` supplies a secondary address for two-dimensional parameters (e.g. `MatrixNode_Gain`'s output channel). Defaults to 0.
- The value type (bool / int / float / string / XYZ position) is learned from the first value received for that parameter, so the same `v` command works generically across the whole parameter space.

```
a <ip>         set host address          p <n>       set port
c              connect (or reconnect)    d           disconnect
v              enter a new value to send; type it on the next line,
               or 'b' / 'back' to cancel
q              quit
```

Enumerate every focusable parameter name (with its addressing hint and description) as a single-shot call — this does not connect to anything:

```bash
NanoOcp1Demo --soundscape --list-params
```

### Terminal-too-small warning

Every mode computes the minimum terminal size its current panel needs — a fixed ~80x20 for Amp and Soundscape focus's mostly-fixed-width rows; for Soundscape overview/routing, however many rows their column/band layout works out to for the active range at the current terminal width, plus room for at least one column.  If the terminal is smaller than that (at startup, or because it was resized smaller mid-session), the panel is replaced with a short centred warning instead of an overflowing or garbled layout:

```
          Terminal too small for Soundscape Overview mode
             Needs at least 28 rows x 16 cols — have 10 x 220
                Enlarge the terminal, or 'q' to quit
```

Input keeps working while the warning is shown — `q`/`quit` always exits, and `b`/`back` still leaves the Soundobject-Routing sub-mode — so enlarging the terminal (or going back) immediately brings the normal panel back.

### Running the demo

```bash
# Build (CMake)
cmake -B build -S . -DNANOOCP1_BUILD_DEMO=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Amp mode (default) — connect to a d&b Dy amplifier with 4 channels
./build/NanoOcp1Demo/Release/NanoOcp1Demo 192.168.1.100 50014 --amp --type dy --ch 4

# Soundscape overview mode — monitor and control sound object 5 (DS100/DS110/DS100M/vCore)
./build/NanoOcp1Demo/Release/NanoOcp1Demo 192.168.1.100 50014 --soundscape 5

# Soundscape overview mode, ranged — monitor/control sound objects 8-32 (25 objects) at once
./build/NanoOcp1Demo/Release/NanoOcp1Demo 192.168.1.100 50014 --soundscape 8-32

# Soundscape focus mode — monitor/set just MatrixInput_Gain on sound object 5
./build/NanoOcp1Demo/Release/NanoOcp1Demo 192.168.1.100 --soundscape 5 --param MatrixInput_Gain

# Soundscape focus mode with a two-dimensional parameter (input 3 -> output 7)
./build/NanoOcp1Demo/Release/NanoOcp1Demo 192.168.1.100 --soundscape 3 --param MatrixNode_Gain --addr2 7

# List every focusable Soundscape parameter and exit
./build/NanoOcp1Demo/Release/NanoOcp1Demo --soundscape --list-params
```

`build/NanoOcp1Demo/Release/` (or `Debug/`) is where the executable lands on every platform — CMake places it there consistently whether the generator is single-config (Makefiles/Ninja on macOS/Linux) or multi-config (Visual Studio/Xcode).

Once connected in overview mode, type `r 8-12` at the prompt to enter the Soundobject-Routing sub-mode for that sub-range (or `r 8` for a single object), `rg 3 -6.0` to set Function Group 3's routing gain to -6.0 dB on every object in `8-12`, `rm 3 8 1` to mute just object 8's routing to Function Group 3, and `b` to return to the overview.

---

## API documentation

The full API reference is generated by [Doxygen](https://www.doxygen.nl/) using the [doxygen-awesome-css](https://jothepro.github.io/doxygen-awesome-css/) theme and published automatically to GitHub Pages on every push to `main` via the `.github/workflows/docs.yml` workflow.

Browse the online docs: **https://ChristianAhrens.github.io/NanoOcp/**

To generate docs locally:
```bash
# Requires doxygen and graphviz to be installed
doxygen Doxyfile
open docs/html/index.html
```

---

## License

NanoOcp is distributed under the **GNU Lesser General Public License v3.0**.  See `LICENSE` for details.
