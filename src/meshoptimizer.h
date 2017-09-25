/**
 * meshoptimizer - version 0.6
 *
 * Copyright (C) 2016-2017, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https: *github.com/zeux/meshoptimizer
 *
 * This library is distributed under the MIT License. See notice at the end of this file.
 */
#pragma once

#include <stddef.h>

/* Version macro; major * 100 + minor * 10 + patch */
#define MESHOPTIMIZER_VERSION 60

/* If no API is defined, assume default */
#ifndef MESHOPTIMIZER_API
#define MESHOPTIMIZER_API
#endif

/* Default vertex cache size */
#define MESHOPTIMIZER_DEFAULT_VCACHE_SIZE 16

/* Index buffer codec version */
#define MESHOPTIMIZER_INDEX_CODEC_VERSION 1

/* C interface */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generates a vertex remap table from the vertex buffer and an optional index buffer and returns number of unique vertices
 *
 * destination must contain enough space for the resulting remap table (vertex_count elements)
 * indices can be NULL if the input is unindexed
 */
MESHOPTIMIZER_API size_t meshopt_generateVertexRemap(unsigned int* destination, const unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size);

/**
 * Generates vertex buffer from the source vertex buffer and remap table generated by generateVertexRemap
 *
 * destination must contain enough space for the resulting vertex buffer (unique_vertex_count elements, returned by generateVertexRemap)
 */
MESHOPTIMIZER_API void meshopt_remapVertexBuffer(void* destination, const void* vertices, size_t vertex_count, size_t vertex_size, const unsigned int* remap);

/**
 * Generate index buffer from the source index buffer and remap table generated by generateVertexRemap
 *
 * destination must contain enough space for the resulting index buffer (index_count elements)
 * indices can be NULL if the input is unindexed
 */
MESHOPTIMIZER_API void meshopt_remapIndexBuffer(unsigned int* destination, const unsigned int* indices, size_t index_count, const unsigned int* remap);

/**
 * Vertex transform cache optimizer
 * Reorders indices to reduce the number of GPU vertex shader invocations
 *
 * destination must contain enough space for the resulting index buffer (index_count elements)
 * cache_size should be less than the actual GPU cache size to avoid cache thrashing
 */
MESHOPTIMIZER_API void meshopt_optimizeVertexCache(unsigned int* destination, const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size);

/**
 * Overdraw optimizer
 * Reorders indices to reduce the number of GPU vertex shader invocations and the pixel overdraw
 *
 * destination must contain enough space for the resulting index buffer (index_count elements)
 * indices must contain index data that is the result of optimizeVertexCache (*not* the original mesh indices!)
 * vertex_positions should have float3 position in the first 12 bytes of each vertex - similar to glVertexPointer
 * cache_size should be less than the actual GPU cache size to avoid cache thrashing
 * threshold indicates how much the overdraw optimizer can degrade vertex cache efficiency (1.05 = up to 5%) to reduce overdraw more efficiently
 */
MESHOPTIMIZER_API void meshopt_optimizeOverdraw(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, unsigned int cache_size, float threshold);

/**
 * Vertex fetch cache optimizer
 * Reorders vertices and changes indices to reduce the amount of GPU memory fetches during vertex processing
 *
 * destination must contain enough space for the resulting vertex buffer (vertex_count elements)
 * indices is used both as an input and as an output index buffer
 */
MESHOPTIMIZER_API size_t meshopt_optimizeVertexFetch(void* destination, unsigned int* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size);

/**
 * Experimental: Index buffer encoder
 * Encodes index data into an array of bytes that is generally much smaller (<1.5 bytes/triangle) and compresses better (<1 bytes/triangle) compared to original.
 * For maximum efficiency the index buffer being encoded has to be optimized for vertex cache and vertex fetch first.
 *
 * buffer must contain enough space for the encoded index buffer (use meshopt_encodeIndexBufferBound to estimate)
 * version is the encoding version used, 1 <= version <= MESHOPTIMIZER_INDEX_CODEC_VERSION
 */
MESHOPTIMIZER_API size_t meshopt_encodeIndexBuffer(unsigned char* buffer, size_t buffer_size, const unsigned int* indices, size_t index_count, int version);
MESHOPTIMIZER_API size_t meshopt_encodeIndexBufferBound(size_t index_count, size_t vertex_count);

/**
 * Experimental: Index buffer decoder
 * Decodes index data from an array of bytes generated by meshopt_encodeIndexBuffer
 *
 * destination must contain enough space for the resulting index buffer (index_count elements)
 * version must match the version used during encoding
 */
MESHOPTIMIZER_API void meshopt_decodeIndexBuffer(unsigned int* destination, size_t index_count, const unsigned char* buffer, size_t buffer_size, int version);

