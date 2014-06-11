#ifndef __HTTPSVC_ATOMICCOUNTER_HPP__
#define __HTTPSVC_ATOMICCOUNTER_HPP__

#include <vplu_types.h>

#include <vpl_th.h>

namespace HttpSvc {
    class AtomicCounter {
    public:
        AtomicCounter();
        ~AtomicCounter();
        int operator++();  // prefix
        int operator--();  // prefix
        operator int();
    private:
        VPLMutex_t mutex;
        int counter;
    };
}

#endif // incl guard
