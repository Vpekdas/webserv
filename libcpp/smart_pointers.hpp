#pragma once

/*
    Reimplementation of C++11 smart points for C++98.
 */

#include <cstddef>
#include <cstdint>

template<typename T>
class SharedPtr
{
public:
    SharedPtr()
    {
        m_ptr = NULL;
        m_strong_refs = NULL;
    }

    SharedPtr(T * ptr)
    {
        m_ptr = ptr;
        m_strong_refs = new uint32_t(1);
    }

    SharedPtr(SharedPtr& other)
    {
        _assign(other);
    }

    void operator=(SharedPtr& other)
    {
        _assign(other);
    }

    bool operator==(SharedPtr& other)
    {
        return m_ptr == other.m_ptr;
    }

    T& get()
    {
        // TODO: What do we do if the pointer NULL? An exception maybe
        return *m_ptr;
    }

    T& operator*()
    {
        return get();
    }

private:
    T *m_ptr;
    uint32_t *m_strong_refs;

    void _ref()
    {
        if (!m_strong_refs)
            return ;
        *m_strong_refs += 1;
    }

    void _deref()
    {
        if (!m_strong_refs)
            return ;
        m_strong_refs -= 1;
        if (*m_strong_refs == 0)
        {
            delete m_ptr;
            delete m_strong_refs;
        }
    }

    void _assign(SharedPtr& other)
    {
        if (other.m_ptr == m_ptr)
            return ;
        _deref();
        m_ptr = other.m_ptr;
        m_strong_refs = other.m_strong_refs;
        _ref();
    }
};
