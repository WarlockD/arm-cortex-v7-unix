//******************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*               
//*     PURPOSE:  User Suport Library Source
//*               
//*     Version: v5.1.0
//*
//*
//*     Copyright (c) 2003-2016, scmRTOS Team
//*
//*     Permission is hereby granted, free of charge, to any person 
//*     obtaining  a copy of this software and associated documentation 
//*     files (the "Software"), to deal in the Software without restriction, 
//*     including without limitation the rights to use, copy, modify, merge, 
//*     publish, distribute, sublicense, and/or sell copies of the Software, 
//*     and to permit persons to whom the Software is furnished to do so, 
//*     subject to the following conditions:
//*
//*     The above copyright notice and this permission notice shall be included 
//*     in all copies or substantial portions of the Software.
//*
//*     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
//*     EXPRESS  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
//*     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
//*     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
//*     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
//*     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
//*     THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//*
//*     =================================================================
//*     Project sources: https://github.com/scmrtos/scmrtos
//*     Documentation:   https://github.com/scmrtos/scmrtos/wiki/Documentation
//*     Wiki:            https://github.com/scmrtos/scmrtos/wiki
//*     Sample projects: https://github.com/scmrtos/scmrtos-sample-projects
//*     =================================================================
//*
//******************************************************************************

#include <usrlib.h>

using namespace usr;

//------------------------------------------------------------------------------
//
///   Circular buffer function-member description
//
//
//
TCbuf::TCbuf(uint8_t* const Address, const size_t Size) :
        _buf(Address),
        _size(Size),
        _count(0),
        _first(0),
        _last(0)
{
}
//------------------------------------------------------------------------------
bool TCbuf::write(const uint8_t* data, const size_t Count)
{
    if( Count > (_size - _count) )
        return false;

    for(size_t i = 0; i < Count; i++)
        push(*(data++));

    return true;
}
//------------------------------------------------------------------------------
void TCbuf::read(uint8_t* data, const size_t Count)
{
	size_t N = Count <= _count ? Count : _count;

    for(size_t i = 0; i < N; i++)
        data[i] = pop();
}
//------------------------------------------------------------------------------
uint8_t TCbuf::get_byte(const size_t index) const
{
	size_t x = _first + index;

    if(x < _size)
        return _buf[x];
    else
        return _buf[x - _size];
}

//------------------------------------------------------------------------------
bool TCbuf::put(const uint8_t item)
{
    if(_count == _size)
        return false;

    push(item);
    return true;
}
//------------------------------------------------------------------------------
int TCbuf::get()
{
    if(_count)
        return pop();
    else
        return -1;
}
//------------------------------------------------------------------------------
//
/// \note
/// For internal purposes.
/// Use this function with care - it doesn't perform free size check.
//
void TCbuf::push(const uint8_t item)
{
    _buf[_last] = item;
    _last++;
    _count++;

    if(_last == _size)
        _last = 0;
}
//------------------------------------------------------------------------------
//
/// \note
/// For internal purposes.
/// Use this function with care - it doesn't perform free size check.
//
uint8_t TCbuf::pop()
{
    uint8_t item = _buf[_first];

    _count--;
    _first++;
    if(_first == _size)
        _first = 0;

    return item;
}
//------------------------------------------------------------------------------
