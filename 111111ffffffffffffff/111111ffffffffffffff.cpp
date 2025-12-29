#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAX_VERTICES 100
#define MAX_HISTORY 100
#define FILENAME "search_history.txt"
#define GRAPH_FILENAME "current_graph.txt"

// Структуры данных
typedef struct {
    int size;
    int matrix[MAX_VERTICES][MAX_VERTICES];
    int transposed[MAX_VERTICES][MAX_VERTICES];
    int is_generated;
} Graph;

typedef struct {
    int vertices[MAX_VERTICES];
    int count;
} Component;

typedef struct {
    char timestamp[50];
    int vertex_count;
    int component_count;
    Component components[MAX_VERTICES];
} SearchResult;

// Глобальные переменные
SearchResult history[MAX_HISTORY];
int history_count = 0;
int N = 7;
Graph current_graph;

// Прототипы функций
void save_to_file(SearchResult result);
void save_current_graph_to_file();
void load_history();
void view_history();
void transpose_graph(Graph* g);
void fill_order(int v, int visited[], int stack[], int* index, Graph* g);
void DFSUtil(int v, int visited[], Component* comp, Graph* g, int use_transposed);
void kosaraju(Graph* g, SearchResult* result, int show_steps);
void find_weak_connectivity(Graph* g, SearchResult* result, int show_steps);
void generate_directed_graph(Graph* g);
void manual_input_graph(Graph* g);
void set_vertex_count_and_generate();
void show_current_graph();
void print_matrix(Graph* g);
void print_connections(Graph* g);
void print_header();
void process_strong_connectivity();
void process_weak_connectivity();

// Вспомогательная функция для очистки буфера ввода
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Функции для работы с файлами
void save_to_file(SearchResult result) {
    FILE* file = fopen(FILENAME, "a");
    if (file == NULL) {
        printf("Ошибка открытия файла!\n");
        return;
    }

    fprintf(file, "Дата и время: %s\n", result.timestamp);
    fprintf(file, "Количество вершин: %d\n", result.vertex_count);
    fprintf(file, "Тип связности: ");

    // Определяем тип связности из названий функций
    if (strstr(result.timestamp, "СИЛЬНАЯ")) {
        fprintf(file, "Сильная связность (алгоритм Косарайю)\n");
    }
    else if (strstr(result.timestamp, "СЛАБАЯ")) {
        fprintf(file, "Слабая связность\n");
    }
    else if (strstr(result.timestamp, "НЕОРИЕНТИРОВАННЫЙ")) {
        fprintf(file, "Обычная связность (неориентированный граф)\n");
    }
    else {
        fprintf(file, "Не определен\n");
    }

    fprintf(file, "Количество компонент связности: %d\n", result.component_count);

    for (int i = 0; i < result.component_count; i++) {
        fprintf(file, "Компонента %d: ", i + 1);
        for (int j = 0; j < result.components[i].count; j++) {
            fprintf(file, "%d ", result.components[i].vertices[j] + 1);
        }
        fprintf(file, "\n");
    }
    fprintf(file, "----------------------------------------\n");
    fclose(file);

    printf("Результаты сохранены в файл %s\n", FILENAME);
}

void save_current_graph_to_file() {
    FILE* file = fopen(GRAPH_FILENAME, "w");
    if (file == NULL) {
        printf("Ошибка открытия файла для сохранения графа!\n");
        return;
    }

    fprintf(file, "Текущий граф (сохранен: ");
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[50];
    strftime(timestamp, sizeof(timestamp), "%d.%m.%Y %H:%M:%S", t);
    fprintf(file, "%s)\n", timestamp);

    fprintf(file, "Количество вершин: %d\n", current_graph.size);
    fprintf(file, "Матрица смежности:\n");

    for (int i = 0; i < current_graph.size; i++) {
        for (int j = 0; j < current_graph.size; j++) {
            fprintf(file, "%d ", current_graph.matrix[i][j]);
        }
        fprintf(file, "\n");
    }

    fprintf(file, "\nСписок исходящих связей:\n");
    for (int i = 0; i < current_graph.size; i++) {
        fprintf(file, "Вершина %d --> { ", i + 1);
        int has_connections = 0;
        for (int j = 0; j < current_graph.size; j++) {
            if (current_graph.matrix[i][j]) {
                fprintf(file, "%d ", j + 1);
                has_connections = 1;
            }
        }
        if (!has_connections) {
            fprintf(file, "нет исходящих связей");
        }
        fprintf(file, " }\n");
    }

    fclose(file);
    printf("Текущий граф сохранен в файл %s\n", GRAPH_FILENAME);
}

