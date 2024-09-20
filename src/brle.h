// MIT License
//
// Copyright (c) 2021 PG1003
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cstdint>
#include <cassert>
#include <algorithm>
#include <limits>
#include <iterator>
#include <type_traits>

#if __cplusplus > 201703L
 #include <version>
#endif

#if defined( __cpp_lib_bitops )
 #include <bit>
#endif

namespace pg
{

namespace brle
{

using brle8 = uint8_t;

namespace detail
{

namespace mode
{

enum : brle8
{
    literal  = 0x00,
    zeros    = 0x80,
    ones     = 0xC0
};

}

static constexpr brle8 max_count    = 71;
static constexpr int   min_brle_len = 8;
static constexpr int   literal_size = 7;

static constexpr brle8 brle8_mode( brle8 rle )
{
    return rle & 0xC0;
}

static constexpr int count( const brle8 rle )
{
    return ( rle & 0x3F ) + min_brle_len;
}

template< typename T >
static constexpr brle8 make_literal( T buffer )
{
    return static_cast< brle8 >( buffer & 0x7F );
}

static constexpr brle8 make_zeros( int count )
{
    assert( count >= min_brle_len && count <= max_count );
    return static_cast< brle8 >( mode::zeros | ( count - min_brle_len ) );
}

static constexpr brle8 make_ones( int count )
{
    assert( count >= min_brle_len && count <= max_count );
    return static_cast< brle8 >( mode::ones | ( count - min_brle_len ) );
}

#if defined( __cpp_lib_bitops )

//
// Use perfect forwarding to create aliases for std::countr_zero and std::countr_one.
//

template< typename T >
constexpr auto countr_zero( T && val ) -> decltype( auto )
{
    return std::countr_zero( std::forward< T >( val ) );
}

template< typename T >
constexpr auto countr_one( T && val ) -> decltype( auto )
{
    return std::countr_one( std::forward< T >( val ) );
}

#else

//
// Surrogates for the std::countr_zero std::countr_one functions when NOT compiling with C++20.
//
// Inspired by:
//   https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
//   http://www.hakank.org/comb/debruijn_arb.cgi
//

template< typename T >
constexpr int countr_zero( const T value )
{
    static_assert( std::is_unsigned< T >::value, "expected an unsigned value" );

    if( value )
    {
        const uint64_t de_bruin_64  = 0x003B1234F32FD42B7;
        const int      lookup[ 64 ] = { 0,  1, 50,  2, 12, 51, 19,  3,
                                       16, 13, 52, 36, 32, 20,  4, 26,
                                       48, 17, 14, 24, 46, 53, 55, 37,
                                        9, 33, 21, 57, 60,  5, 27, 39,
                                       63, 49, 11, 18, 15, 35, 31, 25,
                                       47, 23, 45, 54,  8, 56, 59, 38,
                                       62, 10, 34, 30, 22, 44,  7, 58,
                                       61, 29, 43,  6, 28, 42, 41, 40 };

        const uint64_t hash  = ( value & ( ~value + 1u ) ) * de_bruin_64;
        const uint64_t index = hash >> 58;

        return lookup[ index ];
    }

    return 64;
}

template<>
constexpr int countr_zero< uint8_t >( const uint8_t value )
{
    if( value )
    {
        const uint8_t de_bruin_8  = 0x1D;
        const int     lookup[ 8 ] = { 0, 1, 6, 2, 7, 5, 4, 3 };

        const uint8_t hash  = ( value & ( ~value + 1u ) ) * de_bruin_8;
        const uint8_t index = hash >> 5;

        return lookup[ index ];
    }

    return 8;
}

template<>
constexpr int countr_zero< uint16_t >( const uint16_t value )
{
    if( value )
    {
        const uint32_t de_bruin_16  = 0x0D2F;
        const int      lookup[ 16 ] = { 0, 1, 8,  2,  6, 9,  3, 11,
                                       15, 7, 5, 10, 14, 4, 13, 12 };

        const uint16_t hash  = ( value & ( ~value + 1u ) ) * de_bruin_16;
        const uint16_t index = hash >> 12;

        return lookup[ index ];
    }

    return 16;
}

template<>
constexpr int countr_zero< uint32_t >( const uint32_t value )
{
    if( value )
    {
        const uint32_t de_bruin_32  = 0x077CB531;
        const int      lookup[ 32 ] = { 0,  1, 28,  2, 29, 14, 24, 3,
                                       30, 22, 20, 15, 25, 17, 4,  8,
                                       31, 27, 13, 23, 21, 19, 16, 7,
                                       26, 12, 18,  6, 11,  5, 10, 9 };

        const uint32_t hash  = ( value & ( ~value + 1u ) ) * de_bruin_32;
        const uint32_t index = hash >> 27;

        return lookup[ index ];
    }

    return 32;
}

template< typename T >
constexpr int countr_one( T value )
{
    return countr_zero( static_cast< T >( ~value ) );
}

#endif

}

template< typename DataT, typename OutputIt >
class encoder
{
    static_assert( std::is_unsigned< DataT >::value, "expected an unsigned input type" );

