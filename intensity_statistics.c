#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    static  char  usage_str[] = "\n\
Usage: %s  volume.mnc  input.tag|input.mnc|-  [dump_file]\n\
\n\
     Computes the statistics for the volume intensity of the volume.  If\n\
     an input tag or label file is specified, then only those voxels in\n\
     in the region of the tags or mask volume or considered.   If a\n\
     third argument is specified, all the intensities are placed in the\n\
     file.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *label_filename;
    char                 *dump_filename;
    Real                 x_min, x_max, mean, median, std_dev, *samples, value;
    Volume               volume, label_volume;
    Real                 separations[MAX_DIMENSIONS];
    Real                 min_world[MAX_DIMENSIONS], max_world[MAX_DIMENSIONS];
    Real                 min_voxel[MAX_DIMENSIONS], max_voxel[MAX_DIMENSIONS];
    Real                 real_v[MAX_DIMENSIONS], world[MAX_DIMENSIONS];
    FILE                 *file;
    BOOLEAN              dumping, labels_present, first;
    int                  c, n_samples, v[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &label_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    dumping = get_string_argument( NULL, &dump_filename );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_separations( volume, separations );

    labels_present = strcmp( label_filename, "-" ) != 0;

    if( labels_present )
    {
        label_volume = create_label_volume( volume, NC_UNSPECIFIED );

        set_all_volume_label_data( label_volume, 0 );

        if( filename_extension_matches( label_filename,
                                        get_default_tag_file_suffix() ) )
        {
            if( open_file( label_filename, READ_FILE, ASCII_FORMAT, &file )!=OK)
                return( 1 );

            if( input_tags_as_labels( file, volume, label_volume ) != OK )
                return( 1 );

            (void) close_file( file );
        }
        else
        {
            if( load_label_volume( label_filename, label_volume ) != OK )
                return( 1 );
        }
    }

    if( dumping )
    {
        if( open_file( dump_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );
    }

    n_samples = 0;

    first = TRUE;

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        if( !labels_present || get_volume_label_data( label_volume, v ) != 0 )
        {
            value = get_volume_real_value( volume, v[0], v[1], v[2], v[3],v[4]);

            ADD_ELEMENT_TO_ARRAY( samples, n_samples, value, 100000 );

            if( dumping )
            {
                if( output_real( file, value ) != OK ||
                    output_newline( file ) != OK )
                    return( 1 );
            }

            for_less( c, 0, N_DIMENSIONS )
                real_v[c] = (Real) v[c];

            convert_voxel_to_world( volume, real_v,
                                    &world[X], &world[Y], &world[Z]);

            if( first )
            {
                first = FALSE;
                for_less( c, 0, N_DIMENSIONS )
                {
                    min_voxel[c] = real_v[c];
                    max_voxel[c] = real_v[c];
                    min_world[c] = world[c];
                    max_world[c] = world[c];
                }
            }
            else
            {
                for_less( c, 0, N_DIMENSIONS )
                {
                    if( real_v[c] < min_voxel[c] )
                        min_voxel[c] = real_v[c];
                    else if( real_v[c] > max_voxel[c] )
                        max_voxel[c] = real_v[c];

                    if( world[c] < min_world[c] )
                        min_world[c] = world[c];
                    else if( world[c] > max_world[c] )
                        max_world[c] = world[c];
                }
            }
        }

    END_ALL_VOXELS

    if( dumping )
        (void) close_file( file );

    delete_volume( volume );

    if( labels_present )
        delete_volume( label_volume );

    if( n_samples > 0 )
    {
        compute_statistics( n_samples, samples, &x_min, &x_max,
                            &mean, &std_dev, &median );

        print( "N Voxels : %d\n", n_samples );
        print( "Volume   : %g\n",
               n_samples * separations[X] * separations[Y] * separations[Z] );
        print( "Min      : %g\n", x_min );
        print( "Max      : %g\n", x_max );
        print( "Mean     : %g\n", mean );
        print( "Median   : %g\n", median );
        print( "Std Dev  : %g\n", std_dev );
        print( "Voxel Rng:" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", min_voxel[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", max_voxel[c] );
        print( "\n" );
        print( "World Rng:" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", min_world[c] );
        print( "\n" );
        print( "         :" );
        for_less( c, 0, N_DIMENSIONS )
            print( " %g", max_world[c] );
        print( "\n" );
    }

    return( 0 );
}