void load_history() {
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        history_count = 0;
        return;
    }

    char line[256];
    history_count = 0;

    while (fgets(line, sizeof(line), file) && history_count < MAX_HISTORY) {
        if (strstr(line, "Дата и время:")) {
            sscanf(line, "Дата и время: %49[^\n]", history[history_count].timestamp);
        }
        else if (strstr(line, "Количество вершин:")) {
            sscanf(line, "Количество вершин: %d", &history[history_count].vertex_count);
        }
        else if (strstr(line, "Количество компонент связности:")) {
            sscanf(line, "Количество компонент связности: %d",
                &history[history_count].component_count);
        }
        else if (strstr(line, "Компонента")) {
            int comp_index;
            char comp_data[256];
            sscanf(line, "Компонента %d: %[^\n]", &comp_index, comp_data);

            // Парсинг вершин компоненты
            char* token = strtok(comp_data, " ");
            int vertex_count = 0;
            while (token != NULL && vertex_count < MAX_VERTICES) {
                history[history_count].components[comp_index - 1].vertices[vertex_count] =
                    atoi(token) - 1;
                vertex_count++;
                token = strtok(NULL, " ");
            }
            history[history_count].components[comp_index - 1].count = vertex_count;

            // Если это последняя компонента, увеличиваем счетчик
            if (comp_index == history[history_count].component_count) {
                history_count++;
            }
        }
    }

    fclose(file);
}

void view_history() {
    system("cls");
    printf("=== ИСТОРИЯ ПОИСКА ===\n\n");

    if (history_count == 0) {
        printf("История поиска пуста.\n");
        printf("\nНажмите любую клавишу для продолжения...");
        clear_input_buffer();
        getchar();
        return;
    }

    for (int i = 0; i < history_count; i++) {
        printf("Запись %d:\n", i + 1);
        printf("  Дата и время: %s\n", history[i].timestamp);
        printf("  Количество вершин: %d\n", history[i].vertex_count);
        printf("  Количество компонент: %d\n", history[i].component_count);

        for (int j = 0; j < history[i].component_count; j++) {
            printf("  Компонента %d: ", j + 1);
            for (int k = 0; k < history[i].components[j].count; k++) {
                printf("%d ", history[i].components[j].vertices[k] + 1);
            }
            printf("\n");
        }
        printf("----------------------------------------\n");
    }

    printf("\nНажмите любую клавишу для продолжения...");
    clear_input_buffer();
    getchar();
}

