#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    int              i, n_objects, n_points1, n_points2;
    Real             dx, dy, dz, dist_sq;
    File_formats     format;
    object_struct    **object_list;
    Point            *points1, *points2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s input1.obj  input2.obj  output.txt\n",
                  argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 )
    {
        print( "Couldn't read %s.\n", input1_filename );
        return( 1 );
    }

    n_points1 = get_object_points( object_list[0], &points1 );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 )
    {
        print( "Couldn't read %s.\n", input2_filename );
        return( 1 );
    }

    n_points2 = get_object_points( object_list[0], &points2 );

    if( n_points1 != n_points2 )
    {
        print( "Number of points not equal.\n" );
        return( 1 );
    }

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_points1 )
    {
        dx = Point_x(points1[i]) - Point_x(points2[i]);
        dy = Point_y(points1[i]) - Point_y(points2[i]);
        dz = Point_z(points1[i]) - Point_z(points2[i]);

        dist_sq = dx * dx + dy * dy + dz * dz;

        (void) output_real( file, dist_sq );
        (void) output_newline( file );
    }

    (void) close_file( file );

    return( 0 );
}
