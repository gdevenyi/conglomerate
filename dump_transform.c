#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input1.mnc  output.xfm\n\
\n\
     Dumps the voxel-to-world transform to a file.\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename, output_filename;
    char                 comment[EXTREMELY_LARGE_STRING_SIZE];
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume_header_only( volume_filename, 3, NULL, &volume, NULL) !=OK)
        return( 1 );

    (void) sprintf( comment, "dump_transform %s\n", volume_filename );

    (void) output_transform_file( output_filename, comment,
                                  get_voxel_to_world_transform(volume) );

    return( 0 );
}
