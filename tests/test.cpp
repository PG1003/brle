#include <brle.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <iterator>

static int total_checks  = 0;
static int failed_checks = 0;

static bool report_failed_check( const char* const file, const int line, const char * const condition )
{
    std::cout << "check failed! (file " << file << ", line " << line << "): " << condition << '\n';
    ++failed_checks;
    return false;
}

#define assert_true( c ) do { ++total_checks; ( c ) || report_failed_check( __FILE__, __LINE__, #c ); } while( false );
#define assert_false( c ) do { ++total_checks; !( c ) || report_failed_check( __FILE__, __LINE__, #c ); } while( false );


using namespace pg::brle;


template< typename T, size_t N, typename U = size_t >
static bool roundtrip( const T ( & in )[ N ], U init = U() )
{
    constexpr int len                = N * std::numeric_limits< T >::digits;
    constexpr int encode_buffer_size = ( 2 * len ) / 8; // Larger buffer required than the source size in case the result contains only literals.
    constexpr int decode_buffer_size = N;

    brle8 encode_buffer[ encode_buffer_size ] = { 0 };
    T    decode_buffer[ decode_buffer_size ] = { 0 };

    std::fill( std::begin( decode_buffer ), std::end( decode_buffer ), static_cast< T >( init ) );

    const auto encode_result = encode( std::begin( in ), std::end( in ), encode_buffer );
    const auto decode_result = decode( std::begin( encode_buffer ), encode_result, std::begin( decode_buffer ) );

    return decode_result == std::end( decode_buffer ) &&        // Filled the complete buffer && no overflow.
           memcmp( in, decode_buffer, ( 7 + len ) / 8 ) == 0;   // Same contents as in.
}

static void encode_decode_uint8()
{
    const uint8_t zeros[]         = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    const uint8_t ones[]          = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    const uint8_t literals[]      = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
    const uint8_t zerosones[]     = { 0x00, 0xFF };
    const uint8_t zerosliterals[] = { 0x00, 0xAA };
    const uint8_t oneszeros[]     = { 0xFF, 0x00 };
    const uint8_t onesliterals[]  = { 0xFF, 0x55 };
    const uint8_t literalszeros[] = { 0x55, 0x00 };
    const uint8_t literalsones[]  = { 0xAA, 0xFF };
    const uint8_t mixed[]         = { 0xAA, 0xAA, 0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xAA, 0x00 };
    const uint8_t weird[]         = { 0x00, 0x00, 0x80, 0x40 };

    assert_true( roundtrip( zeros, 0xFFu ) );
    assert_true( roundtrip( ones ) );
    assert_true( roundtrip( literals ) );
    assert_true( roundtrip( zerosones ) );
    assert_true( roundtrip( zerosliterals ) );
    assert_true( roundtrip( oneszeros ) );
    assert_true( roundtrip( onesliterals ) );
    assert_true( roundtrip( literalszeros ) );
    assert_true( roundtrip( literalsones ) );
    assert_true( roundtrip( mixed ) );
    assert_true( roundtrip( weird ) );
}

static void encode_decode_uint16()
{
    const uint16_t zeros[]         = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
    const uint16_t ones[]          = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    const uint16_t literals[]      = { 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA };
    const uint16_t zerosones[]     = { 0x00FF };
    const uint16_t zerosliterals[] = { 0x00AA };
    const uint16_t oneszeros[]     = { 0xFF00 };
    const uint16_t onesliterals[]  = { 0xFF55 };
    const uint16_t literalszeros[] = { 0x5500 };
    const uint16_t literalsones[]  = { 0xAAFF };
    const uint16_t mixed[]         = { 0xAAAA, 0xAAAA, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x00FF, 0xAA00 };

    assert_true( roundtrip( zeros, 0xFFFFu ) );
    assert_true( roundtrip( ones ) );
    assert_true( roundtrip( literals ) );
    assert_true( roundtrip( zerosones ) );
    assert_true( roundtrip( zerosliterals ) );
    assert_true( roundtrip( oneszeros ) );
    assert_true( roundtrip( onesliterals ) );
    assert_true( roundtrip( literalszeros ) );
    assert_true( roundtrip( literalsones ) );
    assert_true( roundtrip( mixed ) );
}

