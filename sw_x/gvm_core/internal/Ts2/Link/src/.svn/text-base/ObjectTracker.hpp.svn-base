#ifndef __TS2_LINK_OBJECT_TRACKER_HPP__
#define __TS2_LINK_OBJECT_TRACKER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <vpl_th.h>
#include <vplu_mutex_autolock.hpp>

#include <map>

namespace Ts2 {
namespace Link {

template <class T>
class ObjectTracker {
public:
    ObjectTracker() : counter(0) {
        VPLMutex_Init(&mutex);
    }
    ~ObjectTracker() {
        VPLMutex_Destroy(&mutex);
    }
    u32 Add(T *p) {
        MutexAutoLock lock(&mutex);
        counter++;
        objs[counter] = p;
        return counter;
    }
    T *Lookup(u32 index) {
        MutexAutoLock lock(&mutex);
        return (objs.find(index) != objs.end()) ? objs[index] : NULL;
    }
    void Remove(u32 index) {
        MutexAutoLock lock(&mutex);
        objs.erase(index);
    }
private:
    mutable VPLMutex_t mutex;
    u32 counter;
    std::map<u32, T*> objs;
};

} // end namespace Link
} // end namespace Ts2

#endif // incl guard
