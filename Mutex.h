/**
 * \file threading.cpp
 * \author Pompei2
 * \date unknown (very old)
 * \brief This file declares everything that has to do with threading.
 **/

#ifndef D_THREADING_H
#define D_THREADING_H
#include <mutex>

namespace FTS {

class Lock;

class Mutex {
    friend class Lock;
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
