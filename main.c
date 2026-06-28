#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracks.h"

#define LINE_BUFFER_SIZE 512

static void discard_line_tail(FILE *file)
{
    int ch = 0;

    while ((ch = fgetc(file)) != '\n' && ch != EOF) {
    }
}

static int parse_double_arg(const char *text, double *value)
{
    char *end = NULL;
    double parsed = 0.0;

    errno = 0;
    parsed = strtod(text, &end);
    if (end == text || *end != '\0' || errno == ERANGE || !isfinite(parsed) ||
        parsed < 0.0 || parsed >= 360.0) {
        return 0;
    }

    *value = parsed;
    return 1;
}

static int parse_int_arg(const char *text, int *value)
{
    char *end = NULL;
    long parsed = 0;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE || parsed <= 0 ||
        parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static int parse_cli(int argc, char *argv[], const char **input_path, ReportOptions *options)
{
    int index = 2;

    if (argc < 2) {
        return 0;
    }

    *input_path = argv[1];
    options->use_sector = 0;
    options->use_top = 0;

    while (index < argc) {
        if (strcmp(argv[index], "--sector") == 0) {
            if (index + 2 >= argc ||
                !parse_double_arg(argv[index + 1], &options->sector_from_deg) ||
                !parse_double_arg(argv[index + 2], &options->sector_to_deg)) {
                return 0;
            }

            options->use_sector = 1;
            index += 3;
        } else if (strcmp(argv[index], "--top") == 0) {
            if (index + 1 >= argc || !parse_int_arg(argv[index + 1], &options->top_count)) {
                return 0;
            }

            options->use_top = 1;
            index += 2;
        } else {
            return 0;
        }
    }

    return 1;
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
    AnalysisReport report;
    ReportOptions options;
    const char *input_path = NULL;

    if (!parse_cli(argc, argv, &input_path, &options)) {
        fprintf(stderr, "Ошибка: неверные аргументы командной строки\n");
        return 3;
    }

    input = fopen(input_path, "r");
    if (input == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть файл '%s'\n", input_path);
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
        fprintf(stderr, "Ошибка: не удалось прочитать файл '%s'\n", input_path);
        fclose(input);
        return 1;
    }

    fclose(input);

    fprintf(stderr,
            "Итого: прочитано %d строк, принято %d, пропущено %d\n",
            read_count,
            accepted_count,
            skipped_count);

    if (accepted_count == 0) {
        return 2;
    }

    prepare_track_analysis(&tracks, &report);
    render_analysis_report(input_path, &tracks, &report, &options);

    return 0;
}
