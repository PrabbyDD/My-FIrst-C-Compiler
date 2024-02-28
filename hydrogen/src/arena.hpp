// File for arena allocator

#pragma once

class ArenaAllocator {

public:
    inline explicit ArenaAllocator(size_t bytes)
        : m_size(bytes)
    {
        m_buffer = (std::byte*) (malloc(m_size));
        // Offset at start of buffer
        m_offset = m_buffer;
    }

    // Use a template so we can know type of object we are allocating, instead of doing sizeof()
    template<typename T>
    inline T* alloc() {
        void* offset = m_offset;
        m_offset += sizeof(T);
        return (T*) offset;
    }

    inline ~ArenaAllocator()
    {
        free(m_buffer);
    }
private:
    size_t m_size;
    // Instead of typeless void*, make a byte buffer
    std::byte* m_buffer;
    std::byte* m_offset;

};