// Функции алгоритма Косарайю
void transpose_graph(Graph* g) {
    if (g->size <= 10) {
        printf("\n=== ПРОЦЕСС ТРАНСПОНИРОВАНИЯ ГРАФА ===\n");
        printf("Исходная матрица смежности:\n");
        printf("   ");
        for (int j = 0; j < g->size; j++) printf("%3d", j + 1);
        printf("\n");
        for (int i = 0; i < g->size; i++) {
            printf("%3d", i + 1);
            for (int j = 0; j < g->size; j++) {
                printf("%3d", g->matrix[i][j]);
            }
            printf("\n");
        }
    }

    // Транспонирование: меняем строки и столбцы местами
    for (int i = 0; i < g->size; i++) {
        for (int j = 0; j < g->size; j++) {
            g->transposed[j][i] = g->matrix[i][j];
        }
    }

    if (g->size <= 10) {
        printf("\nТранспонированная матрица смежности:\n");
        printf("   ");
        for (int j = 0; j < g->size; j++) printf("%3d", j + 1);
        printf("\n");
        for (int i = 0; i < g->size; i++) {
            printf("%3d", i + 1);
            for (int j = 0; j < g->size; j++) {
                printf("%3d", g->transposed[i][j]);
            }
            printf("\n");
        }

        // Наглядное объяснение
        printf("\nНаглядное представление транспонирования:\n");
        printf("Каждое ребро меняет направление на противоположное:\n");
        for (int i = 0; i < g->size; i++) {
            for (int j = 0; j < g->size; j++) {
                if (g->matrix[i][j] == 1) {
                    printf("  Ребро %d --> %d становится %d --> %d\n",
                        i + 1, j + 1, j + 1, i + 1);
                }
            }
        }
        printf("Нажмите любую клавишу для продолжения...\n");
        clear_input_buffer();
        getchar();
    }
}

void fill_order(int v, int visited[], int stack[], int* index, Graph* g) {
    visited[v] = 1;

    for (int i = 0; i < g->size; i++) {
        if (g->matrix[v][i] && !visited[i]) {
            if (g->size <= 10) {
                printf("  Переход из вершины %d в вершину %d\n", v + 1, i + 1);
            }
            fill_order(i, visited, stack, index, g);
        }
    }

    // Добавляем вершину в стек после обработки всех её соседей
    stack[++(*index)] = v;
    if (g->size <= 10) {
        printf("  Вершина %d полностью обработана и добавлена в стек\n", v + 1);
    }
}

void DFSUtil(int v, int visited[], Component* comp, Graph* g, int use_transposed) {
    visited[v] = 1;
    comp->vertices[comp->count++] = v;

    for (int i = 0; i < g->size; i++) {
        int edge_exists = use_transposed ? g->transposed[v][i] : g->matrix[v][i];
        if (edge_exists && !visited[i]) {
            if (g->size <= 10) {
                printf("  Переход из вершины %d в вершину %d\n", v + 1, i + 1);
            }
            DFSUtil(i, visited, comp, g, use_transposed);
        }
    }
}

