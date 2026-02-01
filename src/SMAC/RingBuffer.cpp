//=========================================================
//
//     FILE : RingBuffer.cpp
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

//--- Includes --------------------------------------------

#include "RingBuffer.h"
#include <Arduino.h>
#include <string.h>

//--- Constructor -----------------------------------------

RingBuffer::RingBuffer (BufferType inBufferType)
{
  bufferType  = inBufferType;
  numElements = headIndex = tailIndex = 0;
}

//--- Destructor ------------------------------------------

RingBuffer::~RingBuffer ()
{
  // Free all elements
  for (int i=numElements-1; i>=0; i--)
    free (elements[i]);
}

//--- GetNumElements --------------------------------------

int RingBuffer::GetNumElements ()
{
  return numElements;
}

//--- PushString ------------------------------------------

void RingBuffer::PushString (const char *element)
{
  // Push a new string into the ring
  if (numElements < MAX_ELEMENTS)
  {
    // Allocate space and copy data
    int  elementSize = strlen (element) + 1;  // include NULL terminator
    char *newElement = (char *) malloc (elementSize);
    memcpy (newElement, element, elementSize);

    if (bufferType == FIFO)
    {
      // FIFO
      elements[tailIndex] = newElement;
      tailIndex = (tailIndex + 1) % MAX_ELEMENTS;
    }
    else
    {
      // LIFO
      elements[numElements] = newElement;
    }

    ++numElements;
  }
}

//--- PopString -------------------------------------------

char *RingBuffer::PopString ()
{
  char *element = NULL;

  // Pop a string out of the Ring
  if (numElements > 0)
  {
    if (bufferType == FIFO)
    {
      // FIFO
      element = elements[headIndex];
      headIndex = (headIndex + 1) % MAX_ELEMENTS;
    }
    else
    {
      // LIFO
      element = elements[numElements-1];
    }

    --numElements;
  }

  return element;  // User must free this memory when done !!!  free (element);
}
