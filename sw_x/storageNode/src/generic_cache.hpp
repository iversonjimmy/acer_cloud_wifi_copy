/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

/// Generic cache class
///
/// This generic templated class will cache data according to key and value
/// objects for a maximum duration and number of value objects.
///
/// It is intended to cache data sourced from another webservice where the
/// calls to that webservice may be expensive in terms of time.
///
/// The class takes two types for its template: K and V.
///
/// K is the key for data kept in the class. 
/// This class must have a comparator usable in a std::map.
///
/// V is the value. Any class or type is allowable, but V object may be
/// subject to copying or cloning and should not have dynamic data such as
/// mutexes, threads, file descriptiors, etc.

#ifndef __GENERIC_CACHE_HPP__
#define __GENERIC_CACHE_HPP__

#include "vpl_time.h"

#include "vpl_th.h"

#include <map>
#include <deque>
#include <utility>

template <typename K, typename V>
class genericCache
{
private:
    
    /// Data cache. 
    std::map<K, std::pair<V, VPLTime_t> > cacheData;
    
    /// Oldest-to-newest ranking. Head of queue is oldest in cache.
    std::deque<K> rankQ;
    
    /// Time limit for validity. VPLTIME_INVALID means no expiration.
    VPLTime_t expireAfter;
    
    /// Space limit for entire cache. 0 means no caching. Default:100
    size_t cacheLimit;
    
    VPLMutex_t mutex;
    
    void removeDataUnlocked(K key);

    void removeExpiredDataUnlocked();

public:
    genericCache(VPLTime_t expireAfter = VPLTIME_INVALID,
                 size_t cacheLimit = 100) :
        expireAfter(expireAfter),
        cacheLimit(cacheLimit)
    {
        VPL_SET_UNINITIALIZED(&mutex);
        VPLMutex_Init(&mutex);
    };

    ~genericCache()
    {
        VPLMutex_Destroy(&mutex);
    };

    /// Put data in the cache. 
    /// Any existing data for same key is replaced.
    /// If cache is full, oldest element is purged to make room.
    void putData(K key, V value);

    /// Get data from the cache.
    /// If data returned from cache, V will be a copy of the cached data
    /// and return value will be true.
    /// If no data present, V is unchanged and return value is false.
    /// Cached data that is expired will not be returned.
    bool getData(K key, V& value);

    // Remove specific data from the cache.
    void removeData(K key);

    // Remove all expired data from the cache.
    void removeExpiredData();

    void setExpireAfter(VPLTime_t expireAfter)
    {
        this->expireAfter = expireAfter;
    }

    void setCacheLimit(size_t limit) {
        cacheLimit = limit;
    }
};
    
template <typename K, typename V>
void genericCache<K, V>::removeDataUnlocked(K key)
{
    if(cacheData.find(key) != cacheData.end()) {
        typename std::deque<K>::iterator it;
        for(it = rankQ.begin(); it != rankQ.end(); it++) {
            if((*it) == key) {
                break;
            }
        }
        if(it != rankQ.end()) {
            rankQ.erase(it);
        }
        cacheData.erase(key);            
    }
}

template <typename K, typename V>
void genericCache<K, V>::removeExpiredDataUnlocked()
{
    if(expireAfter != VPLTIME_INVALID) {
        typename std::deque<K>::iterator it;
        while((it = rankQ.begin()) != rankQ.end()) {
            if(cacheData[(*it)].second + expireAfter 
               >= VPLTime_GetTimeStamp()) {
                break;
            }
            else {
                cacheData.erase((*it));            
                rankQ.pop_front();
            }
        }
    }
}

template <typename K, typename V>
void genericCache<K, V>::removeData(K key)
{
    VPLMutex_Lock(&mutex);
    
    removeDataUnlocked(key);

    VPLMutex_Unlock(&mutex);
}


template <typename K, typename V>
void genericCache<K, V>::removeExpiredData()
{
    VPLMutex_Lock(&mutex);
    
    removeExpiredDataUnlocked();

    VPLMutex_Unlock(&mutex);
}

template <typename K, typename V>
void genericCache<K, V>::putData(K key, V value)
{
    VPLMutex_Lock(&mutex);
    
    // Remove any existing data for same key. Will re-add.
    removeDataUnlocked(key);
    
    // If cache would be over-full after add, remove oldest element.
    if(cacheData.size() + 1 > cacheLimit) {
        K& oldKey = rankQ.front();
        cacheData.erase(oldKey);
        rankQ.pop_front();
    }
    
    // Add value to cache.
    cacheData[key] = std::make_pair(value, VPLTime_GetTimeStamp());
    rankQ.push_back(key);
    
    VPLMutex_Unlock(&mutex);
}

template <typename K, typename V>
bool genericCache<K, V>::getData(K key, V& value)
{
    bool rv = false;
    typename std::map<K, std::pair<V, VPLTime_t> >::iterator it;
    
    VPLMutex_Lock(&mutex);
    
    // Find cache entry.
    it = cacheData.find(key);
    if(it != cacheData.end()) {
        // Ignore if expire time set and entry expired.
        if(expireAfter == VPLTIME_INVALID ||
           it->second.second + expireAfter >= VPLTime_GetTimeStamp()) {
            value = it->second.first;
            rv = true;
        }
        else {
            // Remove expired data.
            removeExpiredDataUnlocked();
        }
    }
    
    VPLMutex_Unlock(&mutex);
    
    return rv;
}


#endif // include guard