    enum class encode_state
    {
        init,
        zeros,
        ones
    };

    OutputIt     output      = {};
    DataT        buffer      = {};
    int          buffer_size = {};
    encode_state state       = encode_state::init;
    int          rlen        = {};

    void reset()
    {
        buffer      = {};
        buffer_size = {};
        state       = encode_state::init;
        rlen        = {};
    }

    constexpr int push( const DataT data, const int zeros, const int ones )
    {
        switch( state )
        {
        default:
            assert( !"should not be here!" );
            return 0;

        case encode_state::init:
            if( zeros > detail::literal_size )
            {
                rlen  = zeros;
                state = encode_state::zeros;

                return zeros;
            }
            if( ones > detail::literal_size )
            {
                rlen  = ones;
                state = encode_state::ones;

                return ones;
            }

            assert( rlen == 0 );
            *output++ = detail::make_literal( data );

            return detail::literal_size;

        case encode_state::zeros:
            if( zeros > 0 )
            {
                const auto consumed = std::min( detail::max_count - rlen, zeros );
                rlen                = rlen + consumed;

                assert( rlen <= detail::max_count );
                if( rlen == detail::max_count )
                {
                    *output++ = detail::make_zeros( detail::max_count );
                    state     = encode_state::init;
                    rlen      = 0;
                }

                return consumed;
            }

            *output++ = detail::make_zeros( rlen );
            state     = encode_state::init;
            rlen      = 0;

            return 1;

        case encode_state::ones:
            if( ones > 0 )
            {
                const auto consumed = std::min( detail::max_count - rlen, ones );
                rlen                = rlen + consumed;

                assert( rlen <= detail::max_count );
                if( rlen == detail::max_count )
                {
                    *output++ = detail::make_ones( detail::max_count );
                    state     = encode_state::init;
                    rlen      = 0;
                }

                return consumed;
            }

            *output++ = detail::make_ones( rlen );
            state     = encode_state::init;
            rlen      = 0;

            return 1;
        }
    }

public:
    constexpr encoder() = default;

    constexpr encoder( OutputIt output )
        : output( output )
    {}

    constexpr encoder( encoder && other )
        : output( std::move( other.output ) )
        , buffer( other.buffer )
        , buffer_size( other.buffer_size )
        , state( other.state )
        , rlen( other.rlen )
    {}

    ~encoder()
    {
        if( buffer_size > 0 || rlen > 0 )
        {
            flush();
        }
    }

    constexpr OutputIt push( const DataT data )
    {
        constexpr auto buffer_capacity = std::numeric_limits< DataT >::digits;

        auto shift_buffer = buffer;
        auto bits         = buffer_size;

        do
        {
            shift_buffer = shift_buffer | data << static_cast< DataT >( bits );

            const auto zeros    = detail::countr_zero( shift_buffer );
            const auto ones     = detail::countr_one( shift_buffer );
            const auto consumed = push( shift_buffer, zeros, ones );

            shift_buffer = shift_buffer >> consumed;
            bits         = bits - consumed;
        }
        while( ( bits + buffer_capacity ) >= buffer_capacity );

        if( bits >= 0 )
        {
            buffer = shift_buffer | data << static_cast< DataT >( bits );
        }
        else
        {
            buffer = data >> static_cast< DataT >( -bits );
        }
        buffer_size = bits + buffer_capacity;

        assert( buffer_size >= 0 );

        return output;
    }

