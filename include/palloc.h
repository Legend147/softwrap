/**
 *  Copyright (c) 2015, Ellis Giles, Peter Varman, and Kshitij Doshi
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification,
 *  are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors
 *     may be used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include<memory>
#include<cstdlib>
#include<iostream>
#include<limits>

extern void persistentNotifyPin(void *v, size_t size);

namespace pstd {
 
template<typename T>
class PersistentAllocator{
public :
    //    typedefs

    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

public :
    //    convert an allocator<T> to allocator<U>

    template<typename U>
    struct rebind {
        typedef PersistentAllocator<U> other;
    };

public :
    inline explicit PersistentAllocator() {}
    inline ~PersistentAllocator() {}
    inline explicit PersistentAllocator(PersistentAllocator const&) {}
    template<typename U>
    inline PersistentAllocator(PersistentAllocator<U> const&) {}

    //    address

    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    //    memory allocation

    inline pointer allocate(size_type cnt,
       typename std::allocator<void>::const_pointer = 0)
    {
      //std::cout<<"Trying to allocate "<<cnt<<" objects in memory"<<std::endl;
    	int totalSize = cnt * sizeof(T);
      pointer new_memory = reinterpret_cast<pointer>(pmalloc(totalSize));
//::operator new(totalSize));
      //std::cout<<"Allocated "<<cnt<<" objects in memory at location:"<<new_memory<<std::endl;
      //std::cout << "A " << new_memory << " " << (cnt * sizeof(T)) << " " << cnt << " " << sizeof(T) << std::endl;
      //std::cout << new_memory << " " << totalSize << std::endl;
      //persistentNotifyPin(new_memory, totalSize);
      return new_memory;
    }
    inline void deallocate(pointer p, size_type n)
    {
        //::operator delete(p);
       // std::cout<<"Deleted "<<n<<" objects from memory"<<std::endl;
    }
    //    size
    inline size_type max_size() const {
        return std::numeric_limits<size_type>::max() / sizeof(T);
 }

    //    construction/destruction

    inline void construct(pointer p, const T& t) {
      std::cout<<"Constructing at memory location:" <<p<<std::endl;
      //std::cout << "C " << p << " " << sizeof(T) << std::endl;
      new(p) T(t);
    }
    inline void destroy(pointer p) {
      //std::cout<<"Destroying object at memory location:" <<p<<std::endl;
      //p->~T();
    }

    inline bool operator==(PersistentAllocator const&) { return true; }
    inline bool operator!=(PersistentAllocator const& a) { return !operator==(a); }
};    //    end of class PersistentAllocator
} // end of namespace pstd
