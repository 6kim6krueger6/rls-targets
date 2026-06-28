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

typedef struct {
    TargetTrack targets[MAX_TARGETS];
    int target_count;
    int total_mark_count;
    int duplicate_count;
} TrackCollection;

typedef struct {
    int target_id;
    int mark_count;
    int min_range_m;
    int max_range_m;
    int min_range_time_ms;
    double avg_azimuth_deg;
    int has_speed;
    double speed_mps;
} TargetAnalysis;

typedef struct {
    TargetAnalysis items[MAX_TARGETS];
    int count;
} AnalysisReport;

typedef enum {
    PARSE_MARK_OK,
    PARSE_MARK_SKIP,
    PARSE_MARK_INVALID
} ParseMarkResult;

typedef enum {
    ADD_MARK_ADDED,
    ADD_MARK_DUPLICATE,
    ADD_MARK_NO_TARGET_SPACE,
    ADD_MARK_NO_MARK_SPACE
} AddMarkResult;

ParseMarkResult parse_radar_mark_line(const char *line, RadarMark *mark);
void init_track_collection(TrackCollection *collection);
AddMarkResult add_radar_mark(TrackCollection *collection, const RadarMark *mark);
void prepare_track_analysis(TrackCollection *collection, AnalysisReport *report);

#endif
