# NanoOcp

NanoOcp is a **JUCE-free, C++17** library that provides a minimal **AES70 / OCP.1** TCP client and server, plus the message structures and device-specific object definitions needed to control AES70-compatible audio devices over a plain TCP connection.

No third-party dependencies — only the C++ standard library (C++17) and platform sockets (POSIX / Winsock2).

Full API documentation is auto-generated from source and published at:
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/NanoOcp/)

|Platform|Status|
|:---|:---|
| macOS   | [![CI macOS](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml)   |
| Windows | [![CI Windows](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml) |
| Linux   | [![CI Linux](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/ChristianAhrens/NanoOcp/actions/workflows/ci.yml)   |

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
│       └── NanoTimer.h / .cpp      # Periodic timer (replaces juce::Timer)
├── NanoOcp1Demo/                   # JUCE-free CLI demo application
│   ├── CMakeLists.txt
│   └── main.cpp                    # Two-mode terminal UI: amp control / DS100 SO control
├── CMakeLists.txt                  # Root CMake build (library + optional demo)
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
                       │  typed callbacks (socket thread)
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
                       │  callbacks (socket thread)
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

**`SoundscapeController`** — targets d&b DS100 signal engines (DS100, DS110, DS100M, vCore).  Performs a GUID read on first connect to determine the OCA revision before subscribing.  The full `RemoteObject` vocabulary (74 parameter identifiers) is expressed as `RemoteObject::RemObjIdent` enumerators.  Set the parameters to monitor via `setActiveRemoteObjects()` and receive value updates through `onRemoteObjectReceived`.  Write values via `setObjectValue()`.

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

All low-level callbacks (`onDataReceived`, `onConnectionEstablished`, `onConnectionLost`) fire on the **socket thread**.  The `callbacksOnMessageThread` constructor parameter is retained for API compatibility but has no effect — dispatch to another thread is the caller's responsibility if needed.

Controller callbacks (`onStateChanged`, `onPower`, `onChannelGain`, `onRemoteObjectReceived`, …) likewise fire on the **socket thread**.  If you need to update GUI elements or call framework APIs that require a specific thread (e.g. the JUCE message thread), marshal inside the callback — for example via `juce::MessageManager::callAsync` or by posting a message to a `juce::MessageListener`.

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

### Adding source files directly

1. Add `Source/` to your project's include paths.
2. Add all `.cpp` files from `Source/` and `Source/internal/` to your build target.
3. Require C++17 (`-std=c++17` / `cxx_std_17`).
4. On Windows, link `ws2_32`.

---

## Usage examples

### AmpController — typed amplifier control

```cpp
#include "AmpController.h"

auto amp = std::make_unique<NanoOcp1::AmpController>();

// Configure before connect
amp->setAmpType(NanoOcp1::AmpController::AmpType::Dy, 4 /*channels*/);

// Wire typed callbacks (fired on socket thread)
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

auto ds100 = std::make_unique<NanoOcp1::SoundscapeController>();

using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
using RO  = NanoOcp1::SoundscapeController::RemoteObject;

// Declare which parameters to subscribe and query on every connect
ds100->setActiveRemoteObjects({
    RO{ ROI::MatrixInput_LevelMeterPreMute, ROA{5, 0} },  // level meter SO 5
    RO{ ROI::Positioning_SourcePosition,   ROA{5, 0} },  // XYZ position SO 5
    RO{ ROI::Positioning_SourceSpread,     ROA{5, 0} },  // spread SO 5
    RO{ ROI::ReverbInput_Gain,             ROA{1, 5} },  // En-Space send gain SO 5
});

// Value-change callback (fired on socket thread)
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
    case ROI::MatrixInput_LevelMeterPreMute:
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

// 1. Create client (callbacks fire on the socket thread)
auto client = std::make_unique<NanoOcp1::NanoOcp1Client>(
    "192.168.1.100", 50014, /*callbacksOnMessageThread=*/false);

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
auto server = std::make_unique<NanoOcp1::NanoOcp1Server>(
    "", 50014, /*callbacksOnMessageThread=*/false);

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

`NanoOcp1Demo/main.cpp` is a JUCE-free **CLI application** with a live 19-row terminal panel (ANSI colours, macOS / Linux / Windows) that demonstrates both high-level controllers.  It operates in two modes selected at startup:

### Amp mode (`--amp`, default)

Connects to a d&b amplifier via `AmpController`.  Shows per-channel gain bars, mute state, and protection indicators (ISP / GR / OVL / headroom).

```
a <ip>         set host address          p <n>       set port
c              connect (or reconnect)    d           disconnect
1 / 0          power on / off
g <ch> <dB>    set channel gain  (ch: 1-4,  dB: -57.5 to +6.0)
m <ch> <1|0>   mute / unmute channel
q              quit
```

### Soundscape mode (`--soundscape <N>`)

Connects to a d&b Soundscape signal engine (DS100, DS110, DS100M, or vCore) via `SoundscapeController`.  Monitors and controls sound object N: level meter, XYZ position, spread, delay mode, En-Space send gain.  The exact device model is identified automatically via the GUID handshake.

```
a <ip>         set host address          p <n>       set port
c              connect (or reconnect)    d           disconnect
x/y/z <0-1>   set position XYZ
sp <0-1>       set spread
dm <0|1|2>     set delay mode  (0=off  1=compensate  2=reflect)
es <dB>        set En-Space send gain  (-57.5 to +6.0)
q              quit
```

### Running the demo

```bash
# Build (CMake)
cmake -B build -S . -DNANOOCP1_BUILD_DEMO=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Amp mode (default) — connect to a d&b Dy amplifier with 4 channels
./build/NanoOcp1Demo/NanoOcp1Demo 192.168.1.100 50014 --amp --type dy --ch 4

# Soundscape mode — monitor and control sound object 5 (works for DS100/DS110/DS100M/vCore)
./build/NanoOcp1Demo/NanoOcp1Demo 192.168.1.100 50014 --soundscape 5
```

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
