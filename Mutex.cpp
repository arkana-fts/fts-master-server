/**
 * \file Mutex.cpp
 * \author Pompei2
 * \date unknown (very old)
 * \brief This file implements everything that has to do with locking.
 **/

#include "Mutex.h"
#include <mutex>

namespace FTSSrv2 {

Mutex::Mutex()
{}

Mutex::~Mutex()
{}

void Mutex::lock()
{
   m_mtx.lock();
}

void Mutex::unlock()
{
   m_mtx.unlock();
}

Lock::Lock( Mutex& in_m )
   : m( in_m )
{
   m.lock();
}

Lock::~Lock()
{
   m.unlock();
}

}
 /* EOF */
