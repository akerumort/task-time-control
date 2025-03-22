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

#define FIFO_NAME "/tmp/time_pipe" // для запуска на WSL
#define MAX_TIME 3.0 

int fifo_fd; // дескриптор FIFO
int num_tasks; 

// очистка при завершении
void cleanup(int signum) {
    std::cout << "\nКонтроллер завершает работу, удаление FIFO...\n";
    close(fifo_fd);
    unlink(FIFO_NAME);
    exit(0);
}

// запись в лог при превышении времени выполнения
void log_time_exceedance(int stage, double time_spent) {
    std::string log_path = (std::filesystem::current_path() / "execution_log.txt").string();
    
    std::ofstream logfile(log_path, std::ios::app);
    if (logfile.is_open()) {
        logfile << "Этап " << stage << " превысил время: " << time_spent << " сек\n";
        logfile.close();
    } else {
        std::cerr << "Ошибка открытия файла журнала: " << log_path << std::endl;
    }
}

// имитация выполнения задачи
void execute_task(int stage) {
    srand(getpid()); 

    struct timeval start, end;
    gettimeofday(&start, nullptr);

    // имитация работы до 5 секунд
    struct timespec ts;
    ts.tv_sec = 1 + rand() % 4; 
    ts.tv_nsec = rand() % 1000000000;  // задержка для предотвращения одновременного обращения к FIFO
    nanosleep(&ts, nullptr);

    gettimeofday(&end, nullptr);

    // вычисление затраченного времени в секундах
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // ожидание FIFO до 3 секунд
    for (int i = 0; i < 6; i++) {
        if (access(FIFO_NAME, F_OK) == 0) break;
        usleep(500000);
    }

    // открытие FIFO для записи
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Ошибка открытия FIFO");
        exit(1);
    }

    // формирование строки и отправление в FIFO
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%03d %.6f\n", stage, elapsed_time);
    write(fifo_fd, buffer, strlen(buffer));

    close(fifo_fd);
    exit(0);
}

int main() {
    std::cout << "Введите количество этапов: ";
    std::cin >> num_tasks;

    if (num_tasks <= 0) {
        std::cerr << "Ошибка: количество этапов должно быть положительным числом.\n";
        return 1;
    }

    // очистка перед созданием канала
    signal(SIGINT, cleanup);
    unlink(FIFO_NAME);

    // создание FIFO
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Ошибка создания FIFO");
        return 1;
    }

    std::cout << "Контроллер запущен. Запускаем " << num_tasks << " задач...\n";

    //зЗапуск num_tasks задач в отдельных процессах
    for (int i = 1; i <= num_tasks; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Ошибка fork()");
            return 1;
        }
        if (pid == 0) { // Дочерний процесс
            execute_task(i);
        }
    }

    // открытие FIFO для чтения
    fifo_fd = open(FIFO_NAME, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Ошибка открытия FIFO");
        unlink(FIFO_NAME);
        return 1;
    }

    std::string dataBuffer;
    char buffer[256];
    int received_tasks = 0; // полученные задачи

    // чтение данных из FIFO
    while (received_tasks < num_tasks) {
        ssize_t bytesRead = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            dataBuffer += buffer;

            // обработка строк
            size_t pos;
            while ((pos = dataBuffer.find('\n')) != std::string::npos) {
                std::string line = dataBuffer.substr(0, pos);
                dataBuffer.erase(0, pos + 1);

                int stage = -1;
                double time_spent = 0.0;

                if (sscanf(line.c_str(), "%d %lf", &stage, &time_spent) == 2) {
                    if (stage >= 1 && stage <= num_tasks) {
                        std::cout << "Этап " << stage << " выполнен. Время: " << time_spent << " сек\n";

                        if (time_spent > MAX_TIME) {
                            log_time_exceedance(stage, time_spent);
                        }

                        received_tasks++;
                    } else {
                        std::cerr << "Ошибка: некорректный номер этапа: " << stage << std::endl;
                    }
                } else {
                    std::cerr << "Ошибка разбора данных: " << line << std::endl;
                }
            }
        }
    }

    cleanup(0);
    return 0;
}
