/**
 * \file Mutex.h
 * \author Pompei2
 * \date unknown (very old)
 * \brief This file declares everything that has to do with locking.
 **/

#ifndef D_THREADING_H
#define D_THREADING_H
#include <mutex>

namespace FTSSrv2 {

class Mutex {
    std::recursive_mutex m_mtx;

public:
    Mutex() = default;
    ~Mutex() = default;

    void lock() { m_mtx.lock(); }
    void unlock() { m_mtx.unlock(); }
};

class Lock {
    Mutex& m;
public:
    explicit Lock(Mutex& m) : m( m ) { m.lock(); }
    ~Lock() { m.unlock(); }
};

} // namespace FTS

#endif /* D_THREADING_H */

 /* EOF */
