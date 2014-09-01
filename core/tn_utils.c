/*

  TNKernel real-time kernel

  Copyright © 2004, 2013 Yuri Tiomkin
  All rights reserved.

  Permission to use, copy, modify, and distribute this software in source
  and binary forms and its documentation for any purpose and without fee
  is hereby granted, provided that the above copyright notice appear
  in all copies and that both that copyright notice and this permission
  notice appear in supporting documentation.

  THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

*/

/* ver 2.7  */

#include "tn_common.h"
#include "tn_utils.h"



//----------------------------------------------------------------------------
//      Circular double-linked list queue
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void queue_reset(struct TN_QueHead *que)
{
   que->prev = que->next = que;
}

//----------------------------------------------------------------------------
int  is_queue_empty(struct TN_QueHead *que)
{
   if(que->next == que && que->prev == que)
      return 1;
   return 0;
}

//----------------------------------------------------------------------------
void queue_add_head(struct TN_QueHead * que, struct TN_QueHead * entry)
{
  //--  Insert an entry at the head of the queue.

   entry->next = que->next;
   entry->prev = que;
   entry->next->prev = entry;
   que->next = entry;
}

//----------------------------------------------------------------------------
void queue_add_tail(struct TN_QueHead * que, struct TN_QueHead * entry)
{
  //-- Insert an entry at the tail of the queue.

   entry->next = que;
   entry->prev = que->prev;
   entry->prev->next = entry;
   que->prev = entry;
}

//----------------------------------------------------------------------------
struct TN_QueHead * queue_remove_head(struct TN_QueHead * que)
{
   //-- Remove and return an entry at the head of the queue.

   struct TN_QueHead * entry;

   if(que == NULL || que->next == que)
      return (struct TN_QueHead *) 0;

   entry = que->next;
   entry->next->prev = que;
   que->next = entry->next;
   return entry;
}

//----------------------------------------------------------------------------
struct TN_QueHead * queue_remove_tail(struct TN_QueHead * que)
{
   //-- Remove and return an entry at the tail of the queue.

   struct TN_QueHead * entry;

   if(que->prev == que)
      return (struct TN_QueHead *) 0;

   entry = que->prev;
   entry->prev->next = que;
   que->prev = entry->prev;
   return entry;
}

//----------------------------------------------------------------------------
void queue_remove_entry(struct TN_QueHead * entry)
{
   //--  Remove an entry from the queue.

   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;
}

//----------------------------------------------------------------------------
int  queue_contains_entry(struct TN_QueHead * que, struct TN_QueHead * entry)
{
  //-- The entry in the queue ???

   struct TN_QueHead * curr_que;

   curr_que = que->next;
   while(curr_que != que)
   {
      if(curr_que == entry)
         return 1/*true*/;   //-- Found

      curr_que = curr_que->next;
   }

   return 0/*false*/;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


