# Qt Equipment Simulator

A robust desktop application simulating automated semiconductor manufacturing equipment. The application features live multi-sensor telemetry (position and temperature), full machine state transitions (`Idle`, `Homing`, `Running`, `Error`), asynchronous hardware simulation isolated on a dedicated worker thread, an isolated high-throughput producer-consumer logger thread synchronized with `std::mutex` and `std::condition_variable`, and structured cross-subsystem diagnostic logging.

## Architectural Patterns
- **State Pattern**: Enforces rigid machine lifecycle transitions (`Idle` -> `Homing` -> `Running` -> `Idle`, with fault triggers reaching `Error` from any active state and safe recovery via `Reset`).
- **Observer Pattern**: Qt signals and queued slot connections across thread boundaries guarantee non-blocking GUI updates.
- **Producer-Consumer Pattern**: Decoupled lock-guarded queue ferrying simulation readings to a dedicated file logger without blocking the simulation event loop.

## Build & Run Instructions
```bash
cmake -B build
cmake --build build
./build/equipment_sim
```

## Running Tests
```bash
cd build && ctest --output-on-failure
```

## Documentation
```bash
cd docs && doxygen Doxyfile
# View docs in docs/html/index.html
```