/**
 * Experimental: Mesh simplifier
 * Reduces the number of triangles in the mesh, attempting to preserve mesh appearance as much as possible
 * Returns the number of indices after simplification, with destination containing new index data
 * 
 * destination must contain enough space for the source index buffer (since optimization is iterative, this means index_count elements - *not* target_index_count!)
 * vertex_positions should have float3 position in the first 12 bytes of each vertex - similar to glVertexPointer
 */
MESHOPTIMIZER_API size_t meshopt_simplify(unsigned int* destination, const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count);

struct meshopt_VertexCacheStatistics
{
	unsigned int vertices_transformed;
	float acmr; /* transformed vertices / triangle count; best case 0.5, worst case 3.0, optimum depends on topology */
	float atvr; /* transformed vertices / vertex count; best case 1.0, worse case 6.0, optimum is 1.0 (each vertex is transformed once) */
};

/**
 * Vertex transform cache analyzer
 * Returns cache hit statistics using a simplified FIFO model
 * Results will not match actual GPU performance
 */
MESHOPTIMIZER_API struct meshopt_VertexCacheStatistics meshopt_analyzeVertexCache(const unsigned int* indices, size_t index_count, size_t vertex_count, unsigned int cache_size);

struct meshopt_OverdrawStatistics
{
	unsigned int pixels_covered;
	unsigned int pixels_shaded;
	float overdraw; /* shaded pixels / covered pixels; best case 1.0 */
};

/**
 * Overdraw analyzer
 * Returns overdraw statistics using a software rasterizer
 * Results will not match actual GPU performance
 *
 * vertex_positions should have float3 position in the first 12 bytes of each vertex - similar to glVertexPointer
 */
MESHOPTIMIZER_API struct meshopt_OverdrawStatistics meshopt_analyzeOverdraw(const unsigned int* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride);

struct meshopt_VertexFetchStatistics
{
	unsigned int bytes_fetched;
	float overfetch; /* fetched bytes / vertex buffer size; best case 1.0 (each byte is fetched once) */
};

/**
 * Vertex fetch cache analyzer
 * Returns cache hit statistics using a simplified direct mapped model
 * Results will not match actual GPU performance
 */
MESHOPTIMIZER_API struct meshopt_VertexFetchStatistics meshopt_analyzeVertexFetch(const unsigned int* indices, size_t index_count, size_t vertex_count, size_t vertex_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Quantization into commonly supported data formats */
#ifdef __cplusplus
/**
 * Quantize a float in [0..1] range into an N-bit fixed point unorm value
 * Assumes reconstruction function (q / (2^N-1)), which is the case for fixed-function normalized fixed point conversion
 * Maximum reconstruction error: 1/2^(N+1)
 */
inline int meshopt_quantizeUnorm(float v, int N);

/**
 * Quantize a float in [-1..1] range into an N-bit fixed point snorm value
 * Assumes reconstruction function (q / (2^(N-1)-1)), which is the case for fixed-function normalized fixed point conversion (except OpenGL)
 * Maximum reconstruction error: 1/2^N
 * Warning: OpenGL fixed function reconstruction function can't represent 0 exactly; when using OpenGL, use this function and have the shader reconstruct by dividing by 2^(N-1)-1.
 */
inline int meshopt_quantizeSnorm(float v, int N);

/**
 * Quantize a float into half-precision floating point value
 * Generates +-inf for overflow, preserves NaN, flushes denormals to zero, rounds to nearest
 * Representable magnitude range: [6e-5; 65504]
 * Maximum relative reconstruction error: 5e-4
 */
inline unsigned short meshopt_quantizeHalf(float v);
#endif

/* C++ template interface */
#ifdef __cplusplus
template <typename T, bool ZeroCopy = sizeof(T) == sizeof(unsigned int)>
struct meshopt_IndexAdapter;

template <typename T>
struct meshopt_IndexAdapter<T, false>
{
	T* result;
	unsigned int* data;
	size_t count;

	meshopt_IndexAdapter(T* result, const T* input, size_t count)
	    : result(result)
	    , data(0)
	    , count(count)
	{
		data = new unsigned int[count];

		if (input)
		{
			for (size_t i = 0; i < count; ++i)
				data[i] = input[i];
		}
	}

	~meshopt_IndexAdapter()
	{
		if (result)
		{
			for (size_t i = 0; i < count; ++i)
				result[i] = data[i];
		}

		delete[] data;
	}
};

template <typename T>
struct meshopt_IndexAdapter<T, true>
{
	unsigned int* data;

	meshopt_IndexAdapter(T* result, const T* input, size_t)
	    : data(reinterpret_cast<unsigned int*>(result ? result : const_cast<T*>(input)))
	{
	}
};

template <typename T>
inline size_t meshopt_generateVertexRemap(unsigned int* destination, const T* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	meshopt_IndexAdapter<T> in(0, indices, indices ? index_count : 0);

	return meshopt_generateVertexRemap(destination, indices ? in.data : 0, index_count, vertices, vertex_count, vertex_size);
}

template <typename T>
inline void meshopt_remapIndexBuffer(T* destination, const T* indices, size_t index_count, const unsigned int* remap)
{
	meshopt_IndexAdapter<T> in(0, indices, indices ? index_count : 0);
	meshopt_IndexAdapter<T> out(destination, 0, index_count);

	meshopt_remapIndexBuffer(out.data, indices ? in.data : 0, index_count, remap);
}

template <typename T>
inline void meshopt_optimizeVertexCache(T* destination, const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);
	meshopt_IndexAdapter<T> out(destination, 0, index_count);

	meshopt_optimizeVertexCache(out.data, in.data, index_count, vertex_count, cache_size);
}

