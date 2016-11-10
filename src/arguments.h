/*
 * Command line arguments
 */

#ifndef ARGUMENTS_H
#define ARGUMENTS_H
 
#ifdef  __cplusplus
extern "C" {
#endif

#include <defs.h>
#include <vector.h>

#include <jansson.h>

typedef enum {JSON, XML} output_options_type;

typedef struct {
    char *filename;
    output_options_type output;
    json_t *profile;

    float feedrate;
    bool feedrate_set;
    Vector3D *offsets[2];       // TODO: max 2 extruders?
    ubyte offsets_set;          // Number of offsets parsed from profile
    Vector3D **ignore;          // List of pointers to vector3D's to ignore
    uint32 ignores_set;         // Number of coordinates to ignore
} gcode_options;


/* Try to parse command line options and return in a gcode_options struct. If unable, return NULL */
gcode_options *parse_options(int argc, char *argv[]);
void options_free(gcode_options *);


#ifdef  __cplusplus
}
#endif
 
#endif  /* ARGUMENTS_H */