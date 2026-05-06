## mini-redis

High-performance, in-memory key-value store using asynchronous I/O and multi-threaded request processing

---

### Features
* **Event-driven Architecture**: Utilizes `epoll` for efficient handling of thousands of concurrent connections
* **Thread Pool**: Custom thread pool implementation to minimize thread creation overhead
* **TTL & Expiry**: Keys support time-to-live with both active (background cleanup) and passive (on-access) eviction
* **AOF Persistence**: Crash-resilient logging of mutating commands using absolute timestamp tracking to eliminate TTL drift upon server recovery

---

### 🏗 Build & Run

#### Prerequisites
*   Linux (required for `epoll`)
*   C++20 compatible compiler (GCC/Clang)
*   CMake 3.10+

#### Compilation
```bash
# Debug mode
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Release mode
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

#### Running the Server
```bash
./mredis  # Default port: 4242
```

---

### ⌨️ Supported Commands

| Command | Description |
| :--- | :--- |
| `SET key value` | Stores a value |
| `GET key` | Retrieves value; returns `$-1` if expired or not found |
| `DEL key` | Deletes a key |
| `EXISTS key` | Checks key existence (returns `:1` or `:0`) |
| `EXPIRES key ttl` | Sets TTL in seconds |
| `EXPIREAT key timestamp_ms` | Sets TTL using absolute Unix timestamp in milliseconds (internal, used for AOF recovery) |

---

### Testing
Functional and stress tests are implemented using Python's `unittest`

```bash
# functional & stress tests (requires running server)
python3 tests/test_server.py


# persistence & crash-recovery tests (runs server itself)
python3 tests/test_persistance.py
```

---

### Roadmap
- [x] **Active Expiry**: Background cleanup of expired keys
- [x] **Multithreading**: Safe memory access via RW-lock
- [x] **Thread Pool**: Optimized task scheduling
- [x] **Persistence (AOF)**: Command logging for data recovery
- [ ] **Complex Types**: Support for Lists and Hashes
- [ ] **RESP Protocol**: Full Redis Serialization Protocol compatibility
