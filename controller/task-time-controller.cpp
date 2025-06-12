#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <sys/time.h>
#include <filesystem>

#define FIFO_NAME "/tmp/time_pipe" // for running on WSL
#define MAX_TIME 3.0 

int fifo_fd; // FIFO descriptor
int num_tasks;

// cleanup on termination
void cleanup(int signum) {
    std::cout << "\nController shutting down, removing FIFO...\n";
    close(fifo_fd);
    unlink(FIFO_NAME);
    exit(0);
}

// log time exceedance
void log_time_exceedance(int stage, double time_spent) {
    std::string log_path = (std::filesystem::current_path() / "execution_log.txt").string();

    std::ofstream logfile(log_path, std::ios::app);
    if (logfile.is_open()) {
        logfile << "Stage " << stage << " exceeded time: " << time_spent << " sec\n";
        logfile.close();
    }
    else {
        std::cerr << "Error opening log file: " << log_path << std::endl;
    }
}

// simulate task execution
void execute_task(int stage) {
    srand(getpid());

    struct timeval start, end;
    gettimeofday(&start, nullptr);

    // simulate work up to 5 seconds
    struct timespec ts;
    ts.tv_sec = 1 + rand() % 4;
    ts.tv_nsec = rand() % 1000000000;  // delay to prevent simultaneous FIFO access
    nanosleep(&ts, nullptr);

    gettimeofday(&end, nullptr);

    // calculate elapsed time in seconds
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // wait for FIFO up to 3 seconds
    for (int i = 0; i < 6; i++) {
        if (access(FIFO_NAME, F_OK) == 0) break;
        usleep(500000);
    }

    // open FIFO for writing
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Error opening FIFO");
        exit(1);
    }

    // form string and write to FIFO
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%03d %.6f\n", stage, elapsed_time);
    write(fifo_fd, buffer, strlen(buffer));

    close(fifo_fd);
    exit(0);
}

int main() {
    std::cout << "Enter number of stages: ";
    std::cin >> num_tasks;

    if (num_tasks <= 0) {
        std::cerr << "Error: number of stages must be positive.\n";
        return 1;
    }

    // cleanup before creating pipe
    signal(SIGINT, cleanup);
    unlink(FIFO_NAME);

    // create FIFO
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Error creating FIFO");
        return 1;
    }

    std::cout << "Controller started. Launching " << num_tasks << " tasks...\n";

    // launch num_tasks tasks in separate processes
    for (int i = 1; i <= num_tasks; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error in fork()");
            return 1;
        }
        if (pid == 0) { // child process
            execute_task(i);
        }
    }

    // Open FIFO for reading
    fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Error opening FIFO");
        unlink(FIFO_NAME);
        return 1;
    }

    std::string dataBuffer;
    char buffer[256];
    int received_tasks = 0; // received tasks

    // read data from FIFO
    while (received_tasks < num_tasks) {
        ssize_t bytesRead = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            dataBuffer += buffer;

            // process lines
            size_t pos;
            while ((pos = dataBuffer.find('\n')) != std::string::npos) {
                std::string line = dataBuffer.substr(0, pos);
                dataBuffer.erase(0, pos + 1);

                int stage = -1;
                double time_spent = 0.0;

                if (sscanf(line.c_str(), "%d %lf", &stage, &time_spent) == 2) {
                    if (stage >= 1 && stage <= num_tasks) {
                        std::cout << "Stage " << stage << " completed. Time: " << time_spent << " sec\n";

                        if (time_spent > MAX_TIME) {
                            log_time_exceedance(stage, time_spent);
                        }

                        received_tasks++;
                    }
                    else {
                        std::cerr << "Error: invalid stage number: " << stage << std::endl;
                    }
                }
                else {
                    std::cerr << "Error parsing data: " << line << std::endl;
                }
            }
        }
    }

    cleanup(0);
    return 0;
}