void kosaraju(Graph* g, SearchResult* result, int show_steps) {
    int stack[MAX_VERTICES];
    int stack_index = -1;
    int visited[MAX_VERTICES];

    // Инициализация массива посещенных вершин
    for (int i = 0; i < g->size; i++) {
        visited[i] = 0;
    }

    if (show_steps && g->size <= 10) {
        printf("\n========================================\n");
        printf("ШАГ 1: ПЕРВЫЙ ОБХОД В ГЛУБИНУ (DFS)\n");
        printf("Цель: Определить порядок завершения обработки вершин\n");
        printf("Стек заполняется в порядке убывания времени завершения\n");
        printf("========================================\n");
    }

    // ШАГ 1: Первый обход в глубину для заполнения стека
    for (int i = 0; i < g->size; i++) {
        if (!visited[i]) {
            if (show_steps && g->size <= 10) {
                printf("\nНачинаем обход из вершины %d:\n", i + 1);
            }
            fill_order(i, visited, stack, &stack_index, g);
        }
    }

    if (show_steps && g->size <= 10) {
        printf("\nСодержимое стека (вершины в порядке завершения обработки):\n");
        printf("Вершины: ");
        for (int i = 0; i <= stack_index; i++) {
            printf("%d ", stack[i] + 1);
        }
        printf("\n");
        printf("(Вершина вверху стека: %d, внизу: %d)\n",
            stack[stack_index] + 1, stack[0] + 1);
        printf("\nНажмите любую клавишу для продолжения...\n");
        clear_input_buffer();
        getchar();
    }

    // ШАГ 2: Транспонирование графа
    transpose_graph(g);

    // Сброс массива посещенных вершин для второго обхода
    for (int i = 0; i < g->size; i++) {
        visited[i] = 0;
    }

    if (show_steps && g->size <= 10) {
        printf("\n========================================\n");
        printf("ШАГ 3: ВТОРОЙ ОБХОД В ГЛУБИНУ\n");
        printf("Цель: Найти компоненты сильной связности\n");
        printf("Обход идет по транспонированному графу в порядке из стека\n");
        printf("Каждое дерево поиска - одна компонента сильной связности\n");
        printf("========================================\n");
    }

    // ШАГ 3: Второй обход в глубину по порядку из стека
    result->component_count = 0;
    while (stack_index >= 0) {
        int v = stack[stack_index--];

        if (!visited[v]) {
            if (show_steps && g->size <= 10) {
                printf("\n--- Начинаем обход из вершины %d (корень компоненты) ---\n", v + 1);
            }

            Component new_comp;
            new_comp.count = 0;
            DFSUtil(v, visited, &new_comp, g, 1);

            // Сохраняем найденную компоненту
            result->components[result->component_count] = new_comp;

            if (show_steps && g->size <= 10) {
                printf("Найдена компонента сильной связности %d: ", result->component_count + 1);
                for (int j = 0; j < new_comp.count; j++) {
                    printf("%d ", new_comp.vertices[j] + 1);
                }
                printf("\n");
            }

            result->component_count++;
        }
    }

    // Заполняем информацию о результате поиска
    result->vertex_count = g->size;
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(result->timestamp, sizeof(result->timestamp),
        "%d.%m.%Y %H:%M:%S [СИЛЬНАЯ СВЯЗНОСТЬ]", t);
}

// Функция для поиска слабой связности в орграфе
void find_weak_connectivity(Graph* g, SearchResult* result, int show_steps) {
    int visited[MAX_VERTICES] = { 0 };
    result->component_count = 0;
    result->vertex_count = g->size;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(result->timestamp, sizeof(result->timestamp),
        "%d.%m.%Y %H:%M:%S [СЛАБАЯ СВЯЗНОСТЬ]", t);

    if (show_steps && g->size <= 10) {
        printf("\n========================================\n");
        printf("АЛГОРИТМ ПОИСКА СЛАБОЙ СВЯЗНОСТИ\n");
        printf("========================================\n");
        printf("Принцип: игнорируем направление рёбер\n");
        printf("Считаем граф неориентированным:\n");
        printf("- Если есть ребро i->j, считаем связь i<->j\n");
        printf("- Если есть ребро j->i, считаем связь i<->j\n");
        printf("========================================\n");
    }

    for (int i = 0; i < g->size; i++) {
        if (!visited[i]) {
            Component comp;
            comp.count = 0;

            // Используем стек для DFS
            int stack[MAX_VERTICES];
            int top = -1;

            stack[++top] = i;
            visited[i] = 1;

            if (show_steps && g->size <= 10) {
                printf("\nНачинаем обход из вершины %d (корень компоненты %d):\n",
                    i + 1, result->component_count + 1);
            }

            while (top >= 0) {
                int v = stack[top--];
                comp.vertices[comp.count++] = v;

                if (show_steps && g->size <= 10) {
                    printf("  Обрабатываем вершину %d\n", v + 1);
                }

                // Проверяем все вершины, связанные в ЛЮБОМ направлении
                for (int j = 0; j < g->size; j++) {
                    // Если есть ребро в ЛЮБОМ направлении
                    if ((g->matrix[v][j] == 1 || g->matrix[j][v] == 1) && !visited[j]) {
                        visited[j] = 1;
                        stack[++top] = j;

                        if (show_steps && g->size <= 10) {
                            if (g->matrix[v][j] == 1 && g->matrix[j][v] == 1) {
                                printf("    Переход к вершине %d (двунаправленная связь)\n", j + 1);
                            }
                            else if (g->matrix[v][j] == 1) {
                                printf("    Переход к вершине %d (ребро %d->%d)\n", j + 1, v + 1, j + 1);
                            }
                            else {
                                printf("    Переход к вершине %d (обратное ребро %d->%d)\n",
                                    j + 1, j + 1, v + 1);
                            }
                        }
                    }
                }
            }

            // Сохраняем найденную компоненту
            result->components[result->component_count] = comp;

            if (show_steps && g->size <= 10) {
                printf("  Найдена компонента слабой связности %d: { ",
                    result->component_count + 1);
                for (int j = 0; j < comp.count; j++) {
                    printf("%d ", comp.vertices[j] + 1);
                }
                printf("} (размер: %d)\n", comp.count);
            }

            result->component_count++;
        }
    }
}

