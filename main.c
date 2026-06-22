#include <stdio.h>
#include <string.h>

#include "tracks.h"

#define LINE_BUFFER_SIZE 512

static void discard_line_tail(FILE *file)
{
    int ch = 0;

    while ((ch = fgetc(file)) != '\n' && ch != EOF) {
    }
}

int main(int argc, char *argv[])
{
    FILE *input = NULL;
    char line[LINE_BUFFER_SIZE];
    int line_number = 0;
    int read_count = 0;
    int accepted_count = 0;
    int skipped_count = 0;
    TrackCollection tracks;

    if (argc != 2) {
        fprintf(stderr, "Ошибка: укажите ровно один файл журнала\n");
        return 3;
    }

    input = fopen(argv[1], "r");
    if (input == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть файл '%s'\n", argv[1]);
        return 1;
    }

    init_track_collection(&tracks);

    while (fgets(line, sizeof(line), input) != NULL) {
        RadarMark mark;
        ParseMarkResult result;
        AddMarkResult add_result;
        int line_too_long = strchr(line, '\n') == NULL && !feof(input);

        ++line_number;
        if (line_too_long) {
            discard_line_tail(input);
            ++read_count;
            ++skipped_count;
            fprintf(stderr, "Строка %d: некорректная запись\n", line_number);
            continue;
        }

        result = parse_radar_mark_line(line, &mark);
        if (result == PARSE_MARK_SKIP) {
            continue;
        }

        ++read_count;
        if (result != PARSE_MARK_OK) {
            ++skipped_count;
            fprintf(stderr, "Строка %d: некорректная запись\n", line_number);
            continue;
        }

        add_result = add_radar_mark(&tracks, &mark);
        if (add_result == ADD_MARK_ADDED) {
            ++accepted_count;
        } else if (add_result == ADD_MARK_DUPLICATE) {
            ++accepted_count;
            fprintf(stderr,
                    "Строка %d: дубль цели %d во время %d мс\n",
                    line_number,
                    mark.target_id,
                    mark.time_ms);
        } else {
            ++skipped_count;
            fprintf(stderr, "Строка %d: некорректная запись\n", line_number);
        }
    }

    if (ferror(input)) {
        fprintf(stderr, "Ошибка: не удалось прочитать файл '%s'\n", argv[1]);
        fclose(input);
        return 1;
    }

    fclose(input);

    printf("Итого: прочитано %d, принято %d, пропущено %d\n",
           read_count,
           accepted_count,
           skipped_count);

    if (accepted_count == 0) {
        return 2;
    }

    return 0;
}
