//
//  Copyright 2012 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __VPLEX_UTIL_HPP_
#define __VPLEX_UTIL_HPP_

#include <algorithm>
#include <cctype>
#include <string>

#include <list>
#include <set>
#include <utility>


struct case_insensitive_less {
    struct case_insensitive_compare {
        bool operator() (const unsigned char& c1, const unsigned char& c2) const {
            return tolower(c1) < tolower(c2);
        }
    };

    bool operator() (const std::string & s1, const std::string & s2) const {
        return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), case_insensitive_compare());
    }
};

template < typename KeyType, typename MappedType, typename Comp = std::less< KeyType > >
struct VPLUtil_TimeOrderedMap {

    typedef KeyType key_type;
    typedef MappedType mapped_type;
    typedef std::pair< const key_type, mapped_type > value_type;

private:

    typedef std::list< value_type >      list_type;
    typedef typename list_type::iterator list_iterator;
    typedef typename list_type::const_iterator list_const_iterator;

    struct compare_keys {
        Comp the_order;

        compare_keys ( Comp o )
            : the_order ( o )
        {}

        bool operator() ( list_iterator lhs, list_iterator rhs ) const {
            return ( the_order( lhs->first, rhs->first ) );
        }
    };

    typedef std::set< list_iterator, compare_keys > set_type;
    typedef typename set_type::iterator             set_iterator;
    typedef typename set_type::const_iterator       set_const_iterator;

    list_type the_list;
    set_type  the_set;

public:

    typedef list_iterator iterator;
    typedef list_const_iterator const_iterator;
    typedef typename set_type::size_type size_type;

    VPLUtil_TimeOrderedMap ( Comp o = Comp() )
        : the_list(), the_set ( compare_keys( o ) )
    {}

    iterator find ( key_type const & key ) {
        value_type dummy_value ( key, mapped_type() );
        list_type  dummy_list;
        dummy_list.push_back( dummy_value );
        set_iterator where = the_set.find( dummy_list.begin() );
        if ( where == the_set.end() ) {
            return ( the_list.end() );
        }
        return ( *where );
    }

    const_iterator find ( key_type const & key ) const {
        value_type dummy_value ( key, mapped_type() );
        list_type  dummy_list;
        dummy_list.push_back( dummy_value );
        set_const_iterator where = the_set.find( dummy_list.begin() );
        if ( where == the_set.end() ) {
            return ( the_list.end() );
        }
        return ( *where );
    }

    iterator insert ( value_type const & value ) {
        list_type dummy;
        dummy.push_back( value );
        set_iterator where = the_set.find( dummy.begin() );
        if ( where == the_set.end() ) {
            the_list.push_back( value );
            list_iterator pos = the_list.end();
            -- pos;
            the_set.insert( pos );
            return ( pos );
        } else {
            (*where)->second = value.second;
            return ( *where );
        }
    }

    iterator erase ( iterator where ) {
        the_set.erase( where );
        return ( the_list.erase( where ) );
    }

    iterator begin ( void ) {
        return ( the_list.begin() );
    }

    iterator end ( void ) {
        return ( the_list.end() );
    }

    const_iterator end ( void ) const {
        return ( the_list.end() );
    }

    size_type size ( void ) const {
        return ( the_set.size() );
    }

    mapped_type & operator[] ( key_type const & key ) {
        iterator pos = insert( std::make_pair( key, mapped_type() ) );
        return ( pos->second );
    }

    void clear ( void ) {
        the_set.clear();
        the_list.clear();
    }
};



#endif //ndef __VPLEX_UTIL_HPP_
