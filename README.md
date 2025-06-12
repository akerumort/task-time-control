<h1 align="center" id="title">task-time-control</h1>

<p align="center"><img src="https://socialify.git.ci/akerumort/task-time-control/image?description=1&language=1&name=1&owner=1&theme=Dark"></p>

This repository contains a C++ program that monitors the execution time of tasks across multiple stages. The program records the time taken for each stage, compares it against a predefined threshold, and logs any exceedances in a persistent log file. Communication between processes is achieved using named pipes (FIFO).

## ⚙ Features

- Multi-stage task execution with time monitoring
- Logging of time exceedances in a persistent file
- Inter-process communication using named pipes (FIFO)
- Simulation of task execution with randomized delays

## 🌐 Program Description

The program includes the following functionality:

1. **Time Monitoring**: Tracks the execution time of each task stage and compares it to a maximum allowed time (3 seconds).
2. **Log File Management**: Records instances where a stage exceeds the time limit in a log file (`execution_log.txt`), which is appended with each run.
3. **Named Pipe Communication**: Uses a FIFO pipe (`/tmp/time_pipe`) for inter-process communication to report stage completion times.
4. **Process Management**: Spawns separate processes for each task stage using `fork()` and handles cleanup on termination.


## 💻 Used Technologies

- **C++** (standard library for I/O, filesystem, and process management)
- **POSIX APIs** (for process creation, named pipes, and time management)
- **C++ Compiler** (e.g., GCC) with POSIX support

## 🐋 Installation

1. Install G++:
   ```bash
   sudo apt update && sudo apt install g++ -y
   g++ --version
   ```
2. Clone the repository:
   ```bash
   git clone https://github.com/akerumort/task-time-control.git
   cd task-time-control
   ```
3. Compile the program:
   ```bash
   g++ task_time_control.cpp -o task_time_control
   ```
4. Run the compiled program:
   ```bash
   ./task_time_control
   ```

## 🛡️ License

This project is licensed under the MIT License. See the `LICENSE` file for more details.

## ✉️ Contact

For any questions or inquiries, please contact `akerumort404@gmail.com`.
