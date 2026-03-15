# NanoOcp

NanoOcp is a C++ library built on the [JUCE](https://juce.com/) framework that provides a minimal **AES70 / OCP.1** TCP client and server, plus the message structures and device-specific object definitions needed to control AES70-compatible audio devices over a plain TCP connection.

Full API documentation is auto-generated from source and published at:
[![Documentation](https://img.shields.io/badge/docs-doxygen-blue)](https://ChristianAhrens.github.io/NanoOcp/)

|Appveyor CI build status|NanoOcp1Demo|
|:----------------|:-----|
| macOS Xcode           | [![Build status](https://ci.appveyor.com/api/projects/status/l680mk88wpwh8oq1?svg=true)](https://ci.appveyor.com/project/ChristianAhrens/nanoocp1demo-macos)   |
| Windows Visual Studio | [![Build status](https://ci.appveyor.com/api/projects/status/okx9i5ptj1bv6wnr?svg=true)](https://ci.appveyor.com/project/ChristianAhrens/nanoocp1demo-windows) |
| Linux makefile        | [![Build status](https://ci.appveyor.com/api/projects/status/18kcsabphbu2et4i?svg=true)](https://ci.appveyor.com/project/ChristianAhrens/nanoocp1demo-linux)   |

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
│   └── Ocp1DS100ObjectDefinitions.h# DS100-specific object definitions
├── NanoOcp1Demo/                   # Full JUCE demo application
│   ├── NanoOcp1Demo.jucer          # Projucer project file
│   └── Source/
│       ├── Main.cpp
│       ├── MainComponent.h/.cpp    # Demo UI: connect, subscribe, control a d&b amp
│       └── …
├── submodules/
│   ├── JUCE/                       # JUCE framework
│   └── doxygen-awesome-css/        # Doxygen HTML theme
├── Doxyfile                        # Doxygen configuration
└── .github/workflows/docs.yml      # GitHub Actions: generate + publish docs to gh-pages
```

---

## Architecture

NanoOcp is structured in three layers:

```
┌─────────────────────────────────────────────────────┐
│                  Your application                   │
│  onDataReceived / onConnectionEstablished / onLost  │
└──────────────────────┬──────────────────────────────┘
                       │  callbacks
┌──────────────────────▼──────────────────────────────┐
│          NanoOcp1Client  /  NanoOcp1Server           │  Layer 1 – Connection
│  (NanoOcp1Base + Ocp1Connection + juce::Timer)       │
└──────────────────────┬──────────────────────────────┘
                       │  ByteVector (raw OCP.1 frame)
┌──────────────────────▼──────────────────────────────┐
│   Ocp1Message  (Command / Response / Notification /  │  Layer 2 – Protocol
│                 KeepAlive)  +  Ocp1Header             │
│   Ocp1CommandResponseRequired  ←  Ocp1CommandDefinition│
└──────────────────────┬──────────────────────────────┘
                       │  Ocp1CommandDefinition subclasses
┌──────────────────────▼──────────────────────────────┐
│  Ocp1ObjectDefinitions  /  Ocp1DS100ObjectDefinitions│  Layer 3 – Device objects
│  dbOcaObjectDef_*  structs  (per parameter, per ONo) │
└─────────────────────────────────────────────────────┘
```

### Layer 1 — Connection (`NanoOcp1.h`)

`NanoOcp1Base` is the abstract base class that holds the target address/port and exposes three `std::function` callbacks:

| Callback | When fired |
|---|---|
| `onConnectionEstablished` | TCP connect succeeded |
| `onConnectionLost` | TCP connection dropped or failed |
| `onDataReceived(ByteVector)` | A complete OCP.1 frame arrived |

**`NanoOcp1Client`** — inherits `NanoOcp1Base`, `Ocp1Connection` (raw socket), and `juce::Timer`.  `start()` starts a periodic timer that retries `connectToSocket()` until it succeeds.  Reconnects automatically after a disconnect.

**`NanoOcp1Server`** — inherits `NanoOcp1Base` and `Ocp1ConnectionServer` (accept loop).  `start()` binds a port and waits for an incoming connection.  Only one simultaneous peer is supported.

### Layer 2 — Protocol (`Ocp1Message.h`)

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

### Layer 3 — Device objects (`Ocp1ObjectDefinitions.h`, `Ocp1DS100ObjectDefinitions.h`)

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

`NanoOcp1Client` runs all socket I/O on a dedicated `Ocp1Connection::ConnectionThread`.

The `callbacksOnMessageThread` constructor parameter controls where callbacks fire:

| Value | Callback thread | Use when |
|---|---|---|
| `false` | Socket thread (lower latency) | You manage your own thread safety in the callbacks |
| `true` | JUCE message thread (via `MessageManager::callAsync`) | Callbacks directly update UI components |

---

## Integration

NanoOcp has no CMake or Meson build system — it is designed to be added directly to a [Projucer](https://juce.com/discover/projucer) or CMake-based JUCE project.

1. Add the `Source/` directory to your project's include paths.
2. Add all `.cpp` files from `Source/` to your build target.
3. Ensure `juce_core` and `juce_events` JUCE modules are linked.

---

## Usage examples

### Client — connect, subscribe, get, set

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
    "", 50014, /*callbacksOnMessageThread=*/true);

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

`NanoOcp1Demo/` is a complete JUCE desktop application that demonstrates:

- Connecting to a d&b amplifier by IP/port.
- Sending `AddSubscription` commands for power state and gain.
- Displaying live property values as they arrive via notifications.
- Sending `SetValue` commands from UI controls (power on/off button, gain slider).

Open `NanoOcp1Demo/NanoOcp1Demo.jucer` in the Projucer, export a native project, and build with Xcode / Visual Studio / make.

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