// Функции для работы с графами 
void generate_directed_graph(Graph* g) {
    srand(time(NULL));

    // ШАГ 1: Инициализация всей матрицы нулями
    for (int i = 0; i < g->size; i++) {
        for (int j = 0; j < g->size; j++) {
            g->matrix[i][j] = 0;
        }
    }

    // ШАГ 2: Генерация рёбер 
    printf("\nГенерация ориентированного графа...\n");
    printf("Правила генерации:\n");
    printf("1. Петли (ребра из вершины в себя) не генерируются\n");
    printf("2. Вероятность создания ребра между двумя разными вершинами: 40%%\n");
    printf("3. Ребра могут быть однонаправленными или двунаправленными\n");

    int edge_count = 0;
    for (int i = 0; i < g->size; i++) {
        for (int j = 0; j < g->size; j++) {
            if (i != j) { // Исключаем петли
                if (i < j) { // Обрабатываем только верхний треугольник для избежания дублирования
                    int probability = rand() % 100;

                    if (probability < 40) {
                        // Создаем однонаправленное или двунаправленное ребро
                        int direction = rand() % 3;

                        switch (direction) {
                        case 0: // i → j
                            g->matrix[i][j] = 1;
                            printf("  Создано ребро: %d -> %d\n", i + 1, j + 1);
                            edge_count++;
                            break;
                        case 1: // j → i
                            g->matrix[j][i] = 1;
                            printf("  Создано ребро: %d -> %d\n", j + 1, i + 1);
                            edge_count++;
                            break;
                        case 2: // i ↔ j (двунаправленное)
                            g->matrix[i][j] = 1;
                            g->matrix[j][i] = 1;
                            printf("  Создано двунаправленное ребро: %d <-> %d\n", i + 1, j + 1);
                            edge_count += 2;
                            break;
                        }
                    }
                }
            }
        }
    }

    printf("Всего создано ребер: %d\n", edge_count);

    // ШАГ 3: Гарантируем, что у каждой вершины есть хотя бы одно исходящее ребро
    for (int i = 0; i < g->size; i++) {
        int has_outgoing = 0;
        for (int j = 0; j < g->size; j++) {
            if (g->matrix[i][j] == 1) {
                has_outgoing = 1;
                break;
            }
        }

        if (!has_outgoing && g->size > 1) {
            // Добавляем случайное исходящее ребро
            int target;
            do {
                target = rand() % g->size;
            } while (target == i);

            g->matrix[i][target] = 1;
            printf("  Добавлено обязательное ребро: %d → %d\n", i + 1, target + 1);
            edge_count++;
        }
    }

    g->is_generated = 1;
    // Автоматически сохраняем граф в файл при генерации
    save_current_graph_to_file();
}

