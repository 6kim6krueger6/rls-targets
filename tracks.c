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

static TargetTrack *find_track(TrackCollection *collection, int target_id)
{
    int index = 0;

    for (index = 0; index < collection->target_count; ++index) {
        if (collection->targets[index].target_id == target_id) {
            return &collection->targets[index];
        }
    }

    return NULL;
}

static RadarMark *find_mark_by_time(TargetTrack *track, int time_ms)
{
    int index = 0;

    for (index = 0; index < track->mark_count; ++index) {
        if (track->marks[index].time_ms == time_ms) {
            return &track->marks[index];
        }
    }

    return NULL;
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

void init_track_collection(TrackCollection *collection)
{
    collection->target_count = 0;
    collection->total_mark_count = 0;
    collection->duplicate_count = 0;
}

AddMarkResult add_radar_mark(TrackCollection *collection, const RadarMark *mark)
{
    TargetTrack *track = find_track(collection, mark->target_id);
    RadarMark *duplicate = NULL;

    if (track == NULL && collection->total_mark_count >= MAX_MARKS) {
        return ADD_MARK_NO_MARK_SPACE;
    }

    if (track == NULL) {
        if (collection->target_count >= MAX_TARGETS) {
            return ADD_MARK_NO_TARGET_SPACE;
        }

        track = &collection->targets[collection->target_count];
        track->target_id = mark->target_id;
        track->mark_count = 0;
        ++collection->target_count;
    }

    duplicate = find_mark_by_time(track, mark->time_ms);
    if (duplicate != NULL) {
        if (mark->range_m < duplicate->range_m) {
            *duplicate = *mark;
        }
        ++collection->duplicate_count;
        return ADD_MARK_DUPLICATE;
    }

    if (collection->total_mark_count >= MAX_MARKS || track->mark_count >= MAX_MARKS) {
        return ADD_MARK_NO_MARK_SPACE;
    }

    track->marks[track->mark_count] = *mark;
    ++track->mark_count;
    ++collection->total_mark_count;
    return ADD_MARK_ADDED;
}

static void sort_marks_by_time_insertion(TargetTrack *track)
{
    int i = 0;

    for (i = 1; i < track->mark_count; ++i) {
        RadarMark key = track->marks[i];
        int j = i - 1;

        while (j >= 0 && track->marks[j].time_ms > key.time_ms) {
            track->marks[j + 1] = track->marks[j];
            --j;
        }

        track->marks[j + 1] = key;
    }
}

static void analyze_track(const TargetTrack *track, TargetAnalysis *analysis)
{
    int index = 0;
    double azimuth_sum = 0.0;

    analysis->target_id = track->target_id;
    analysis->mark_count = track->mark_count;
    analysis->has_speed = 0;
    analysis->speed_mps = 0.0;

    if (track->mark_count == 0) {
        analysis->min_range_m = 0;
        analysis->max_range_m = 0;
        analysis->min_range_time_ms = 0;
        analysis->avg_azimuth_deg = 0.0;
        return;
    }

    analysis->min_range_m = track->marks[0].range_m;
    analysis->max_range_m = track->marks[0].range_m;
    analysis->min_range_time_ms = track->marks[0].time_ms;

    for (index = 0; index < track->mark_count; ++index) {
        const RadarMark *mark = &track->marks[index];

        if (mark->range_m < analysis->min_range_m) {
            analysis->min_range_m = mark->range_m;
            analysis->min_range_time_ms = mark->time_ms;
        }
        if (mark->range_m > analysis->max_range_m) {
            analysis->max_range_m = mark->range_m;
        }

        azimuth_sum += mark->azimuth_deg;
    }

    analysis->avg_azimuth_deg = azimuth_sum / (double)track->mark_count;

    if (track->mark_count >= 3) {
        const RadarMark *first_mark = &track->marks[0];
        const RadarMark *last_mark = &track->marks[track->mark_count - 1];
        int delta_time_ms = last_mark->time_ms - first_mark->time_ms;

        if (delta_time_ms > 0) {
            analysis->has_speed = 1;
            analysis->speed_mps =
                -(double)(last_mark->range_m - first_mark->range_m) * 1000.0 /
                (double)delta_time_ms;
        }
    }
}

static void sort_analysis_by_min_range_selection(TargetAnalysis *items, int count)
{
    int i = 0;

    for (i = 0; i < count - 1; ++i) {
        int min_index = i;
        int j = 0;

        for (j = i + 1; j < count; ++j) {
            if (items[j].min_range_m < items[min_index].min_range_m) {
                min_index = j;
            }
        }

        if (min_index != i) {
            TargetAnalysis temp = items[i];
            items[i] = items[min_index];
            items[min_index] = temp;
        }
    }
}

void prepare_track_analysis(TrackCollection *collection, AnalysisReport *report)
{
    int index = 0;

    report->count = collection->target_count;

    for (index = 0; index < collection->target_count; ++index) {
        sort_marks_by_time_insertion(&collection->targets[index]);
        analyze_track(&collection->targets[index], &report->items[index]);
    }

    sort_analysis_by_min_range_selection(report->items, report->count);
}
