#ifndef TRACKS_H
#define TRACKS_H

#define MAX_TARGETS 64
#define MAX_MARKS 1000

typedef struct {
    int time_ms;
    int target_id;
    int range_m;
    double azimuth_deg;
} RadarMark;

typedef struct {
    int target_id;
    RadarMark marks[MAX_MARKS];
    int mark_count;
} TargetTrack;

typedef enum {
    PARSE_MARK_OK,
    PARSE_MARK_SKIP,
    PARSE_MARK_INVALID
} ParseMarkResult;

ParseMarkResult parse_radar_mark_line(const char *line, RadarMark *mark);

#endif
