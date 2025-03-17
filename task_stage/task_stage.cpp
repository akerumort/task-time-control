#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/time.h>

#define FIFO_NAME "/tmp/time_pipe" // для запуска на wsl

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <номер_этапа>\n";
        return 1;
    }

    int stage = std::atoi(argv[1]);

    // засекаем реальное время начала
    struct timeval start, end;
    gettimeofday(&start, nullptr);

    // имитация выполнения задачи (случайное время от 1 до 5 секунд)
    int work_time = 1 + rand() % 5;
    sleep(work_time);

    // засекаем реальное время окончания
    gettimeofday(&end, nullptr);

    // вычисляем затраченное время в секундах
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;

    // ожидание существования FIFO (до 3 секунд)
    for (int i = 0; i < 6; i++) {
        if (access(FIFO_NAME, F_OK) == 0) break;
        std::cerr << "Ожидание создания FIFO...\n";
        sleep(1);
    }

    // открываем именованный канал для записи
    int fifo_fd = open(FIFO_NAME, O_WRONLY);
    if (fifo_fd == -1) {
        perror("Ошибка открытия FIFO");
        return 1;
    }

    // формируем строку и отправляем через канал
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%d %.6f", stage, elapsed_time);
    write(fifo_fd, buffer, strlen(buffer));

    close(fifo_fd);
    return 0;
}
