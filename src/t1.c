
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    const int* data;         // Указатель на массив
    size_t length;           // Длина массива
    int search_value;        // Искомое значение
    int found_position;      // Позиция найденного элемента (-1 если не найдено)
    pthread_mutex_t lock;    // Мьютекс для синхронизации
} ThreadSearchContext;

// Функция, выполняемая в каждом потоке
void* thread_search_function(void* context_ptr) {
    ThreadSearchContext* context = (ThreadSearchContext*)context_ptr;
    
    // Поиск значения в выделенной части массива
    for (size_t i = 0; i < context->length; i++) {
        if (context->data[i] == context->search_value) {
            // Блокируем мьютекс перед изменением общего ресурса
            pthread_mutex_lock(&context->lock);
            
            // Обновляем позицию, если нашли более раннее вхождение
            if (context->found_position == -1 || i < (size_t)context->found_position) {
                context->found_position = i;
            }
            
            // Разблокируем мьютекс
            pthread_mutex_unlock(&context->lock);
            
            // Прерываем поиск после первого найденного вхождения
            break;
        }
    }
    
    return NULL;
}

// Основная функция параллельного поиска
int find_first_occurrence(const int* array, size_t array_size, int target, int thread_count) {
    // Инициализация контекста поиска
    ThreadSearchContext search_context = {
        .data = array,
        .length = array_size,
        .search_value = target,
        .found_position = -1,
        .lock = PTHREAD_MUTEX_INITIALIZER
    };
    
    // Выделение памяти для идентификаторов потоков
    pthread_t* workers = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    if (workers == NULL) {
        perror("Failed to allocate memory for threads");
        return -1;
    }
    
    // Создание потоков
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&workers[i], NULL, thread_search_function, &search_context) != 0) {
            perror("Failed to create thread");
            free(workers);
            return -1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < thread_count; i++) {
        pthread_join(workers[i], NULL);
    }
    
    // Освобождение ресурсов
    free(workers);
    pthread_mutex_destroy(&search_context.lock);
    
    return search_context.found_position;
}

// Пример использования
int main() {
    int numbers[] = {1, 5, 3, 7, 5, 9, 2, 5, 8};
    size_t count = sizeof(numbers) / sizeof(numbers[0]);
    int value_to_find = 5;
    int worker_threads = 4;
    
    int position = find_first_occurrence(numbers, count, value_to_find, worker_threads);
    
    if (position != -1) {
        printf("First occurrence of %d found at position: %d\n", value_to_find, position);
    } else {
        printf("Value %d not found in the array\n", value_to_find);
    }
    
    return 0;
}
