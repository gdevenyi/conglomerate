#include  <internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    STRING               input_volume_filename, input_surface_filename;
    STRING               output_volume_filename;
    Real                 set_voxel, separations[MAX_DIMENSIONS];
    Real                 voxel_size;
    STRING               history;
    File_formats         format;
    Volume               volume, label_volume;
    int                  x, y, z, n_objects, label, voxel[MAX_DIMENSIONS];
    int                  sizes[MAX_DIMENSIONS];
    int                  range_changed[2][N_DIMENSIONS];
    object_struct        **objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_surface_filename ) ||
        !get_string_argument( "", &output_volume_filename ) )
    {
        print( "Usage: %s  in_volume.mnc  in_surface.obj  out_volume.mnc\n",
               argv[0] );
        return( 1 );
    }

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    label_volume = create_label_volume( volume, NC_BYTE );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    voxel_size = MIN3( FABS(separations[0]), FABS(separations[1]),
                       FABS(separations[2]) );

    print( "Scanning object to volume.\n" );

    scan_object_to_volume( objects[0], volume, label_volume, 1,
                           voxel_size / 2.0 );

    dilate_voxels_3d( volume, label_volume,
                      1.0, 1.0, 0.0, -1.0, 
                      0.0, 0.0, 0.0, -1.0,
                      1.0, EIGHT_NEIGHBOURS, range_changed );

    dilate_voxels_3d( volume, label_volume,
                      0.0, 0.0, 0.0, -1.0,
                      1.0, 1.0, 0.0, -1.0, 
                      0.0, EIGHT_NEIGHBOURS, range_changed );

    voxel[0] = 0;
    voxel[1] = 0;
    voxel[2] = 0;
    fill_connected_voxels( volume, label_volume, FOUR_NEIGHBOURS,
                           voxel, 0, 0, 2, 0.0, -1.0, range_changed );

    print( "Masking off external voxels.\n" );

    set_voxel = get_volume_voxel_min( volume );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                label = (int) get_volume_real_value( label_volume, x, y, z,
                                                     0, 0 );
                if( label == 2 )
                {
                    set_volume_real_value( volume, x, y, z, 0, 0,
                                           (Real) set_voxel );
                }
            }
        }
    }
    
    delete_object_list( n_objects, objects );
    delete_volume( label_volume );

    history = create_string( "Surface masked." );

    status = output_volume( output_volume_filename, NC_UNSPECIFIED,
                            FALSE, 0.0, 0.0, volume, history,
                            (minc_output_options *) NULL );

    delete_string( history );

    delete_volume( volume );

    return( status != OK );
}
