#include <arguments.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <string.h>


static struct option long_options[] =
{
    {"profile",     required_argument,  0,  'p'},
    {"file",        required_argument,  0,  'f'},
    {"version",     no_argument,        0,  'v'},
    {"help",        no_argument,        0,  'h'},
    {"output",      required_argument,  0,  'o'},
    {"ignore",      required_argument,  0,  'i'},
    {0, 0, 0, 0}
};


static void print_help(void)
{
    char *helpstring = 
    "Usage: gcode_analyze [options]\n"
    "Options:\n"
    "  -p, --profile            Configuration file with profile settings.\n"
    "  -f, --file               GCode file to process.\n"
    "  -h, --help               This help message.\n"
    "  -i, --ignore             JSON file containing an array of coordinates to ignore.\n"
    "  -o, --output             Output format (JSON, XML)\n"
    "  -v, -?, --version        Version information.\n";
    printf(helpstring);
}


/* Determine the feedrate from the JSON profile */
static void parse_profile_feedrate(gcode_options *options)
{
    float fr_x, fr_y;
    bool fr_x_found = false, fr_y_found = false;
    json_t *axes, *axis, *speed;

    options->feedrate_set = false;
    if ((axes = json_object_get(options->profile, "axes")) != NULL) {     // Axes object is specified
        // Retrieve x axis speed:
        if ((axis = json_object_get(axes, "x")) != NULL) {
            if ((speed = json_object_get(axis, "speed")) != NULL) {
                fr_x = json_real_value(speed);
                fr_x_found = true;
            }
        }
        
        // Retrieve y axis speed:
        if ((axis = json_object_get(axes, "y")) != NULL) {
            if ((speed = json_object_get(axis, "speed")) != NULL) {
                fr_y = json_real_value(speed);
                fr_y_found = true;
            }
        }
    }

    if (fr_x_found && fr_y_found) {
        options->feedrate = fminf(fr_x, fr_y);
        options->feedrate_set = true;
    }
    else if (fr_x_found && !fr_y_found) {
        options->feedrate = fr_x;
        options->feedrate_set = true;
    }
    else if (!fr_x_found && fr_y_found) {
        options->feedrate = fr_y;
        options->feedrate_set = true;
    }
}


/* */
static void parse_profile_offsets(gcode_options *options)
{
    json_t *extruder, *offset_array, *offset, *val1, *val2;
    uint16 i;

    if ((extruder = json_object_get(options->profile, "extruder")) != NULL) {
        if ((offset_array = json_object_get(extruder, "offsets")) != NULL) {
            if (json_is_array(offset_array)) {
                // Fetch the offset for the two extruders:
                for (i = 0; i < 2; i++) {
                    if ((offset = json_array_get(offset_array, i)) != NULL) {  // Should be an X, Y tuple
                        val1 = json_array_get(offset, 0);
                        val2 = json_array_get(offset, 1);
                        if (val1 != NULL && val2 != NULL) {
                            options->offsets[i] = vector3D_init((float)json_real_value(val1), (float)json_real_value(val2), 0.0);
                        }
                        else { return; }
                    }
                }
                options->offsets_set = i;
            }
        }
    }
}


/* Parse a given JSON file with printer profiles
 * http://docs.octoprint.org/en/master/api/printerprofiles.html#profile
 */
static void parse_profile(gcode_options *options, const char *profile_fname)
{
    json_t *root;
    json_error_t error;
    FILE *profile_file = fopen(profile_fname, "r");

    // Try to read and parse the JSON printer profile file:
    if (!profile_file) {
        fprintf(stderr,"Unable to open printer profile file %s\n", profile_fname);
        exit(EXIT_FAILURE);
    }
    root = json_loadf(profile_file, 0, &error);     // No flags passed - default behavior
    if (!root) {
        fprintf(stderr, "Parsing error in printer profile file: on line %d: %s\n", error.line, error.text);
        exit(EXIT_FAILURE);
    }
    fclose(profile_file);
    options->profile = root;

    // Parse feedrate and offsets:
    parse_profile_feedrate(options);
    parse_profile_offsets(options);
}


