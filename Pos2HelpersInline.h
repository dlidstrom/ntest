// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

#pragma once


#include "Pos2Helpers.h"
#include "Debug.h"

///////////////////////////////////////////////////////////////////////////////
// CEmList routines
///////////////////////////////////////////////////////////////////////////////

inline void CEmpty::Remove() {
   prev->next=next;
#if CIRCULAR_EMPTIES
   next->prev=prev;
#else
   if (next)
      next->prev=prev;
#endif
}

inline void CEmpty::Add() {
   prev->next=this;
#if CIRCULAR_EMPTIES
   next->prev=this;
#else
   if (next)
      next->prev=this;
#endif
}

inline void CEmpty::RemoveParity() {
   prev->next=next;
   if (next)
      next->prev=prev;
#if PARITY_USAGE
   holeParity^=holeMask;
#endif
}

inline void CEmpty::AddParity() {
   prev->next=this;
   if (next)
      next->prev=this;
#if PARITY_USAGE
   holeParity^=holeMask;
#endif
}
