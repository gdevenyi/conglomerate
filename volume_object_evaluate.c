#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, object_filename;
    STRING               output_filename;
    File_formats         format;
    Volume               volume;
    int                  point, n_points, n_objects;
    Point                *points;
    object_struct        **objects;
    Real                 value;
    FILE                 *file;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &object_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
           "Usage: %s  volume.mnc  object.obj  output_file.txt\n", argv[0]);
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    if( input_graphics_file( object_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects != 1 )
        print( "Warning, more than one object in file: %s\n", object_filename );

    n_points = get_object_points( objects[0], &points );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( point, 0, n_points )
    {
        evaluate_volume_in_world( volume,
                                  RPoint_x(points[point]),
                                  RPoint_y(points[point]),
                                  RPoint_z(points[point]),
                                  0, FALSE, 0.0,
                                  &value,
                                  NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL );

        if( output_real( file, value ) != OK ||
            output_newline( file ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    delete_object_list( n_objects, objects );

    return( 0 );
}