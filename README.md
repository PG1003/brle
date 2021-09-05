# A C++ library to compress or expand binary data using Run-Length Encoding

This library implements a variant of the Run-Length Encoding compression method that is optimized for sequences of ones or zeros.

The advantage of the RLE over other compression methods is that RLE can compress data in a single pass and does not require any buffering of the input or output data.
These properties can make a RLE good fit for applications that are tight on memory usage or require low latencies.
However due to simplicity of RLE the compression may not be as good as achieved by other compression methods.

## Features

* Optimized for sequences of ones or zeros.
* Robust
  * The encoder and decoder does not depend on previously processed data.
  * Fixed size RLE [data blocks](#Block-format).
* Compress and expand data in a single pass.
* No buffering of input data or output data.

## Requirements

A C++14 compiler when using the library or when building the test program.
The [brle utility](#brle-utility) requires at least a C++17 compliant compiler.

## Goals

* Use standard C++.
* Minimalistic.
* Flexibile in usage.

## Non-goals

* Optimazations for specific compilers and targets.  
  The source code of this library to compress and expand data is simple.
  Adapting and optimizing these functions to fit your application should be easy.
* Defining a container format.  
  This library focuses only on converting data to and from RLE.
  You have to take care for the information such as the original data size, length of the RLE stream, checksums, etc.

## Examples

Besides the following examples you can take a peek in the sources of the [test program](https://github.com/PG1003/brle/blob/main/tests/test.cpp) and the [brle utility](https://github.com/PG1003/brle/blob/main/util/brle.cpp) about the usage of the `brle` library.

### Encode

```c++
const uint8_t   data[ 8 ] = { 0xFF, 0xFF, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xAA };
pg::brle::brle8 rle[ 10 ] = { 0 };

auto end = pg::brle::encode( std::begin( data ), std::end( data ), rle );

assert( std::distance( rle, end ) == 3 );
```

### Decode

```c++
const pg::brle::brle8 rle[ 3 ]  = { 0xCC, 0x9C, 0x2A };
uint8_t               data[ 8 ] = { 0 };

auto end = pg::brle::decode( std::begin( rle ), std::end( data ), data );

assert( std::distance( data, end ) == 8 );
```

### Decoding using output iterators

The iterator traits used in the implementation of `pg::brle::decode` does not provide information about the underlaying data structure for output iterators such as returned by the `std::back_inserter` function.
In this rare case you need explicit provide all the template parameters.

```c++
const pg::brle::brle8   rle[ 3 ] = { 0xCC, 0x9C, 0x2A };
std::vector< uint16_t > data;

pg::brle::decode< const pg::brle::brle8 *,
                  std::back_insert_iterator< std::vector< uint16_t > >,
                  uint16_t >
                ( std::begin( rle ), std::end( rle ), std::back_inserter( data ) );

assert( data.size() == 4u );
```

## brle utility

The brle utility is an [implementation](https://github.com/PG1003/brle/blob/main/util/brle.cpp) for a commandline program that use this `brle` library.
The utility can be used to test the efficiency of the compression for your use case or to create binary blobs that are going to be included in your application or firmware.

You can build the utility with the provided makefile by running the following make command;

```sh
make brle
```

### Usage

``` sh
brle [-e|-d] [-?] [input] [output]
```

The input and output can be a path to a file or a `-`.  
When a `-` is provided as input parameter then `brle` will read its data from the standard input.
A `-` for the output lets `brle` write to the standard output.
When `input` or `output` is omitted then the standard input and/or standard output is selected.

|Option | Description|
|-------|------------|
| -e | Encode input |
| -d | Decode input |
| -? | Shows help |

The `e` option is default when no `e` or `d` option is provided.
When both of `e` or `d` are provided then the last option from the commanline is used.

Compress an input file and write the result to an output file.

```sh
brle -e file1 file2
brle file1 file2
```

The extract input file en write data to output file.

```sh
blre -d file1 file2
```

Use the output from another command as input, in this example 'cat'.

```sh
cat file1 | blre -e - file2
```

## Documentation

### API

This library has two functions that live in the `pg::brle` namespace; `encode` and `decode`.  
Like the algorithms in STL library these functions use iterators to read and write values.
Iterators give you the freedom to use raw pointers, iterators from standard containers or your own fancy iterator.

#### `pg::brle::brle8`

`pg::brle::brle8` is the data type containing the encoded RLE data.

More about the format of this type is described in the [block format](#Block-format) paragraph.

#### `output_iterator pg::brle::encode( input_iterator in, input_iterator last, output_iterator out )`

Reads data from `in` until the iterator is equal to last. The encoded RLE values are written to `out`.
The function returns an `output_iterator` that points to one past the last written RLE value.

The data type returned by the `input_iterator` can be of any size but must be an unsigned type, e.g. `unsigned char`, `uint32_t`, etc.
The `output_iterator` must have a underlaying data of the `pg::brle::brle8` type but this is not enforced by the library.
So be sure about the `output_iterator` type.

`Out` must accommodate at least 114.3% the size of the input data.
This space is required in case the function emits only [literal](#Literal) blocks.

#### `output_iterator pg::brle::decode( input_iterator in, input_iterator last, output_iterator out )`

Reads RLE values from `in` until the iterator is equal to last. The decoded data are written to `out`.
The function returns an output_iterator that points to one past the last written decoded value.

The `input_iterator` must return values of the `pg::brle::brle8` type.
The underlaying value type of the `output_iterator` can be of any unsigned types.

Be sure that `out` can buffer all the data that is encoded by the input RLE values.

There is a special case for output iterators such as returned by `std::back_inserter`.
The iterator traits for these kind of iterators do not expose the value type of the underlaying data structure.
In this case you need to specify all the template parameters as shown in the [Decoding using output iterators](#Decoding-using-output-iterators) example. 

#### Endianess

The functions are written with an little endian architecture in mind.  
Big endian is only supported when the input and output data are byte-sized types (for which the byte ordering is irrelevant).

### Block format

There are 3 block types; literal, zeros and ones.
These block types are all one byte (8 bits).
A block does not depend on other blocks.

#### Literal

|  bit  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-------|---|---|---|---|---|---|---|---|
| value | 0 | x | x | x | x | x | x | x |

The literal blocks are marked by a `0` for most significant bit.
The remaining 7 bits packs the uncompressed literal data.

A literal block is used when are less than 8 successive bits of the same value (zero or one).
Using literal blocks for these cases reduces the overhead by avoiding many blocks for very short sequences of ones and zeros.
However there is still an inevitable overhead added for literal data.
This results into more output data which is `8 / 7 = 114.3%` the size of the original data.

#### Zeros

|  bit  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-------|---|---|---|---|---|---|---|---|
| value | 1 | 0 | n | n | n | n | n | n |

A zeros block is marked by the value `10` for the two most significant bits.
The remaing bits is zeros' block value.
By adding 8 to that value you get the length of the zeros sequence represented by this block.

E.g. a zeros block with a binary representation of `1000 1001` has a decimal value of 9.
The length of the zeros sequence that is represented by this block is `8 + 9 = 17`.

The maximum number of zeros that can be represented is `8 + (2 ^ 6 ) - 1 = 71`.
This means that the maximum compression ratio that can be achieved is `8 / 71 = 11.25%`.

When a block represents _less_ then 71 zeros then sequence of zeros is always followed by an `1`.
This bit is stuffed by the encoder to improve the compression ratio, especially for sequences of zeros are followed by a single `1`.

#### Ones

|  bit  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-------|---|---|---|---|---|---|---|---|
| value | 1 | 1 | n | n | n | n | n | n |

A ones block is marked by the value `11` for the two most significant bits.
The remaing bits is ones' block value.

The description of an ones block is very much the same of a zeros block but then for sequences of ones.
When an ones block represents _less_ then 71 ones then the sequence of ones is always followed by a `0`.
