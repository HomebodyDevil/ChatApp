# Chat Project Plan and Progress

## 1. Project Overview

This project aims to build a simple chat application with the following stack:

- **Server**: C++17, Boost.Asio, TCP socket communication
- **Client**: C#, WPF desktop application on Windows
- **Environment**: Windows development environment, Visual Studio + CMake recommended for server development

The goal is not just to make a basic chat app, but to build a project that can be used as a portfolio piece to demonstrate:

- asynchronous network programming
- session and connection lifecycle management
- client-server protocol design
- desktop UI development with WPF
- step-by-step architecture growth from MVP to extended features

---

## 2. Project Goals

### MVP Goals

The first playable and testable version should support the following:

- the server listens on a TCP port
- multiple clients can connect to the server
- a client can register a nickname
- a client can send a chat message
- the server broadcasts messages to connected clients
- join/leave system messages are displayed
- the WPF client can connect, send messages, and show chat logs

### Extended Goals

After the MVP is stable, the project can be expanded with:

- connected user list
- whisper/private message feature
- multiple chat rooms
- message timestamps
- JSON-based protocol
- reconnect handling
- server-side logging
- abnormal disconnect handling

---

## 3. Development Strategy

The recommended order of implementation is:

1. **Build the Boost.Asio server skeleton first**
2. **Verify networking with a simple test client or console-based TCP tool**
3. **Add session management and broadcast logic**
4. **Then build the WPF client**
5. **Refactor structure and add advanced features**

This order is important because starting from WPF too early usually causes duplicated work when the protocol changes.

---

## 4. Initial Protocol Plan

At the beginning, the protocol should remain simple and line-based.

### Client to Server

```text
NICK|UserA
MSG|Hello
```

### Server to Client

```text
SYSTEM|UserA joined
CHAT|UserA|Hello
SYSTEM|UserA left
```

### Planned Message Types

- `NICK|{nickname}`
- `MSG|{message}`
- `SYSTEM|{message}`
- `CHAT|{nickname}|{message}`

A JSON protocol may be introduced later after the MVP becomes stable.

---

## 5. Planned Project Structure

```text
ChatApp/
├─ server/
│  ├─ CMakeLists.txt
│  ├─ include/
│  │  ├─ chat_server.h
│  │  └─ client_session.h
│  └─ src/
│     ├─ main.cpp
│     ├─ chat_server.cpp
│     └─ client_session.cpp
└─ client/
   └─ ChatClient.Wpf/
```

---

## 6. What Has Been Implemented So Far

### 6.1 Server Skeleton

A minimal Boost.Asio TCP server structure has been created.

Implemented files:

- `main.cpp`
- `chat_server.h`
- `chat_server.cpp`
- `client_session.h`
- `client_session.cpp`

### 6.2 Server Startup Flow

The current startup flow is:

1. create `boost::asio::io_context`
2. create `work_guard`
3. create `ChatServer`
4. start listening on port `9000`
5. call `io_context.run()`

### 6.3 Accept Logic

The server currently:

- opens an acceptor on the configured port
- asynchronously waits for incoming connections
- logs client endpoint information when connected
- creates a `ClientSession` for the connected socket
- immediately starts the session
- schedules the next async accept

### 6.4 Session Read Logic

Each client session currently:

- owns its own TCP socket
- starts an asynchronous line-based read loop
- uses `async_read_until(..., '\n')`
- reads one line at a time from the stream buffer
- prints the received message to the console
- continues the read loop until disconnect or error

### 6.5 Current Achievement Level

At the current stage, the project has reached this milestone:

- server can start successfully
- server can accept a client connection
- server can read text line input from a client
- server can detect disconnect or read error

This means the project has completed the **"receive-only server MVP skeleton"** stage.

---

## 7. Problem Encountered During Development

### 7.1 Symptom

When a client connected, the server printed:

```text
[Server] Accept Handler Error : bad_weak_ptr
```

### 7.2 Root Cause

The issue came from this declaration in `client_session.h`:

```cpp
class ClientSession : std::enable_shared_from_this<ClientSession>
```

Because `class` uses **private inheritance by default**, `enable_shared_from_this` was inherited privately.

As a result, although the session object was created with `std::make_shared<ClientSession>(...)`, the internal weak ownership used by `shared_from_this()` was not set up correctly for intended use.

Then this line in `DoRead()` caused the crash:

```cpp
auto self = shared_from_this();
```

### 7.3 Fix

The inheritance must be changed to **public inheritance**:

```cpp
class ClientSession : public std::enable_shared_from_this<ClientSession>
```

After this fix, `shared_from_this()` can safely be used inside asynchronous handlers.

---

## 8. Additional Notes from Debugging

During stabilization, the following improvements were also identified as important:

- `work_guard` should be used to prevent `io_context.run()` from returning unexpectedly
- async handlers should be wrapped carefully to prevent the application from terminating due to uncaught exceptions
- `#pragma once` should be removed from `.cpp` files because it is only meaningful in header files

---

## 9. Current State Summary

The current implementation is **not yet a full chat server**, but it is already a solid base.

### Already Done

- basic server bootstrap
- asynchronous accept loop
- client session object creation
- line-based asynchronous read loop
- connection and disconnect logging
- root cause analysis for `bad_weak_ptr`
- solution identified and applied conceptually

### Not Yet Done

- session list management
- broadcast to all clients
- nickname registration
- protocol parsing
- system message propagation
- write queue
- WPF client implementation

---

## 10. Recommended Next Steps

The next implementation order should be:

### Step 1. Fix `ClientSession` inheritance

- change to `public std::enable_shared_from_this<ClientSession>`
- confirm that connection and read loop work without `bad_weak_ptr`

### Step 2. Add session ownership to `ChatServer`

- store active sessions in a container
- add join/remove logic
- prepare for broadcast support

### Step 3. Add write support to `ClientSession`

- implement `Send()`
- add write queue to avoid overlapping async writes

### Step 4. Add basic protocol parsing

- parse `NICK|...`
- parse `MSG|...`
- generate `SYSTEM|...` and `CHAT|...`

### Step 5. Add broadcast logic

- when one client sends a message, distribute it to all connected sessions

### Step 6. Build a temporary console test client or use TCP tools

- verify protocol correctness before moving to WPF

### Step 7. Start WPF client implementation

- connection UI
- message log UI
- send input UI
- network service layer
- MVVM structure

---

## 11. Suggested Milestone Plan

### Milestone 1
- server startup
- async accept
- line-based read
- disconnect handling

### Milestone 2
- session container
- broadcast
- nickname handling
- system messages

### Milestone 3
- console-based protocol verification

### Milestone 4
- WPF client MVP

### Milestone 5
- refactoring and advanced features

---

## 12. Suggested Commit Messages

```text
feat: add minimal Boost.Asio TCP server skeleton
feat: implement async accept and line-based read loop
fix: resolve bad_weak_ptr by using public enable_shared_from_this inheritance
feat: add session container for connected clients
feat: implement broadcast messaging
feat: add nickname registration protocol
feat: create WPF chat client MVP
refactor: separate network service from WPF UI logic
```

---

## 13. Short Summary

This project has been planned as a **Boost.Asio-based chat server + WPF desktop client** application.

So far, the server foundation has been implemented up to the point where it can:

- start listening
- accept client connections
- read line-based messages asynchronously
- detect disconnects

The main issue encountered was a `bad_weak_ptr` error caused by private inheritance of `enable_shared_from_this`, and the correct fix is to change it to public inheritance.

The next major goal is to move from a single-session read server into a real chat server by adding session management, broadcast logic, protocol parsing, and then building the WPF client on top of that stable foundation.
