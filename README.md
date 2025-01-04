# KSE (King Stock Exchange)

KSE is a stock exchange simulator written in C++ for educational purposes. It supports core trading functionalities such as adding, canceling, and modifying limit orders using a FIFO matching algorithm. The system processes client orders over TCP and provides real-time market data via UDP multicast.

## Features

- **Order Management**:  
  Add, cancel, and modify limit orders.
  
- **Matching Engine**:  
  - FIFO-based matching algorithm.  
  - Orders are matched from highest to lowest price for buy orders and lowest to highest price for sell orders.  
  - Orders at the same price and side are matched based on their arrival time.  
  
- **Real-Time Market Data**:  
  - Market data includes information on orders added, canceled, modified, and trades executed.  
  - Market data snapshots are available for clients experiencing packet loss.  
  
- **Networking**:  
  - TCP-based order server.  
  - UDP multicast for market data.  
  
- **Low-Latency Architecture**:  
  - Event-driven architecture powered by the `libuv` library.  
  - Lock-free queues for communication between threads.  

## Architecture

### Overview

![Architecture](https://github.com/spiraln/kse/blob/main/assets/arch.png?raw=true)

The system consists of the following main threads:

1. **Matching Engine Thread**:  
   - Processes orders received from the order server.  
   - Generates responses and market data.  

2. **Order Server Thread**:  
   - Accepts client connections.  
   - Forwards received orders to the matching engine via lock-free queues.  
   - Sends responses back to clients asynchronously.  

3. **Market Data Publisher Thread**:  
   - Receives market data from the matching engine.  
   - Publishes data via UDP multicast.  
   - Maintains market data snapshots for clients experiencing packet loss.  

### Communication
- Lock-free queues are used for:  
  - Sending requests from the order server to the matching engine.  
  - Receiving responses from the matching engine.  
  - Sending market data to the market data publisher.  

## Installation

### Prerequisites
- Install `libuv` on your system.

### Build Steps
1. Create a build directory:  
   ```bash
   mkdir -p cmake-build-release
   ```

2. Configure the build:  
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release -S .. -B cmake-build-release
   ```

3. Build the project:  
   ```bash
   cmake --build cmake-build-release --config Release
   ```

4. (Windows Only) Add the `libuv` DLL alongside the executable.

## Default Configuration

### Order Server
- **IP Address**: Any interface  
- **Port**: 54321  

### Market Data Publisher
- **Multicast Address**: 233.252.14.3  
- **Port**: 54323  

### Market Data Snapshot
- **Multicast Address**: 233.252.14.1  
- **Port**: 54322  

## Protocol

### Serialization
- The project uses a simple binary protocol for data serialization.  
- Relevant files:  
  - `src/order_server/serializer.hpp`  
  - `src/market_data/market_data_encoder.hpp`  
- Request, response, and market data structures are defined in `src/models`.  

## Usage
- An example of an algorithmic trading system communicating with the simulator is available in the `example` directory.

## Performance

### Measured Latencies
- **Order Execution Mean Latency**: 215 microseconds  

  ![Order Execution Latency](https://github.com/spiraln/kse/blob/main/assets/exec_lat.png?raw=true)

- **Order to Tick Mean Latency**: 705 microseconds  

  ![Order to Tick Latency](https://github.com/spiraln/kse/blob/main/assets/odt_lat.png?raw=true)

### Test System
- **OS**: Windows 11  
- **Compiler**: MSVC  
- **Processor**: 12th Gen Intel(R) Core(TM) i7-12700H @ 2.30 GHz  
- **Memory**: 16 GB RAM  
- **Cores**: 14 physical, 20 logical  
- **Cache**:  
  - L1: 1.2 MB  
  - L2: 11.5 MB  
  - L3: 24 MB  

### Performance Analysis
- Analyze performance data using the Jupyter notebook in the `perf_analysis` directory.  
- Logs for performance data are included in the accompanying files.  

## Limitations
- The project is intended for educational purposes and may contain bugs.  

## License
This project is licensed under MIT.
