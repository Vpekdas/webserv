#pragma once

#include <exception>
#include "maybe_uninit.hpp"

/*
    Implementation of Rust's and C++20's `Option` type in C++98.
 */

template<typename T>
class Option
{
public:
    Option()
    {
    }

    Option(T value) : m_value(MaybeUninit<T>(value))
    {
    }

    T unwrap()
    {
        if (!m_value.is_init()) throw new std::exception;
        T value = _get();
        return value;
    }

    T unwrap_or(T other)
    {
        if (is_none()) return other;
        T value = _get();
        return value;
    }

    T unwrap_or_default()
    {
        if (is_none()) return T();
        T value = _get();
        return value;
    }

    inline bool is_some() const
    {
        return m_value.is_init();
    }

    inline bool is_none() const
    {
        return !is_some();
    }

private:
    MaybeUninit<T> m_value;

    T _get()
    {
        return m_value.get();
    }
};

template<typename T>
Option<T> None()
{
    return Option<T>();
}

template<typename T>
Option<T> Some(T value)
{
    return Option<T>(value);
}