void manual_input_graph(Graph* g) {
    printf("\n=== РУЧНОЙ ВВОД ГРАФА ===\n");
    printf("Введите матрицу смежности (%dx%d):\n", g->size, g->size);
    printf("(0 - нет ребра, 1 - есть ребро)\n\n");

    for (int i = 0; i < g->size; i++) {
        printf("Строка %d (вершина %d): ", i + 1, i + 1);
        for (int j = 0; j < g->size; j++) {
            int value;
            scanf("%d", &value);
            if (value != 0 && value != 1) {
                printf("Ошибка: введите 0 или 1!\n");
                j--;
                continue;
            }
            g->matrix[i][j] = value;
        }
    }

    g->is_generated = 1;
    printf("\nГраф успешно введен вручную!\n");
    // Автоматически сохраняем граф в файл при ручном вводе
    save_current_graph_to_file();
}

void set_vertex_count_and_generate() {
    printf("Текущее количество вершин: %d\n", N);
    printf("Введите новое количество вершин (2-%d): ", MAX_VERTICES);

    int new_count;
    if (scanf("%d", &new_count) != 1) {
        printf("Некорректный ввод!\n");
        clear_input_buffer();
        printf("Нажмите любую клавишу для продолжения...");
        getchar();
        return;
    }

    if (new_count >= 2 && new_count <= MAX_VERTICES) {
        N = new_count;
        current_graph.size = N;
        current_graph.is_generated = 0;

        printf("Количество вершин изменено на %d\n", N);

        int choice;
        printf("\nВыберите способ создания графа:\n");
        printf("1. Сгенерировать случайный граф\n");
        printf("2. Ввести граф вручную\n");
        printf("Выберите (1 или 2): ");
        scanf("%d", &choice);

        if (choice == 1) {
            generate_directed_graph(&current_graph);
            printf("\nГраф успешно сгенерирован и сохранен в файл!\n");
        }
        else if (choice == 2) {
            manual_input_graph(&current_graph);
        }
        else {
            printf("Неверный выбор. Граф не создан.\n");
        }
    }
    else {
        printf("Некорректное значение! Допустимый диапазон: 2-%d\n", MAX_VERTICES);
    }

    printf("Нажмите любую клавишу для продолжения...");
    clear_input_buffer();
    getchar();
}

void show_current_graph() {
    system("cls");
    printf("=== ТЕКУЩИЙ ГРАФ ===\n\n");

    if (!current_graph.is_generated) {
        printf("Граф еще не создан!\n");
        printf("Используйте пункт меню 'Установить количество вершин и создать граф'\n");
    }
    else {
        printf("Количество вершин: %d\n", current_graph.size);
        printf("(Граф автоматически сохранен в файле %s)\n", GRAPH_FILENAME);

        print_matrix(&current_graph);
        print_connections(&current_graph);
    }

    printf("\nНажмите любую клавишу для продолжения...");
    clear_input_buffer();
    getchar();
}

void print_matrix(Graph* g) {
    printf("\n=== МАТРИЦА СМЕЖНОСТИ ===\n");
    printf("(1 - есть ребро, 0 - нет ребра)\n\n");

    printf("     ");
    for (int j = 0; j < g->size; j++) {
        printf("%4d  ", j + 1);
    }
    printf("\n");

    printf("     ");
    for (int j = 0; j < g->size; j++) {
        printf("------");
    }
    printf("\n\n");

    // Строки матрицы
    for (int i = 0; i < g->size; i++) {
        printf("%3d |", i + 1);
        for (int j = 0; j < g->size; j++) {
            printf("%4d  ", g->matrix[i][j]);
        }
        printf("\n\n");
    }
}

void print_connections(Graph* g) {
    printf("\n=== СПИСОК ИСХОДЯЩИХ СВЯЗЕЙ ===\n");
    printf("(Для ориентированного графа)\n\n");

    for (int i = 0; i < g->size; i++) {
        printf("Вершина %d --> { ", i + 1);
        int has_connections = 0;

        for (int j = 0; j < g->size; j++) {
            if (g->matrix[i][j]) {
                printf("%d ", j + 1);
                has_connections = 1;
            }
        }

        if (!has_connections) {
            printf("нет исходящих связей");
        }
        printf(" }\n");
    }
}

