#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>


int successful_rounds = 0; // колчиство совпадений 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// функция которая определяет совпадение в одной колоде 
int simulate_round() {
    int deck[52];
    
    // Заполняем колоду: 0-12 - пики, 13-25 - червы, 26-38 - бубны, 39-51 - трефы
    // Значение карты = номер % 13 (0-12 соответствует 2,3,4,...,Туз)
    for (int i = 0; i < 52; i++) {
        deck[i] = i;
    }
    
    // тусовка колоды
    for (int i = 0; i < 52; i++) {
        int j = rand() % 52;
        int temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
    
    // Проверяем, одинаковы ли две ВЕРХНИЕ карты по ЗНАЧЕНИЮ
    int top_card1 = deck[0] % 13;
    int top_card2 = deck[1] % 13;
    
    return (top_card1 == top_card2);
}

// Функция, которую выполняет каждый поток
void* thread_work(void* arg) {
    int* data = (int*)arg;
    int rounds = data[0];
    int thread_id = data[1];
    int local_success = 0;
    
    // Уникальный seed для каждого потока
    unsigned int seed = time(NULL) + thread_id;
    
    for (int i = 0; i < rounds; i++) {
        if (simulate_round()) {
            local_success++;
        }
    }
    
    // Безопасно добавляем результаты к общему счетчику
    pthread_mutex_lock(&mutex);
    successful_rounds += local_success;
    pthread_mutex_unlock(&mutex);
    
    printf("Поток %d: %d успешных из %d раундов\n", thread_id, local_success, rounds);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    
    // Проверяем аргументы командной строки
    if (argc != 3) {
        printf("Использование: %s <количество_раундов> <количество_потоков>\n", argv[0]);
        printf("Пример: %s 1000000 4\n", argv[0]);
        return 1;
    }
    
    int total_rounds = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    
    if (total_rounds <= 0 || num_threads <= 0) {
        printf("Ошибка: оба параметра должны быть положительными числами\n");
        return 1;
    }
    
    printf("=== Монте-Карло симуляция: две верхние карты одинаковы ===\n");
    printf("Всего раундов: %d\n", total_rounds);
    printf("Количество потоков: %d\n", num_threads);

    
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    // Рассчитываем, сколько раундов выполнит каждый поток
    int rounds_per_thread = total_rounds / num_threads;
    int extra_rounds = total_rounds % num_threads;
    
    printf("Распределение работы:\n");
    printf("- Базово на поток: %d раундов\n", rounds_per_thread);
    if (extra_rounds > 0) {
        printf("- Дополнительные раунды: %d\n", extra_rounds);
    }
    printf("\n");
    
    // Создаем потоки
    pthread_t threads[num_threads];
    int thread_data[num_threads][2]; // [0] - rounds, [1] - thread_id
    
    // Замеряем время начала
    clock_t start_time = clock();
    
    // Запускаем потоки
    for (int i = 0; i < num_threads; i++) {
        thread_data[i][0] = rounds_per_thread;
        thread_data[i][1] = i;
        
        // Первые несколько потоков получают дополнительные раунды
        if (i < extra_rounds) {
            thread_data[i][0]++;
        }
        
        if (pthread_create(&threads[i], NULL, thread_work, thread_data[i]) != 0) {
            // pthread_create(
            // &threads[i],           // указатель на переменную для сохранения дескриптора
            // NULL,                  // атрибуты потока (NULL = стандартные)
            // thread_work,           // функция, которую будет выполнять поток
            // thread_data[i]         // аргумент для функции
            // );
            perror("Ошибка создания потока");
            return 1;
        }
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Замеряем время окончания
    clock_t end_time = clock();
    double execution_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    // Вычисляем вероятность
    double probability = (double)successful_rounds / total_rounds;
    double theoretical = 3.0 / 51.0; // Теоретическая вероятность

    // Выводим результаты
    printf("\n=== РЕЗУЛЬТАТЫ ===\n");
    printf("Успешных раундов: %d из %d\n", successful_rounds, total_rounds);
    printf("Экспериментальная вероятность: %.6f (%.4f%%)\n", probability, probability * 100);
    printf("Теоретическая вероятность: %.6f (%.4f%%)\n", theoretical, theoretical * 100);
    printf("Разница: %.6f\n", probability - theoretical);
    printf("Время выполнения: %.3f секунд\n", execution_time);
    printf("Скорость: %.0f раундов/сек\n", total_rounds / execution_time);
    
    // Объяснение теоретического расчета
    printf("\n=== ТЕОРЕТИЧЕСКИЙ РАСЧЕТ ===\n");
    printf("После того как первая карта вытянута:\n");
    printf("- В колоде осталось 51 карта\n");
    printf("- Карт с таким же значением осталось: 3\n");
    printf("- Вероятность = 3/51 = %.6f\n", theoretical);
    
    // Информация для демонстрации потоков в системе
    printf("\n=== ДЕМОНСТРАЦИЯ ПОТОКОВ ===\n");
    printf("PID процесса: %d\n", getpid());
    printf("\nДля просмотра потоков выполните в другом окне терминала:\n");
    printf("ps -eLf | grep %d\n", getpid());
    printf("или: top -H -p %d\n", getpid());
    
    // Очищаем ресурсы
    pthread_mutex_destroy(&mutex);
    
    return 0;
}