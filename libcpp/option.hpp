#pragma once

#include <exception>

/*
    Implementation of Rust's and C++20's `Option` type in C++98.
 */

template<typename T>
class Option
{
public:
    Option() : m_some(false)
    {
    }

    Option(T value) : m_value(value), m_some(true)
    {
    }

    T unwrap()
    {
        if (is_none()) throw new std::exception;
        return m_value;
    }

    T unwrap_or(T other)
    {
        if (is_none()) return other;
        return m_value;
    }

    T unwrap_or_default()
    {
        if (is_none()) return T();
        return m_value;
    }

    inline bool is_some() const
    {
        return m_some;
    }

    inline bool is_none() const
    {
        return !m_some;
    }

private:
    T m_value;
    bool m_some;
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
