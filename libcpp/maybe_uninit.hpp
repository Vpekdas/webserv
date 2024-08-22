#pragma once

#include <cstddef>
#include <exception>

/*
    Inspired by Rust's `MaybeUninit`.
    This class can store any type, even references like `int&` and initialize it at later time, or never.
    It will throw an exception when accessing the inner value if it is not initialized.
 */
template<typename T>
class MaybeUninit
{
public:
    MaybeUninit() : m_init(false)
    {
    }

    MaybeUninit(T value) : m_init(true)
    {
        set(value);
    }

    ~MaybeUninit()
    {
        // Since we store the value as a char[], the value needs to be destroyed manually.
        // Of course only if the value is initialized.
        if (is_init())
        {
            T value = get();
            (void) value;
        }
    }

    T get()
    {
        if (!m_init) throw std::exception();
        return _get_unchecked();
    }

    void set(T value)
    {
        for (size_t i = 0; i < sizeof(T); i++)
            m_value[i] = ((T *)&value)[i];
        m_init = true;
    }

    inline bool is_init() const
    {
        return m_init;
    }

private:
    char m_value[sizeof(T)];
    bool m_init;

    inline T _get_unchecked()
    {
        return *(T *)(&m_value[0]);
    }
};
