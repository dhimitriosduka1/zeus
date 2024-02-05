/**
 * @file iterators.hpp
 * @brief Convenience classes to iterate over 1-D and 2-D ranges within for loops and similar constructs.
 */

#pragma once

#include <lightwave/core.hpp>

namespace lightwave {

/// @brief Iterates over an interval of integers.
class Range {
public:
    struct iterator {
        int operator*() const { return m_index; }
        bool operator!=(const iterator &other) const { return m_index != other.m_index; }
        iterator &operator++() {
            m_index++;
            return *this;
        }
    
    private:
        friend class Range;
        iterator(int index) : m_index(index) {}
        int m_index;
    };

    /// @brief Constructs a range, starting with element @c start and ending with element @code end - 1 @endcode .
    Range(int start, int end)
    : m_start(start), m_end(end) {}
 
    iterator begin() const { return iterator(m_start); } 
    iterator end() const { return iterator(m_end); }

    /// @brief The number of elements in the range. 
    int count() const { return m_end - m_start; }

private:
    int m_start, m_end;
};

/**
 * @brief Iterates over an interval of integers in chunks.
 * This is particularly useful when distributing work to different threads, where using an ordinary
 * (unchunked) Range iterator can lead to excessive overhead when there is little work to do with each range element.
 */
class ChunkedRange {
public:
    struct iterator {
        Range operator*() const { return { m_start, m_end }; }
        bool operator!=(const iterator &other) const { return m_start != other.m_start; }
        iterator &operator++() {
            const int size = m_end - m_start;
            m_start += size;
            m_end = std::min(m_rangeEnd, m_end + size);
            return *this;
        }
    
    private:
        friend class ChunkedRange;
        iterator(int start, int end, int rangeEnd) : m_start(start), m_end(end), m_rangeEnd(rangeEnd) {}
        int m_start, m_end;
        int m_rangeEnd;
    };

    /// @brief Constructs a range [start, end), which is iterated in chunks of size @c blockSize .
    ChunkedRange(int start, int end, int blockSize)
    : m_start(start), m_end(end), m_blockSize(blockSize) {}

    /// @brief Constructs a range [0, count), which is iterated in chunks of size @c blockSize .
    ChunkedRange(int count, int blockSize)
    : ChunkedRange(0, count, blockSize) {}

    iterator begin() const { return iterator(m_start, m_start + m_blockSize, m_end); }
    iterator end() const { return iterator(m_end, m_end, m_end); }

private:
    int m_start, m_end, m_blockSize;
};

/**
 * @brief Uses a spiral pattern to iterate over a two-dimensional integer interval in blocks.
 * Using blocks allows each thread to have a meaningful amount of (coherent) work, avoiding overhead and being cache friendlier.
 * The spiral pattern forces the center of the image to be rendered first, which is typically most interesting region of the image,
 * and hence gives early feedback to the user whether the render looks correct.
 */
class BlockSpiral {
    Vector2i m_imageSize;
    Vector2i m_blockSize;

public:
    struct iterator {
        Bounds2i operator*() const {
            const Vector2i min = (m_queue.imageSize() - m_queue.blockSize()) / 2 + m_queue.blockSize() * curve(m_rank, m_index);
            const Vector2i max = min + m_queue.blockSize();
            return Bounds2i(Vector2i(0), m_queue.imageSize()).clip(Bounds2i(min, max));
        }

        bool operator!=(const iterator &other) const { return m_rank != other.m_rank || m_index != other.m_index; }

        iterator &operator++() {
            if (m_rank == -1) return *this;

            bool changedRank = false;
            do {
                if (++m_index >= 8 * m_rank) {
                    if (changedRank) {
                        m_index = 0;
                        m_rank = -1;
                        break;
                    }

                    m_index = 0;
                    m_rank++;
                    changedRank = true;
                }

                auto bounds = *(*this);
                if (!bounds.isEmpty()) break;
            } while (true);

            return *this;
        }

    private:
        friend class BlockSpiral;
        
        iterator(const BlockSpiral &queue, int rank, int index)
        : m_queue(queue), m_rank(rank), m_index(index) {}

        const BlockSpiral &m_queue;
        int m_rank;
        int m_index;

        static Vector2i curve(int rank, int index) {
            if (rank == 0) return Vector2i(0);
            index = (index + 1) % (8 * rank);
            const int quadrant = index / (2 * rank);
            const int shift = index % (2 * rank);
            switch (quadrant) {
            case 0: return { shift - rank, -rank };
            case 1: return { +rank, shift - rank };
            case 2: return { rank - shift, +rank };
            case 3: return { -rank, rank - shift };
            default: return {};
            }
        }
    };

    BlockSpiral(const Vector2i &imageSize, const Vector2i &blockSize)
    : m_imageSize(imageSize), m_blockSize(blockSize) {}

    const Vector2i &imageSize() const { return m_imageSize; }
    const Vector2i &blockSize() const { return m_blockSize; }

    iterator begin() const { return { *this, 0, 0 }; }
    iterator end() const { return { *this, -1, 0 }; }
};

}
