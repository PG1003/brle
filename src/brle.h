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
#include <bit>
#include <limits>
#include <iterator>
#include <type_traits>

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
    literal2 = 0x40,
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

static constexpr brle8 make_literal( size_t buffer )
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

}


template< typename InputIt, typename OutputIt >
auto encode( InputIt input, InputIt last, OutputIt output ) -> OutputIt
{
    using InputValueT = typename std::iterator_traits< InputIt >::value_type;
    
    static_assert( std::is_unsigned< InputValueT >::value );
    
    constexpr int data_bits = std::numeric_limits< InputValueT >::digits;

    InputValueT input_bufferd = 0;
    InputValueT bits          = 0; //*input++;
    int         bit_count     = 0; //data_bits;
    int         rlen          = 0;

    enum encode_state { init, zeros, ones } state = init;
    while( bit_count > 0 || input != last )
    {
        if( bit_count < data_bits && input != last )
        {
            input_bufferd = *input++;
            bits          = bits | ( input_bufferd << bit_count );
            bit_count     = bit_count + data_bits;
        }

        int count        = 0;
        const auto zeros = std::min( std::countr_zero( bits ), bit_count );
        const auto ones  = std::min( std::countr_one( bits ), bit_count );

        switch( state )
        {
        case encode_state::init:
            if( zeros > detail::literal_size )
            {
                count = zeros;
                rlen  = zeros;
                state = encode_state::zeros;
            }
            else if( ones > detail::literal_size )
            {
                count = ones;
                rlen  = ones;
                state = encode_state::ones;
            }
            else
            {
                count     = std::min( detail::literal_size, bit_count );
                *output++ = detail::make_literal( bits );
            }
            break;

        case encode_state::zeros:
            if( zeros > 0 )
            {
                count = std::min( detail::max_count - rlen, zeros );
                rlen  = rlen + count;

                assert( rlen <= detail::max_count );
                if( rlen == detail::max_count )
                {
                    *output++ = detail::make_zeros( detail::max_count );
                    state     = encode_state::init;
                }
            }
            else
            {
                count     = 1;
                *output++ = detail::make_zeros( rlen );
                state     = encode_state::init;
            }
            break;

        case encode_state::ones:
            if( ones > 0 )
            {
                count = std::min( detail::max_count - rlen, ones );
                rlen  = rlen + count;

                assert( rlen <= detail::max_count );
                if( rlen == detail::max_count )
                {
                    *output++ = detail::make_ones( detail::max_count );
                    state     = encode_state::init;
                }
            }
            else
            {
                count     = 1;
                *output++ = detail::make_ones( rlen );
                state     = encode_state::init;
            }
        }

        bit_count = bit_count - count;
        if( bit_count == 0 )
        {
            bits = 0;
        }
        else
        {
            const int input_shift = bit_count - data_bits;

            bits = bits >> count;
            bits = bits | ( input_shift >= 0 ? input_bufferd << input_shift : input_bufferd >> -input_shift );
        }
    }

    switch( state )
    {
    case encode_state::init:
        return output;

    case encode_state::zeros:
        *output = detail::make_zeros( rlen );
        break;

    case encode_state::ones:
        *output = detail::make_ones( rlen );
    }

    return ++output;
}

template< typename InputIt, typename OutputIt, typename OutputValueT = typename std::iterator_traits< OutputIt >::value_type >
auto decode( InputIt input, InputIt last, OutputIt output ) -> OutputIt
{
    static_assert( std::is_same< typename std::iterator_traits< InputIt >::value_type, brle8 >::value );
    static_assert( std::is_unsigned< OutputValueT >::value );
    
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
    }

    return output;
}

}

}
