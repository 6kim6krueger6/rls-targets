#include "tracks.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static const char *skip_spaces_and_tabs(const char *text)
{
    while (*text == ' ' || *text == '\t') {
        ++text;
    }

    return text;
}

static int read_int_field(const char **cursor, int *value)
{
    char *end = NULL;
    long parsed = 0;

    errno = 0;
    *cursor = skip_spaces_and_tabs(*cursor);
    if (**cursor == '\0' || **cursor == '\n' || **cursor == '#') {
        return 0;
    }

    parsed = strtol(*cursor, &end, 10);
    if (end == *cursor || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    *cursor = end;
    return 1;
}

static int read_double_field(const char **cursor, double *value)
{
    char *end = NULL;
    double parsed = 0.0;

    errno = 0;
    *cursor = skip_spaces_and_tabs(*cursor);
    if (**cursor == '\0' || **cursor == '\n' || **cursor == '#') {
        return 0;
    }

    parsed = strtod(*cursor, &end);
    if (end == *cursor || errno == ERANGE) {
        return 0;
    }

    *value = parsed;
    *cursor = end;
    return 1;
}

ParseMarkResult parse_radar_mark_line(const char *line, RadarMark *mark)
{
    RadarMark parsed;
    const char *cursor = skip_spaces_and_tabs(line);

    if (*cursor == '\0' || *cursor == '\n' || *cursor == '#') {
        return PARSE_MARK_SKIP;
    }

    if (!read_int_field(&cursor, &parsed.time_ms) ||
        !read_int_field(&cursor, &parsed.target_id) ||
        !read_int_field(&cursor, &parsed.range_m) ||
        !read_double_field(&cursor, &parsed.azimuth_deg)) {
        return PARSE_MARK_INVALID;
    }

    cursor = skip_spaces_and_tabs(cursor);
    if (*cursor != '\0' && *cursor != '\n' && *cursor != '#') {
        return PARSE_MARK_INVALID;
    }

    if (parsed.time_ms < 0 ||
        parsed.target_id < 0 ||
        parsed.range_m < 0 ||
        !isfinite(parsed.azimuth_deg) ||
        parsed.azimuth_deg < 0.0 ||
        parsed.azimuth_deg >= 360.0) {
        return PARSE_MARK_INVALID;
    }

    *mark = parsed;
    return PARSE_MARK_OK;
}