// функции меню
void print_header() {
    system("cls");
    printf("===================================================\n");
    printf("          КУРСОВАЯ РАБОТА\n");
    printf("          по дисциплине \"Логика и основы алгоритмизации\"\n");
    printf("          Тема: Реализация алгоритма поиска компонент связности\n");
    printf("                в орграфе, используя поиск в глубину\n\n");
    printf("          Студент: Тусков Арсений Андреевич\n");
    printf("          Группа: 24-ВВВ1\n");
    printf("          Преподаватель: Юрова О.В.\n");
    printf("===================================================\n\n");

    // Информация о текущем графе
    if (current_graph.is_generated) {
        printf("Текущий граф: создан (%d вершин), сохранен в %s\n",
            current_graph.size, GRAPH_FILENAME);
    }
    else {
        printf("Текущий граф: не создан\n");
    }
    printf("===================================================\n\n");
}

void process_strong_connectivity() {
    if (!current_graph.is_generated) {
        printf("\nОШИБКА: Граф не создан!\n");
        printf("Сначала создайте граф через пункт меню 'Установить количество вершин и создать граф'\n");
        printf("\nНажмите любую клавишу для продолжения...");
        clear_input_buffer();
        getchar();
        return;
    }

    printf("\n=== СИЛЬНАЯ СВЯЗНОСТЬ ОРИЕНТИРОВАННОГО ГРАФА ===\n");
    printf("(Алгоритм Косарайю)\n");
    printf("Используется текущий граф (%d вершин)\n\n", current_graph.size);

    print_matrix(&current_graph);
    print_connections(&current_graph);

    int show_steps;
    printf("\nПоказывать подробные шаги алгоритма Косарайю? (1 - да, 0 - нет): ");
    scanf("%d", &show_steps);

    SearchResult result;
    // Инициализация структуры результата
    result.component_count = 0;
    result.vertex_count = current_graph.size;

    kosaraju(&current_graph, &result, show_steps);

    printf("\n========================================\n");
    printf("ФИНАЛЬНЫЕ РЕЗУЛЬТАТЫ (СИЛЬНАЯ СВЯЗНОСТЬ)\n");
    printf("========================================\n");
    printf("Количество компонент сильной связности: %d\n", result.component_count);
    printf("Всего вершин в графе: %d\n", result.vertex_count);

    if (result.component_count == 1) {
        printf("\nГраф является СИЛЬНО СВЯЗНЫМ!\n");
        printf("Любая вершина достижима из любой другой.\n");
    }
    else if (result.component_count == result.vertex_count) {
        printf("\nКаждая вершина образует отдельную компоненту.\n");
        printf("В графе нет сильных связей между вершинами.\n");
    }

    printf("\nКомпоненты сильной связности:\n");
    for (int i = 0; i < result.component_count; i++) {
        printf("Компонента %d (размер: %d): { ", i + 1, result.components[i].count);
        for (int j = 0; j < result.components[i].count; j++) {
            printf("%d ", result.components[i].vertices[j] + 1);
        }
        printf("}\n");
    }

    char save;
    printf("\nСохранить результаты в файл? (y/n): ");
    clear_input_buffer();
    scanf(" %c", &save);

    if (save == 'y' || save == 'Y') {
        save_to_file(result);
        if (history_count < MAX_HISTORY) {
            history[history_count++] = result;
        }
        else {
            printf("История переполнена, результат не добавлен в историю.\n");
        }
    }

    printf("\nНажмите любую клавишу для продолжения...");
    clear_input_buffer();
    getchar();
}