/* Default options */
static void set_option_defaults(gcode_options *options)
{
    options->output = JSON;
    options->profile = NULL;
    options->feedrate = 2000;       // some somewhat sane default if axes speeds are insane...

    options->offsets[0] = NULL;
    options->offsets[1] = NULL;
    options->offsets_set = 0;

    options->ignore = NULL;
    options->ignores_set = 0;
}


static void create_ignore_coords(gcode_options *options, json_t *root)
{
    uint32 i;
    json_t *coord, *val;
    Vector3D *v;

    if ((options->ignores_set = json_array_size(root)) == 0) {
        fprintf(stderr,"JSON file with coordinates to ignore should be an array\n");
        exit(EXIT_FAILURE);
    }

    options->ignore = malloc(options->ignores_set * sizeof(Vector3D *));
    for (i = 0; i < options->ignores_set; i++) {
        v = vector3D_init2();                       // Initiate with an empty vector
        
        coord = json_array_get(root, i);            // Get the coordinate object
        if ((val = json_object_get(coord, "x")) != NULL) {
            v->x = json_real_value(val);
            v->x_none = false;
        }
        if ((val = json_object_get(coord, "y")) != NULL) {
            v->y = json_real_value(val);
            v->y_none = false;
        }
        if ((val = json_object_get(coord, "z")) != NULL) {
            v->z = json_real_value(val);
            v->z_none = false;
        }
        
        options->ignore[i] = v;
    }
}


static void parse_ignore_list(gcode_options *options, const char *ignore_fname)
{
    json_t *root;
    json_error_t error;
    FILE *ignore_file = fopen(ignore_fname, "r");

    // Try to read and parse the JSON array of coordinates to ignore
    if (!ignore_file) {
        fprintf(stderr,"Unable to open JSON file with coordinates to ignore %s\n", ignore_fname);
        exit(EXIT_FAILURE);
    }
    root = json_loadf(ignore_file, 0, &error);     // No flags passed - default behavior
    if (!root) {
        fprintf(stderr, "Parsing error in ignore file: on line %d: %s\n", error.line, error.text);
        exit(EXIT_FAILURE);
    }
    fclose(ignore_file);

    // Create vector3D coords for these:
    create_ignore_coords(options, root);
}


/* Parse all given commandline options: */
gcode_options *parse_options(int argc, char *argv[])
{
    int c, option_index;
    gcode_options *options;

    // Allocate, clear and set default options:
    options = malloc(sizeof(gcode_options));
    memset(options, '\0', sizeof(gcode_options));
    set_option_defaults(options);

    // Parse command line:
    while (true) {
        option_index = 0;
        c = getopt_long(argc, argv, "p:f:hi:o:v?", long_options, &option_index);
        if (c == -1) { break; }

        switch (c) {
            case 'p':
                parse_profile(options, optarg); break;
            case 'i':
                parse_ignore_list(options, optarg); break;
            case 'f':
                options->filename = optarg; break;
            case '?':
            case 'h':
                print_help(); exit(0);
            case 'o':
                if (!strcmp(optarg, "JSON")) { options->output = JSON; }
                else if (!strcmp(optarg, "XML")) { options->output = XML; }
                else {
                    fprintf(stderr,"Incorrect output format %s\n", optarg);
                    return NULL;
                }
                break;
            case 'v':
                printf("%s\n", GCODE_VERSION); exit(0);
            default:
                fprintf(stderr,"Unrecognized option %s\n", optarg);
                return NULL;
        }
    }
    return options;
}


void options_free(gcode_options *options)
{
    uint32 i;

    // First free all the ignores set
    for (i = 0; i < options->ignores_set; i++) {
        free(options->ignore[i]);
    }

    // Then the offsets
    for (i = 0; i < options->offsets_set; i++) {
        free(options->offsets[i]);
    }
    
    // TODO then in a proper way the json_t profile

    // Then the options themselves
    free(options);
}