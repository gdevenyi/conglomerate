#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_N_POINTS   100

private  void  match_histograms(
    lines_struct   *lines,
    lines_struct   *target,
    int            n_points,
    Real           scale,
    Real           oversize,
    lines_struct   *new_hist,
    lines_struct   *offsets );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.obj target.obj output.obj [n_points] [scale]\n\
\n\
     Matches the histograms.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int  argc,
    char *argv[] )
{
    STRING          input_filename, target_filename, output_filename;
    int             n_objects, n_points;
    Real            scale, oversize;
    File_formats    format;
    object_struct   **object_list, **output_objects;
    lines_struct    *lines, *target;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &target_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( DEFAULT_N_POINTS, &n_points );
    (void) get_real_argument( 1.0, &scale );
    (void) get_real_argument( 0.05, &oversize );

    if( input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list ) != OK ||
        n_objects != 1 || get_object_type(object_list[0]) != LINES )
    {
        print_error( "File %s must contain one lines object.\n",
                     input_filename );
        return( 1 );
    }

    lines = get_lines_ptr( object_list[0] );

    if( input_graphics_file( target_filename, &format, &n_objects,
                                  &object_list ) != OK ||
        n_objects != 1 || get_object_type(object_list[0]) != LINES )
    {
        print_error( "File %s must contain one lines object.\n",
                     target_filename );
        return( 1 );
    }

    target = get_lines_ptr( object_list[0] );

    ALLOC( output_objects, 2 );
    output_objects[0] = create_object( LINES );
    output_objects[1] = create_object( LINES );

    match_histograms( lines, target, n_points, scale, oversize,
                      get_lines_ptr( output_objects[0] ),
                      get_lines_ptr( output_objects[1] ) );

    (void) output_graphics_file( output_filename, format, 2, output_objects );

    return( 0 );
}

private  void  match_histograms(
    lines_struct   *lines,
    lines_struct   *target,
    int            n_targets,
    Real           scale,
    Real           oversize,
    lines_struct   *new_hist,
    lines_struct   *offsets )
{
    int              extra, p, int_pos, ip, **which, best_which;
    Real             min_pos, max_pos, pos, *target_values, *values;
    Real             min_x, max_x, target_scale, target_trans;
    Real             used_scale, used_trans, *best, *next_best, *tmp;
    Real             best_so_far, diff, x, value;
    progress_struct  progress;

    initialize_lines_with_size( new_hist, WHITE, lines->n_points, FALSE );
    initialize_lines( offsets, GREEN );

    min_x = (Real) Point_x(target->points[0]);
    max_x = (Real) Point_x(target->points[target->n_points-1]);

    target_scale = (max_x - min_x) / (Real) (target->n_points-1);
    target_trans = min_x;

    extra = ROUND( (Real) target->n_points * oversize );

    if( n_targets <= 1 )
        n_targets = target->n_points + 2 * extra;

    if( n_targets < lines->n_points )
        n_targets = lines->n_points;

    min_pos = (Real) - extra;
    max_pos = (Real) (target->n_points - 1 + extra);

    used_scale = target_scale * (max_pos - min_pos) / (Real) (n_targets-1);
    used_trans = target_scale * min_pos + target_trans;

    ALLOC( target_values, n_targets );
    for_less( p, 0, n_targets )
    {
        pos = INTERPOLATE( (Real) p / (Real) (n_targets-1), min_pos, max_pos );
        int_pos = FLOOR( pos );
        if( int_pos == target->n_points-1 && pos == (Real) int_pos )
            --int_pos;

        if( int_pos < 0 || int_pos >= target->n_points-1 )
            target_values[p] = 0.0;
        else
        {
            target_values[p] = ((Real) (int_pos+1) - pos) *
                                    (Real) Point_y(target->points[int_pos]) +
                               (pos - (Real) int_pos) *
                                    (Real) Point_y(target->points[int_pos+1]);
        }
    }

    ALLOC( values, lines->n_points );
    for_less( p, 0, lines->n_points )
        values[p] = scale * (Real) Point_y(lines->points[p]);

    ALLOC( best, n_targets );
    ALLOC( next_best, n_targets );
    ALLOC2D( which, lines->n_points, n_targets );

    for_less( p, 0, n_targets )
    {
        diff = values[0] - target_values[p];
        best[p] = diff * diff;
    }

    initialize_progress_report( &progress, FALSE, lines->n_points-1,
                                "Minimizing" );

    for_less( ip, 1, lines->n_points )
    {
        value = values[ip];

        best_so_far = best[ip-1];
        best_which = ip-1;
        for_less( p, ip, n_targets )
        {
            diff = value - target_values[p];
            diff = diff * diff;
            next_best[p] = best_so_far + diff * diff;
            which[ip][p] = best_which;
            if( best[p] < best_so_far )
            {
                best_so_far = best[p];
                best_which = p;
            }
        }

        tmp = best;
        best = next_best;
        next_best = tmp;

        update_progress_report( &progress, ip );
    }

    terminate_progress_report( &progress );

    for_less( p, lines->n_points-1, n_targets )
    {
        if( p == lines->n_points-1 || best[p] < best_so_far )
        {
            best_so_far = best[p];
            best_which = p;
        }
    }

    print( "Value: %g\n", best_so_far );

    for_down( p, lines->n_points-1, 0 )
    {
        x = used_scale * (Real) best_which + used_trans;
        fill_Point( new_hist->points[p],
                    x,
                    Point_y(lines->points[p]),
                    Point_z(lines->points[p]) );

        best_which = which[p][best_which];
    }

    FREE( which );
    FREE( best );
    FREE( next_best );
    FREE( target_values );
    FREE( values );
}