template <typename T>
inline void meshopt_optimizeOverdraw(T* destination, const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, unsigned int cache_size, float threshold)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);
	meshopt_IndexAdapter<T> out(destination, 0, index_count);

	meshopt_optimizeOverdraw(out.data, in.data, index_count, vertex_positions, vertex_count, vertex_positions_stride, cache_size, threshold);
}

template <typename T>
inline size_t meshopt_optimizeVertexFetch(void* destination, T* indices, size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size)
{
	meshopt_IndexAdapter<T> inout(indices, indices, index_count);

	return meshopt_optimizeVertexFetch(destination, inout.data, index_count, vertices, vertex_count, vertex_size);
}

template <typename T>
inline size_t meshopt_encodeIndexBuffer(unsigned char* buffer, size_t buffer_size, const T* indices, size_t index_count, int version)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);

	return meshopt_encodeIndexBuffer(buffer, buffer_size, in.data, index_count, version);
}

template <typename T>
inline void meshopt_decodeIndexBuffer(T* destination, size_t index_count, const unsigned char* buffer, size_t buffer_size, int version)
{
	meshopt_IndexAdapter<T> out(destination, 0, index_count);

	meshopt_decodeIndexBuffer(out.data, index_count, buffer, buffer_size, version);
}

template <typename T>
inline size_t meshopt_simplify(T* destination, const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride, size_t target_index_count)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);
	meshopt_IndexAdapter<T> out(destination, 0, index_count);

	return meshopt_simplify(out.data, in.data, index_count, vertex_positions, vertex_count, vertex_positions_stride, target_index_count);
}

template <typename T>
inline meshopt_VertexCacheStatistics meshopt_analyzeVertexCache(const T* indices, size_t index_count, size_t vertex_count, unsigned int cache_size)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);

	return meshopt_analyzeVertexCache(in.data, index_count, vertex_count, cache_size);
}

template <typename T>
inline meshopt_OverdrawStatistics meshopt_analyzeOverdraw(const T* indices, size_t index_count, const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);

	return meshopt_analyzeOverdraw(in.data, index_count, vertex_positions, vertex_count, vertex_positions_stride);
}

template <typename T>
inline meshopt_VertexFetchStatistics meshopt_analyzeVertexFetch(const T* indices, size_t index_count, size_t vertex_count, size_t vertex_size)
{
	meshopt_IndexAdapter<T> in(0, indices, index_count);

	return meshopt_analyzeVertexFetch(in.data, index_count, vertex_count, vertex_size);
}
#endif

/* Inline implementation */
#ifdef __cplusplus
inline int meshopt_quantizeUnorm(float v, int N)
{
	const float scale = float((1 << N) - 1);

	v = (v >= 0) ? v : 0;
	v = (v <= 1) ? v : 1;

	return int(v * scale + 0.5f);
}

inline int meshopt_quantizeSnorm(float v, int N)
{
	const float scale = float((1 << (N - 1)) - 1);

	float round = (v >= 0 ? 0.5f : -0.5f);

	v = (v >= -1) ? v : -1;
	v = (v <= +1) ? v : +1;

	return int(v * scale + round);
}

inline unsigned short meshopt_quantizeHalf(float v)
{
	union { float f; unsigned int ui; } u = {v};
	unsigned int ui = u.ui;

	int s = (ui >> 16) & 0x8000;
	int em = ui & 0x7fffffff;

	/* bias exponent and round to nearest; 112 is relative exponent bias (127-15) */
	int h = (em - (112 << 23) + (1 << 12)) >> 13;

	/* underflow: flush to zero; 113 encodes exponent -14 */
	h = (em < (113 << 23)) ? 0 : h;

	/* overflow: infinity; 143 encodes exponent 16 */
	h = (em >= (143 << 23)) ? 0x7c00 : h;

	/* NaN; note that we convert all types of NaN to qNaN */
	h = (em > (255 << 23)) ? 0x7e00 : h;

	return (unsigned short)(s | h);
}
#endif

/**
 * Copyright (c) 2016-2017 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
