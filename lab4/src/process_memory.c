#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define SHW_ADR(ID, I) printf("ID %s is at virtual address: %p\n", ID, (void *)&I)
// etext — конец области кода (Text)
// edata — конец области инициализированных данных (Data)
// end — конец области неинициализированных данных (BSS)
extern int etext, edata, end;

// Инициализированная глобальная переменная — Data
char *cptr = "This message is output by the function showit()\n";

// Неинициализированный глобальный массив — BSS
char buffer1[25];

void showit(char *p);

// Функция main — Text
int main(void) {
    // Локальная переменная — Stack
    int i = 0;

    printf("\nAddress etext: %p\n", (void *)&etext);
    printf("Address edata: %p\n", (void *)&edata);
    printf("Address end:   %p\n", (void *)&end);

    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);

    strcpy(buffer1, "A demonstration\n");
    write(1, buffer1, strlen(buffer1) + 1);
    showit(cptr);

    return 0;
}

// Функция showit — Text
void showit(char *p) {
    // Локальная переменная-указатель — Stack
    char *buffer2;

    SHW_ADR("buffer2", buffer2);

    // Выделяемая через malloc память — Heap
    if ((buffer2 = malloc(strlen(p) + 1)) != NULL) {
        printf("Allocated memory at %p\n", (void *)buffer2);
        strcpy(buffer2, p);
        printf("%s", buffer2);
        free(buffer2);
    } else {
        printf("Allocation error\n");
        exit(1);
    }
}
