#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    const int* data_array;      // Указатель на массив данных
    size_t search_start;        // Начальный индекс для поиска
    size_t search_end;          // Конечный индекс для поиска
    int target_value;           // Искомое значение
    int* found_indices;         // Массив для хранения найденных индексов
    size_t* found_count;        // Счетчик найденных элементов
    pthread_mutex_t* lock;      // Мьютекс для синхронизации
} ThreadSearchData;

// Функция сравнения для сортировки (возрастающий порядок)
int compare_ascending(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

// Функция потока для поиска всех вхождений
void* thread_search_all_occurrences(void* arg) {
    ThreadSearchData* search_data = (ThreadSearchData*)arg;
    
    for (size_t i = search_data->search_start; i < search_data->search_end; i++) {
        if (search_data->data_array[i] == search_data->target_value) {
            // Блокируем мьютекс перед изменением общих данных
            pthread_mutex_lock(search_data->lock);
            
            // Добавляем найденный индекс в массив результатов
            search_data->found_indices[(*search_data->found_count)] = i;
            (*search_data->found_count)++;
            
            // Разблокируем мьютекс
            pthread_mutex_unlock(search_data->lock);
        }
    }
    
    return NULL;
}

// Основная функция параллельного поиска
int* find_all_occurrences(const int* array, size_t array_size, 
                         int target, int thread_count, size_t* result_size) {
    // Выделяем память для результатов (максимально возможный размер)
    int* results = malloc(array_size * sizeof(int));
    if (!results) {
        perror("Memory allocation failed");
        return NULL;
    }
    
    size_t count = 0;
    pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Выделяем память для потоков и их данных
    pthread_t* threads = malloc(thread_count * sizeof(pthread_t));
    ThreadSearchData* thread_data = malloc(thread_count * sizeof(ThreadSearchData));
    
    if (!threads || !thread_data) {
        free(results);
        free(threads);
        free(thread_data);
        perror("Memory allocation failed");
        return NULL;
    }
    
    // Распределяем работу между потоками
    size_t chunk_size = array_size / thread_count;
    for (int i = 0; i < thread_count; i++) {
        thread_data[i] = (ThreadSearchData){
            .data_array = array,
            .search_start = i * chunk_size,
            .search_end = (i == thread_count - 1) ? array_size : (i + 1) * chunk_size,
            .target_value = target,
            .found_indices = results,
            .found_count = &count,
            .lock = &result_mutex
        };
        
        if (pthread_create(&threads[i], NULL, thread_search_all_occurrences, &thread_data[i]) != 0) {
            perror("Thread creation failed");
            free(results);
            free(threads);
            free(thread_data);
            pthread_mutex_destroy(&result_mutex);
            return NULL;
        }
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Сортируем результаты по возрастанию
    qsort(results, count, sizeof(int), compare_ascending);
    
    // Освобождаем ресурсы
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&result_mutex);
    
    *result_size = count;
    return results;
}

// Пример использования
int main() {
    int test_data[] = {1, 5, 3, 7, 5, 9, 2, 5, 8, 5, 4, 5};
    size_t data_size = sizeof(test_data) / sizeof(test_data[0]);
    int value_to_find = 5;
    int worker_threads = 4;
    size_t found_count;
    
    int* found_indices = find_all_occurrences(test_data, data_size, 
                                            value_to_find, worker_threads, 
                                            &found_count);
    
    if (found_indices) {
        if (found_count > 0) {
            printf("Found %zu occurrences of %d at indices:", found_count, value_to_find);
            for (size_t i = 0; i < found_count; i++) {
                printf(" %d", found_indices[i]);
            }
            printf("\n");
        } else {
            printf("Value %d not found in the array\n", value_to_find);
        }
        free(found_indices);
    } else {
        printf("Search operation failed\n");
    }
    
    return 0;
}
