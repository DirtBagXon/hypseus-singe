// mpegscan.h
// by Matt Ownby

enum { P_ERROR, P_IN_PROGRESS, P_FINISHED };

int parse_system_stream(FILE *F, FILE *datafile, int &status);