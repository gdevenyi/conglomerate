#include  <module.h>

#define  CHUNK_SIZE   1000000

private  void  extract_isosurface(
    Minc_file         minc_file,
    Volume            volume,
    int               spatial_axes[],
    General_transform *voxel_to_world_transform,
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    polygons_struct   *polygons );
private  void  extract_surface(
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    int               x_size,
    int               y_size,
    float             ***slices,
    int               slice_index,
    BOOLEAN           right_handed,
    int               spatial_axes[],
    General_transform *voxel_to_world_transform,
    int               ***point_ids[],
    polygons_struct   *polygons );

static  char    *dimension_names[] = { ANY_SPATIAL_DIMENSION,
                                       ANY_SPATIAL_DIMENSION,
                                       ANY_SPATIAL_DIMENSION };

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_volume_filename, *output_filename;
    Volume               volume;
    Real                 min_threshold, max_threshold;
    Real                 valid_low, valid_high;
    Minc_file            minc_file;
    BOOLEAN              binary_flag;
    int                  c, spatial_axes[N_DIMENSIONS];
    object_struct        *object;
    volume_input_struct  volume_input;
    General_transform    voxel_to_world_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print( "Usage: %s  volume_file  object_file  min_thresh max_thresh\n",
               argv[0] );
        print( "       [valid_low  valid_high]\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_threshold );
    (void) get_real_argument( min_threshold-1.0, &max_threshold );

    if( max_threshold < min_threshold )
    {
        binary_flag = FALSE;
        max_threshold = min_threshold;
    }
    else
        binary_flag = TRUE;

    (void) get_real_argument( 0.0, &valid_low );
    (void) get_real_argument( -1.0, &valid_high );

    if( start_volume_input( input_volume_filename, 3, dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != OK )
        return( 0 );

    copy_general_transform( &volume->voxel_to_world_transform,
                            &voxel_to_world_transform );

    for_less( c, 0, N_DIMENSIONS )
        spatial_axes[c] = volume->spatial_axes[c];

    delete_volume_input( &volume_input );
    delete_volume( volume );

    volume = create_volume( 2, dimension_names, NC_UNSPECIFIED, FALSE,
                            0.0, 0.0 );

    minc_file = initialize_minc_input( input_volume_filename, volume,
                                       (minc_input_options *) NULL );

    if( minc_file == (Minc_file) NULL )
        return( 1 );

    object = create_object( POLYGONS );

    extract_isosurface( minc_file, volume, spatial_axes,
                        &voxel_to_world_transform,
                        binary_flag,
                        min_threshold, max_threshold,
                        valid_low, valid_high, get_polygons_ptr(object) );

    (void) close_minc_input( minc_file );

    if( output_graphics_file( output_filename, BINARY_FORMAT, 1, &object ) !=OK)
        return( 1 );

    delete_object( object );

    delete_volume( volume );

    delete_marching_cubes_table();

    delete_general_transform( &voxel_to_world_transform );

    output_alloc_to_file( ".alloc_stats" );

    return( 0 );
}

private  void  input_slice(
    Minc_file         minc_file,
    Volume            volume,
    float             **slice )
{
    int    x, y, sizes[MAX_DIMENSIONS], x_size, y_size;
    Real   amount_done;

    while( input_more_minc_file( minc_file, &amount_done ) )
    {}

    (void) advance_input_volume( minc_file );

    get_volume_sizes( volume, sizes );
    x_size = sizes[X];
    y_size = sizes[Y];

    for_less( x, 0, x_size )
    {
        for_less( y, 0, y_size )
        {
            GET_VALUE_2D( slice[x][y], volume, x, y );
        }
    }
}

private  void  clear_points(
    int         x_size,
    int         y_size,
    int         ***point_ids )
{
    int    x, y, edge;

    for_less( x, 0, x_size )
    {
        for_less( y, 0, y_size )
        {
            for_less( edge, 0, N_DIMENSIONS )
                point_ids[x][y][edge] = -1;
        }
    }
}

private  void   get_world_point(
    Real                slice,
    Real                x,
    Real                y,
    int                 spatial_axes[],
    General_transform   *voxel_to_world_transform,
    Point               *point )
{
    int            c;
    Real           xw, yw, zw;
    Real           real_voxel[N_DIMENSIONS], voxel_pos[N_DIMENSIONS];

    real_voxel[0] = slice;
    real_voxel[1] = x;
    real_voxel[2] = y;

    for_less( c, 0, N_DIMENSIONS )
    {
        if( spatial_axes[c] >= 0 )
            voxel_pos[c] = real_voxel[spatial_axes[c]];
        else
            voxel_pos[c] = 0.0;
    }

    general_transform_point( voxel_to_world_transform,
                             voxel_pos[X], voxel_pos[Y], voxel_pos[Z],
                             &xw, &yw, &zw );

    fill_Point( *point, xw, yw, zw );
}

private  void  extract_isosurface(
    Minc_file         minc_file,
    Volume            volume,
    int               spatial_axes[],
    General_transform *voxel_to_world_transform,
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    polygons_struct   *polygons )
{
    int             n_slices, sizes[MAX_DIMENSIONS], x_size, y_size, slice;
    int             ***point_ids[2], ***tmp_point_ids;
    float           **slices[2], **tmp_slices;
    progress_struct progress;
    Surfprop        spr;
    Point           point000, point100, point010, point001;
    Vector          v100, v010, v001, perp;
    BOOLEAN         right_handed;

    get_world_point( 0.0, 0.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point000 );
    get_world_point( 1.0, 0.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point100 );
    get_world_point( 0.0, 1.0, 0.0, spatial_axes, voxel_to_world_transform,
                     &point010 );
    get_world_point( 0.0, 0.0, 1.0, spatial_axes, voxel_to_world_transform,
                     &point001 );

    SUB_POINTS( v100, point100, point000 );
    SUB_POINTS( v010, point010, point000 );
    SUB_POINTS( v001, point001, point000 );
    CROSS_VECTORS( perp, v100, v010 );

    right_handed = DOT_VECTORS( perp, v001 ) >= 0.0;

    n_slices = get_n_input_volumes( minc_file );

    get_volume_sizes( volume, sizes );
    x_size = sizes[X];
    y_size = sizes[Y];

    ALLOC2D( slices[0], x_size, y_size );
    ALLOC2D( slices[1], x_size, y_size );
    ALLOC3D( point_ids[0], x_size, y_size, N_DIMENSIONS );
    ALLOC3D( point_ids[1], x_size, y_size, N_DIMENSIONS );

    input_slice( minc_file, volume, slices[1] );

    clear_points( x_size, y_size, point_ids[0] );
    clear_points( x_size, y_size, point_ids[1] );

    Surfprop_a(spr) = 0.3;
    Surfprop_d(spr) = 0.6;
    Surfprop_s(spr) = 0.6;
    Surfprop_se(spr) = 30.0;
    Surfprop_t(spr) = 1.0;
    initialize_polygons( polygons, WHITE, &spr );

    initialize_progress_report( &progress, FALSE, n_slices-1,
                                "Extracting Surface" );

    for_less( slice, 0, n_slices-1 )
    {
        tmp_slices = slices[0];
        slices[0] = slices[1];
        slices[1] = tmp_slices;
        input_slice( minc_file, volume, slices[1] );

        tmp_point_ids = point_ids[0];
        point_ids[0] = point_ids[1];
        point_ids[1] = tmp_point_ids;
        clear_points( x_size, y_size, point_ids[1] );

        extract_surface( binary_flag, min_threshold, max_threshold,
                         valid_low, valid_high,
                         x_size, y_size, slices, slice,
                         right_handed, spatial_axes, voxel_to_world_transform,
                         point_ids, polygons );

        update_progress_report( &progress, slice+1 );
    }

    terminate_progress_report( &progress );

    if( polygons->n_points > 0 )
    {
        ALLOC( polygons->normals, polygons->n_points );
        compute_polygon_normals( polygons );
    }

    FREE2D( slices[0] );
    FREE2D( slices[1] );
    FREE3D( point_ids[0] );
    FREE3D( point_ids[1] );
}

private  int   get_point_index(
    int                 x,
    int                 y,
    int                 slice_index,
    voxel_point_type    *point,
    float               **slices[],
    int                 spatial_axes[],
    General_transform   *voxel_to_world_transform,
    BOOLEAN             binary_flag,
    Real                min_threshold,
    Real                max_threshold,
    int                 ***point_ids[],
    polygons_struct     *polygons )
{
    int            voxel[N_DIMENSIONS], edge, point_index;
    Real           value1, value2;
    Point          v1, v2, v, world_point;
    Point_classes  point_class;

    voxel[X] = x + point->coord[X];
    voxel[Y] = y + point->coord[Y];
    voxel[Z] = point->coord[Z];
    edge = point->edge_intersected;

    point_index = point_ids[voxel[Z]][voxel[X]][voxel[Y]][edge];
    if( point_index < 0 )
    {
        value1 = slices[voxel[Z]][voxel[X]][voxel[Y]];
        fill_Point( v1, voxel[X], voxel[Y], voxel[Z] + (Real) slice_index );

        ++voxel[edge];
        value2 = slices[voxel[Z]][voxel[X]][voxel[Y]];
        fill_Point( v2, voxel[X], voxel[Y], voxel[Z] + (Real) slice_index );

        --voxel[edge];

        point_class = get_isosurface_point( &v1, value1, &v2, value2,
                                            binary_flag,
                                            min_threshold, max_threshold, &v );

        get_world_point( Point_z(v), Point_x(v), Point_y(v),
                         spatial_axes, voxel_to_world_transform, &world_point );

        point_index = polygons->n_points;
        ADD_ELEMENT_TO_ARRAY( polygons->points, polygons->n_points,
                              world_point, CHUNK_SIZE );

        point_ids[voxel[Z]][voxel[X]][voxel[Y]][edge] = point_index;
    }

    return( point_index );
}

private  void  extract_surface(
    BOOLEAN           binary_flag,
    Real              min_threshold,
    Real              max_threshold,
    Real              valid_low,
    Real              valid_high,
    int               x_size,
    int               y_size,
    float             ***slices,
    int               slice_index,
    BOOLEAN           right_handed,
    int               spatial_axes[],
    General_transform *voxel_to_world_transform,
    int               ***point_ids[],
    polygons_struct   *polygons )
{
    int                x, y, *sizes, tx, ty, tz, n_polys, ind;
    int                p, point_index, poly, size, start_points, dir;
    voxel_point_type   *points;
    Real               corners[2][2][2];
    BOOLEAN            valid;

    for_less( x, 0, x_size-1 )
    {
        for_less( y, 0, y_size-1 )
        {
            valid = TRUE;
            for_less( tx, 0, 2 )
            for_less( ty, 0, 2 )
            for_less( tz, 0, 2 )
            {
                corners[tx][ty][tz] = slices[tz][x+tx][y+ty];
                if( (corners[tx][ty][tz] < min_threshold ||
                    corners[tx][ty][tz] > max_threshold) &&
                    valid_low <= valid_high &&
                    (corners[tx][ty][tz] < valid_low ||
                     corners[tx][ty][tz] > valid_high) )
                    valid = FALSE;
            }

            if( !valid )
                continue;

            n_polys = compute_isosurface_in_voxel( MARCHING_NO_HOLES,
                                  corners, binary_flag, min_threshold,
                                  max_threshold, &sizes, &points );

            if( n_polys == 0 )
                continue;

            if( right_handed )
            {
                start_points = 0;
                dir = 1;
            }
            else
            {
                start_points = sizes[0]-1;
                dir = -1;
            }

            for_less( poly, 0, n_polys )
            {
                size = sizes[poly];

                start_new_polygon( polygons );

                /*--- orient polygons properly */

                for_less( p, 0, size )
                {
                    ind = start_points + p * dir;
                    point_index = get_point_index( x, y, slice_index,
                                   &points[ind],
                                   slices,
                                   spatial_axes, voxel_to_world_transform,
                                   binary_flag, min_threshold, max_threshold,
                                   point_ids, polygons );

                    ADD_ELEMENT_TO_ARRAY( polygons->indices,
                             polygons->end_indices[polygons->n_items-1],
                             point_index, CHUNK_SIZE );
                }

                if( right_handed )
                    start_points += size;
                else if( poly < n_polys-1 )
                    start_points += sizes[poly+1];
            }
        }
    }
}