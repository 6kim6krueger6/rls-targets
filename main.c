#include <stdio.h>

#include "tracks.h"

int main(int argc, char *argv[])
{
    (void)argv;

    if (argc < 2) {
        fprintf(stderr, "Ошибка: укажите файл журнала\n");
        return 3;
    }

    fprintf(stderr, "Анализатор журнала РЛС пока содержит только каркас проекта.\n");
    return 0;
}