    constexpr OutputIt flush()
    {
        while( buffer_size >= detail::literal_size ||
               state != encode_state::init )
        {
            const auto zeros    = std::min( detail::countr_zero( buffer ), buffer_size );
            const auto ones     = std::min( detail::countr_one( buffer ), buffer_size );
            const auto consumed = push( buffer, zeros, ones );

            buffer      = buffer >> static_cast< DataT >( consumed );
            buffer_size = buffer_size - consumed;
        }

        switch( state )
        {
        case encode_state::init:
            assert( buffer_size < detail::literal_size );
            if( buffer_size > 0 )
            {
                *output = detail::make_literal( buffer );
                break;
            }
            reset();
            return output;

        case encode_state::zeros:
            assert( rlen > detail::literal_size );
            *output = detail::make_zeros( rlen );
            break;

        case encode_state::ones:
            assert( rlen > detail::literal_size );
            *output = detail::make_ones( rlen );
            break;
        }

        reset();
        return ++output;
    }
};

template< typename InputIt, typename OutputIt >
constexpr auto encode( InputIt input, InputIt last, OutputIt output ) -> OutputIt
{
    using DataT = typename std::iterator_traits< InputIt >::value_type;

    encoder< DataT, OutputIt > e( output );

    while( input != last )
    {
        e.push( *input++ );
    }

    return e.flush();
}

template< typename InputIt, typename OutputIt, typename OutputValueT = typename std::iterator_traits< OutputIt >::value_type >
constexpr auto decode( InputIt input, InputIt last, OutputIt output ) -> OutputIt
{
    static_assert( std::is_same< typename std::iterator_traits< InputIt >::value_type, brle8 >::value,
                   "expected an input iterator that returns brle8 like type when dereferenced" );
    static_assert( std::is_unsigned< OutputValueT >::value,
                   "expected an unsigned value type as output" );

    constexpr int          data_bits = std::numeric_limits< OutputValueT >::digits;
    constexpr OutputValueT data_mask = std::numeric_limits< OutputValueT >::max();

    OutputValueT bits      = 0;
    int          bit_count = 0;

    while( input != last )
    {
        const auto in   = *input++;
        const auto mode = detail::brle8_mode( in );
        if( mode == detail::mode::zeros )
        {
            const auto count = detail::count( in );

            bit_count = bit_count + count;
            for( ; bit_count >= data_bits ; bit_count -= data_bits )
            {
                *output++ = bits;
                bits      = 0;
            }

            if( count < detail::max_count )
            {
                constexpr OutputValueT one = { 1 };

                bits      = bits | ( one << bit_count );
                bit_count = bit_count + 1;
            }
        }
        else if( mode == detail::mode::ones )
        {
            const auto count = detail::count( in );

            bits      = bits | ( data_mask << bit_count );
            bit_count = bit_count + count;
            for( ; bit_count >= data_bits ; bit_count -= data_bits )
            {
                *output++ = bits;
                bits      = data_mask;
            }

            if( bit_count > 0 )
            {
                bits = bits & ~( data_mask << bit_count );  // Clear remaining bits
            }
            else
            {
                bits = 0;
            }

            if( count < detail::max_count )
            {
                bit_count = bit_count + 1;
            }
        }
        else    // detail::mode::literal
        {
            bits      = bits | static_cast< OutputValueT >( in ) << bit_count;
            bit_count = bit_count + detail::literal_size;
        }

        if( bit_count >= data_bits )
        {
            *output++ = bits;
            bit_count = bit_count - data_bits;
            bits      = in >> ( detail::literal_size - bit_count );
        }

        if( bit_count == 0 )
        {
            bits = 0;
        }
    }

    return output;
}

}

}