static void encode_decode_uint32()
{
    const uint32_t zeros[]             = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
    const uint32_t ones[]              = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    const uint32_t literals[]          = { 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA };
    const uint32_t zerosones[]         = { 0x00FF00FF };
    const uint32_t zerosliterals[]     = { 0x00AA00AA };
    const uint32_t oneszeros[]         = { 0xFF00FF00 };
    const uint32_t onesliterals[]      = { 0xFF55FF55 };
    const uint32_t literalszeros[]     = { 0x55005500 };
    const uint32_t literalsones[]      = { 0xAAFFAAFF };
    const uint32_t mixed[]             = { 0xAAAAAAAA, 0x00000000, 0xFFFFFFFF, 0x00FFAA00 };
    const uint32_t max_literal_ones[]  = { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
    const uint32_t max_literal_zeros[] = { 0x00FFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF };

    assert_true( roundtrip( zeros, 0xFFFFFFFFu ) );
    assert_true( roundtrip( ones ) );
    assert_true( roundtrip( literals ) );
    assert_true( roundtrip( zerosones ) );
    assert_true( roundtrip( zerosliterals ) );
    assert_true( roundtrip( oneszeros ) );
    assert_true( roundtrip( onesliterals ) );
    assert_true( roundtrip( literalszeros ) );
    assert_true( roundtrip( literalsones ) );
    assert_true( roundtrip( mixed ) );
    assert_true( roundtrip( max_literal_ones ) );
    assert_true( roundtrip( max_literal_zeros ) );
}

static void encode_decode_uint64()
{
    const uint64_t zeros[]         = { 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 };
    const uint64_t ones[]          = { 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF };
    const uint64_t literals[]      = { 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA, 0xAAAAAAAAAAAAAAAA };
    const uint64_t zerosones[]     = { 0x00FF00FF00FF00FF };
    const uint64_t zerosliterals[] = { 0x00AA00AA00AA00AA };
    const uint64_t oneszeros[]     = { 0xFF00FF00FF00FF00 };
    const uint64_t onesliterals[]  = { 0xFF55FF55FF55FF55 };
    const uint64_t literalszeros[] = { 0x5500550055005500 };
    const uint64_t literalsones[]  = { 0xAAFFAAFFAAFFAAFF };
    const uint64_t mixed[]         = { 0xAAAAAAAA00000000, 0xFFFFFFFF00FFAA00, 0xDEADBEEFFFFFFF00 };

    assert_true( roundtrip( zeros, 0xFFFFFFFFFFFFFFFFu ) );
    assert_true( roundtrip( ones ) );
    assert_true( roundtrip( literals ) );
    assert_true( roundtrip( zerosones ) );
    assert_true( roundtrip( zerosliterals ) );
    assert_true( roundtrip( oneszeros ) );
    assert_true( roundtrip( onesliterals ) );
    assert_true( roundtrip( literalszeros ) );
    assert_true( roundtrip( literalsones ) );
    assert_true( roundtrip( mixed ) );
}

static void bitmap_header()
{
    const uint8_t header[] =
    {
        0x42, 0x4d, 0xb6, 0xbb, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0xa5, 0x04, 0x00, 0x00, 0x48, 0x03, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0xbb, 0x2d, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    };

    assert_true( roundtrip( header ) );
}

static void readme_examples()
{
    {
        const uint8_t   data[ 8 ] = { 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xAA };
        pg::brle::brle8 rle[ 10 ] = { 0 };

        auto end = pg::brle::encode( std::cbegin( data ), std::cend( data ), std::begin( rle ) );
        
        assert_true( std::distance( rle, end ) == 3 );
    }
    {
        const pg::brle::brle8 rle[ 3 ]  = { 0xCC, 0x9C, 0x2A };
        uint8_t               data[ 8 ] = { 0 };

        auto end = pg::brle::decode( std::cbegin( rle ), std::cend( rle ), std::begin( data ) );
        
        assert_true( std::distance( data, end ) == 8 );
    }
    {
        const pg::brle::brle8   rle[ 3 ] = { 0xCC, 0x9C, 0x2A };
        std::vector< uint16_t > data;

        pg::brle::decode< const pg::brle::brle8 *,
                          std::back_insert_iterator< std::vector< uint16_t > >,
                          uint16_t >
                        ( std::begin( rle ), std::end( rle ), std::back_inserter( data ) );
                        
        assert_true( data.size() == 4u );
    }
}


int main()
{
    encode_decode_uint8();
    encode_decode_uint16();
    encode_decode_uint32();
    encode_decode_uint64();
    bitmap_header();
    readme_examples();

    std::cout << "Total tests: " << total_checks << ", Tests failed: " << failed_checks << '\n';

    return failed_checks ? 1 : 0;
}
