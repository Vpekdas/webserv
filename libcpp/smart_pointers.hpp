#pragma once

/*
    Reimplementation of C++11 smart points for C++98.
 */

#include <iostream>
#include <stddef.h>
#include <stdint.h>

template <typename T>
class SharedPtr
{
public:
    SharedPtr()
    {
        m_ptr = NULL;
        m_strong_refs = NULL;
    }

    SharedPtr(T *ptr)
    {
        if (!ptr)
        {
            m_ptr = NULL;
            m_strong_refs = NULL;
            return;
        }

        m_ptr = ptr;
        m_strong_refs = new uint32_t(1);
    }

    SharedPtr(SharedPtr const& other)
    {
        _assign(other);
    }

    SharedPtr& operator=(SharedPtr const& other)
    {
        _assign(other);
        return *this;
    }

    bool operator==(SharedPtr& other)
    {
        return m_ptr == other.m_ptr;
    }

    T& get()
    {
        // TODO: What do we do if the pointer NULL? An exception maybe
        if (!m_ptr)
            std::cout << "m_ptr = " << m_ptr << "\n";
        return *m_ptr;
    }

    T& operator*()
    {
        return get();
    }

    T *operator->()
    {
        return m_ptr;
    }

    T *ptr()
    {
        return m_ptr;
    }

private:
    T *m_ptr;
    uint32_t *m_strong_refs;

    void _ref()
    {
        if (!m_strong_refs)
            return;
        *m_strong_refs += 1;
    }

    void _deref()
    {
        if (!m_strong_refs)
            return;
        *m_strong_refs -= 1;
        if (*m_strong_refs == 0)
        {
            delete m_ptr;
            delete m_strong_refs;
            m_ptr = NULL;
            m_strong_refs = NULL;
        }
    }

    void _assign(SharedPtr const& other)
    {
        if (other.m_ptr == m_ptr)
            return;
        _deref();
        m_ptr = other.m_ptr;
        m_strong_refs = other.m_strong_refs;
        _ref();
    }
};

template <typename T>
SharedPtr<T> make_shared(T *ptr)
{
    return SharedPtr<T>(ptr);
}
