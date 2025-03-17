#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <filesystem>

#define FIFO_NAME "/tmp/time_pipe" // для запуска на wsl
#define MAX_TIME 3 // максимально допустимое время выполнения этапа в секундах

int fifo_fd; // Дескриптор FIFO

void cleanup(int signum) {
    std::cout << "\nКонтроллер завершает работу, удаление FIFO...\n";
    close(fifo_fd);
    unlink(FIFO_NAME);
    exit(0);
}

void log_time_exceedance(int stage, double time_spent) {
    std::string log_path = (std::filesystem::current_path() / "execution_log.txt").string();

    std::ofstream logfile(log_path, std::ios::app);
    if (logfile.is_open()) {
        logfile << "Этап " << stage << " превысил время: " << time_spent << " сек\n";
        logfile.close();
        std::cout << "Лог записан в: " << log_path << std::endl;
    } else {
        std::cerr << "Ошибка открытия файла журнала: " << log_path << std::endl;
    }
}

int main() {
    signal(SIGINT, cleanup);
    unlink(FIFO_NAME); // Удаляем FIFO, если он остался от прошлого запуска

    // создание именованного канала
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        perror("Ошибка создания FIFO");
        return 1;
    }

    std::cout << "Контроллер запущен. Ожидание данных от этапов...\n";

    fifo_fd = open(FIFO_NAME, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("Ошибка открытия FIFO");
        unlink(FIFO_NAME);
        return 1;
    }

    char buffer[256];

    while (true) {
        ssize_t bytesRead = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            int stage;
            double time_spent;
            sscanf(buffer, "%d %lf", &stage, &time_spent);

            std::cout << "Получены данные: Этап " << stage << ", время " << time_spent << " сек\n";

            if (time_spent > MAX_TIME) {
                log_time_exceedance(stage, time_spent);
            }
        }
        usleep(500000);
    }

    cleanup(0);
    return 0;
}
