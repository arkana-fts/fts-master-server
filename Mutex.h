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
    Mutex();
    virtual ~Mutex();

    void lock();
    void unlock();

};

class Lock {
protected:
    Mutex& m;
public:
    Lock(Mutex& m);
    ~Lock();
};

} // namespace FTS

#endif /* D_THREADING_H */

 /* EOF */