void process_weak_connectivity() {
    if (!current_graph.is_generated) {
        printf("\nОШИБКА: Граф не создан!\n");
        printf("Сначала создайте граф через пункт меню 'Установить количество вершин и создать граф'\n");
        printf("\nНажмите любую клавишу для продолжения...");
        clear_input_buffer();
        getchar();
        return;
    }

    printf("\n=== СЛАБАЯ СВЯЗНОСТЬ ОРИЕНТИРОВАННОГО ГРАФА ===\n");
    printf("(Игнорируем направление рёбер)\n");
    printf("Используется текущий граф (%d вершин)\n\n", current_graph.size);

    print_matrix(&current_graph);
    print_connections(&current_graph);

    int show_steps;
    printf("\nПоказывать подробные шаги алгоритма? (1 - да, 0 - нет): ");
    scanf("%d", &show_steps);

    SearchResult result;
    // Инициализация структуры результата
    result.component_count = 0;
    result.vertex_count = current_graph.size;

    find_weak_connectivity(&current_graph, &result, show_steps);

    printf("\n========================================\n");
    printf("ФИНАЛЬНЫЕ РЕЗУЛЬТАТЫ (СЛАБАЯ СВЯЗНОСТЬ)\n");
    printf("========================================\n");
    printf("Количество компонент слабой связности: %d\n", result.component_count);
    printf("Всего вершин в графе: %d\n", result.vertex_count);

    if (result.component_count == 1) {
        printf("\nГраф является СЛАБО СВЯЗНЫМ!\n");
        printf("Если игнорировать направление рёбер, граф связен.\n");
    }
    else if (result.component_count == result.vertex_count) {
        printf("\nКаждая вершина изолирована.\n");
        printf("Даже без учета направления рёбер вершины не связаны.\n");
    }

    printf("\nКомпоненты слабой связности:\n");
    for (int i = 0; i < result.component_count; i++) {
        printf("Компонента %d (размер: %d): { ", i + 1, result.components[i].count);
        for (int j = 0; j < result.components[i].count; j++) {
            printf("%d ", result.components[i].vertices[j] + 1);
        }
        printf("}\n");
    }

    // ДОБАВЛЕНО: Запрос на сохранение результатов
    char save;
    printf("\nСохранить результаты в файл? (y/n): ");
    clear_input_buffer();
    scanf(" %c", &save);

    if (save == 'y' || save == 'Y') {
        save_to_file(result);
        if (history_count < MAX_HISTORY) {
            history[history_count++] = result;
        }
        else {
            printf("История переполнена, результат не добавлен в историю.\n");
        }
    }

    // ДОБАВЛЕНО: Ожидание нажатия клавиши
    printf("\nНажмите любую клавишу для продолжения...");
    clear_input_buffer();
    getchar();
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    // Инициализация текущего графа
    current_graph.size = N;
    current_graph.is_generated = 0;

    // Инициализация матриц нулями
    for (int i = 0; i < MAX_VERTICES; i++) {
        for (int j = 0; j < MAX_VERTICES; j++) {
            current_graph.matrix[i][j] = 0;
            current_graph.transposed[i][j] = 0;
        }
    }

    load_history();

    while (1) {
        print_header();

        printf("МЕНЮ:\n");
        printf("1. Установить количество вершин и создать граф\n");
        printf("2. Показать текущий граф\n");
        printf("3. Сильная связность (алгоритм Косарайю)\n");
        printf("4. Слабая связность\n");
        printf("5. Просмотреть историю поиска\n");
        printf("6. Выход\n");
        printf("\nВыберите пункт меню: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("Некорректный ввод!\n");
            clear_input_buffer();
            printf("Нажмите любую клавишу для продолжения...");
            getchar();
            continue;
        }

        switch (choice) {
        case 1:
            set_vertex_count_and_generate();
            break;
        case 2:
            show_current_graph();
            break;
        case 3:
            process_strong_connectivity();
            break;
        case 4:
            process_weak_connectivity();
            break;
        case 5:
            view_history();
            break;
        case 6:
            printf("Выход из программы...\n");
            return 0;
        default:
            printf("Некорректный выбор! Выберите пункт от 1 до 6.\n");
            printf("Нажмите любую клавишу для продолжения...");
            clear_input_buffer();
            getchar();
            break;
        }
    }

    return 0;
}