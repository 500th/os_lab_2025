#include "sum.h"

// Подсчёт суммы элементов заданного участка массива
long long Sum(const struct SumArgs *args) {
    long long sum = 0;
    for (int i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }
    return sum;
}
