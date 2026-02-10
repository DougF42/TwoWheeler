//=========================================================
//
//     FILE : RingBuffer.h
//
//  PROJECT : Any
//
//    NOTES : Implements a LIFO or Circular FIFO Stack.
//
//   AUTHOR : Bill Daniels
//            Copyright 1992-2026, D+S Tech Labs, Inc.
//            All Rights Reserved
//
//=========================================================

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

//--- Defines ---------------------------------------------

#define MAX_ELEMENTS  20

//--- Types -----------------------------------------------

enum BufferType
{
  FIFO,
  LIFO
};


//=========================================================
//  class RingBuffer
//=========================================================

class RingBuffer
{
  protected:
    BufferType  bufferType;
    int         numElements;
    int         headIndex;
    int         tailIndex;
    char        *elements[MAX_ELEMENTS];

  public:
    RingBuffer (BufferType inBufferType);
   ~RingBuffer ();

    int    GetNumElements ();
    void   PushString     (const char *newElement);
    char  *PopString      ();
};


#endif
