# Neuron Core

## Sparse Storage

A special data structure designed for O(1) read and write operations over an indexed space which is not fully allocated.

The class signature looks like this:
```cpp
template<typename T, typename Index, Index BlockSize, signed Index BlockCount = -1>
class sparse_storage;
```

`T` must be copyable, movable, and strictly sized.
`Index` must be an unsigned integral type.

`BlockSize` must not be 0. If `BlockCount` is -1, then the storage is assumed to have a dynamic length (the array will be grown to fit as needed). Using a dynamic count may increase the number of possible allocations made, but will decrease the initial memory usage. If `BlockCount` is not -1, then `BlockSize * BlockCount` **must** be less than `std::numeric_limits<Index>::max()`.

The system works by splitting the data into blocks which are not allocated until the first write to an index in a block.

For efficient use, block sizes should be powers of two (so that we can just use bitwise operators to extract the two indices).

For efficient cache reads, you can use batch read and batch write functions to access data. These return one or more `std::span` objects (splitting of the spans is due to the non-contiguous nature of the memory).

If a known number of blocks will be required, you can use the `sparse_storage::preallocate_blocks(Index start, Index end)` function, which may allocate several blocks in a single allocation.

When doing a `read_batch_cutoff`, the cutoff will only occur if the memory is not contigous (don't assume that 

There are several forms of these batch functions, looking like follows (there are also const forms which return `std::span<const T>` instead):

```cpp

// The least efficient (at compile time) option: This cannot determine the true number of blocks that may be received and must return a vector of spans instead.
std::vector<std::span<T>> sparse_storage<T, Index, ...>::read_batch(Index start, Index end);

// This form maintains efficiency by only reading up to the end of the block containing `start`. 
std::span<T> sparse_storage<T, Index, ...>::read_batch_cutoff(Index start, Index end);
```

You can also merge blocks using the `sparse_storage::defragment()` call, which will attempt to reallocate neighboring blocks into a single allocation (and copy data over).

`sparse_storage::set_range(Index start, Index end, T value)` can be used to set a value across a range. This only works if T is copyable.
`sparse_storage::set_range(Index start, const std::span<T>& data)` can be used to set many values across a range. This only works if T is copyable.

Unfortunately, doing moves in batches is messy and not currently supported.

> Side note about using this for fast, safe accesses across threads (as I plan to do with my asset system):
> It may make more sense to make the least significant bits into my thread id markers, since that would allow me to interleave assets.
> This would cause worse data locality for assets loaded on a single thread, however the system should balance loads across the thread pool, so this interleaving may keep the safety and improve the likelihood of a cache hit when accessing them later (which can be vital to performance).
