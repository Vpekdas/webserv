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

    T get()
    {
        if (!m_init) throw std::exception();
        return *(T *)(&m_value[0]);
    }

    void set(T value)
    {
        for (size_t i = 0; i < sizeof(T); i++)
            m_value[i] = ((T *)&value)[i];
        m_init = true;
    }

    bool is_init() const
    {
        return m_init;
    }

private:
    char m_value[sizeof(T)];
    bool m_init;
};
