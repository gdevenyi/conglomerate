#include  <internal_volume_io.h>
#include  <images.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING            input_filename, output_filename;
    int               x, y;
    pixels_struct     pixels;
    Colour            col;
    Real              r, g, b, strength;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &strength ) )
    {
        print( "Usage: %s input.rgb output.rgb  strength\n", argv[0] );
        return( 1 );
    }

    if( input_rgb_file( input_filename, &pixels ) != OK )
        return( 1 );

    if( pixels.pixel_type != RGB_PIXEL )
    {
        print( "Pixels must be RGB type.\n" );
        return( 1 );
    }

    for_less( x, 0, pixels.x_size )
    {
        for_less( y, 0, pixels.y_size )
        {
            col = PIXEL_RGB_COLOUR(pixels,x,y);
            r = get_Colour_r_0_1( col );
            g = get_Colour_g_0_1( col );
            b = get_Colour_b_0_1( col );
            col = make_Colour_0_1( r * strength, g * strength, b * strength );
            PIXEL_RGB_COLOUR(pixels,x,y) = col;
        }
    }

    if( output_rgb_file( output_filename, &pixels ) != OK )
        return( 1 );

    return( 0 );
}
