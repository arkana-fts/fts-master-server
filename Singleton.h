#ifndef D_SINGLETON_H
#define D_SINGLETON_H

#include <memory>
#include <exception>

#ifdef __GNUC__
#  define SINGLETON_NAME __PRETTY_FUNCTION__
#elif defined(__STDC_VERSION__) && __STDC_VERSION__  >= 19901L
#  define SINGLETON_NAME __func__
#else
#  define SINGLETON_NAME __FUNCTION__
#endif

struct already_exist : std::exception
{
    const char* what() const noexcept { return "Singleton already exist!\n"; }
};
struct doesnot_exist : std::exception
{
    const char* what() const noexcept { return "Singleton doesn't exist!\n"; }
};

namespace FTS {

/** Really nice singleton base-class originally written by Andrew Dai,
 *  Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *  GNU GPLv2 licensed.
 *
 */
template <typename T>
class Singleton
{
public:
    Singleton( const Singleton& ) = delete ;
    const Singleton& operator=( const Singleton& ) = delete;

    virtual ~Singleton()
    {
        m_pSingleton = nullptr;
    }

    static T& getSingleton()
    {
        if(m_pSingleton == nullptr)
            throw doesnot_exist();

        return *m_pSingleton;
    }

    static T* getSingletonPtr()
    {
        return m_pSingleton;
    }

protected:
    Singleton(T* in_p)
    {
        if(m_pSingleton)
            throw already_exist();

        m_pSingleton = in_p;
    }

    // Use this constructor only when the derived class is only deriving from Singleton
    Singleton()
    {
        if(m_pSingleton)
            throw already_exist();

        m_pSingleton = (T*) (this);
    }

private:
    /// The singleton object itself.
    static T* m_pSingleton;
};

template <typename T>
T* Singleton<T>::m_pSingleton = nullptr;


}; // namespace FTS

#endif // D_SINGLETON_H
