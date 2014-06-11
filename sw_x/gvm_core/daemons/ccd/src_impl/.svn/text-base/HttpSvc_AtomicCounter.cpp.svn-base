#include "HttpSvc_AtomicCounter.hpp"

HttpSvc::AtomicCounter::AtomicCounter()
    : counter(0)
{
    VPLMutex_Init(&mutex);
}

HttpSvc::AtomicCounter::~AtomicCounter()
{
    VPLMutex_Destroy(&mutex);
}

int HttpSvc::AtomicCounter::operator++()
{
    VPLMutex_Lock(&mutex);
    int newvalue = ++counter;
    VPLMutex_Unlock(&mutex);
    return newvalue;
}

int HttpSvc::AtomicCounter::operator--()
{
    VPLMutex_Lock(&mutex);
    int newvalue = --counter;
    VPLMutex_Unlock(&mutex);
    return newvalue;
}

HttpSvc::AtomicCounter::operator int()
{
    VPLMutex_Lock(&mutex);
    int curvalue = counter;
    VPLMutex_Unlock(&mutex);
    return curvalue;
}

