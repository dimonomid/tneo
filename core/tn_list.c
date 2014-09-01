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
#include "tn_list.h"



//----------------------------------------------------------------------------
//      Circular double-linked list queue
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void tn_list_reset(struct TN_ListItem *list)
{
   list->prev = list->next = list;
}

//----------------------------------------------------------------------------
BOOL tn_is_list_empty(struct TN_ListItem *list)
{
   return (list->next == list && list->prev == list);
}

//----------------------------------------------------------------------------
void tn_list_add_head(struct TN_ListItem *list, struct TN_ListItem *entry)
{
  //--  Insert an entry at the head of the queue.

   entry->next = list->next;
   entry->prev = list;
   entry->next->prev = entry;
   list->next = entry;
}

//----------------------------------------------------------------------------
void tn_list_add_tail(struct TN_ListItem *list, struct TN_ListItem *entry)
{
  //-- Insert an entry at the tail of the queue.

   entry->next = list;
   entry->prev = list->prev;
   entry->prev->next = entry;
   list->prev = entry;
}

//----------------------------------------------------------------------------
struct TN_ListItem *tn_list_remove_head(struct TN_ListItem *list)
{
   //-- Remove and return an entry at the head of the queue.

   struct TN_ListItem *entry = NULL;

   if (list != NULL && list->next != list){
      entry             = list->next;
      entry->next->prev = list;
      list->next        = entry->next;
   } else {
      //-- NULL will be returned
   }

   return entry;
}

//----------------------------------------------------------------------------
struct TN_ListItem *tn_list_remove_tail(struct TN_ListItem *list)
{
   //-- Remove and return an entry at the tail of the queue.

   struct TN_ListItem *entry;

   if(list->prev == list)
      return (struct TN_ListItem *) 0;

   entry = list->prev;
   entry->prev->next = list;
   list->prev = entry->prev;
   return entry;
}

//----------------------------------------------------------------------------
void tn_list_remove_entry(struct TN_ListItem *entry)
{
   //--  Remove an entry from the queue.

   entry->prev->next = entry->next;
   entry->next->prev = entry->prev;
}

//----------------------------------------------------------------------------
BOOL tn_list_contains_entry(struct TN_ListItem *list, struct TN_ListItem *entry)
{
   struct TN_ListItem *curr_que;

   curr_que = list->next;

   while(curr_que != list){
      if (curr_que == entry)
         return TRUE;   //-- Found

      curr_que = curr_que->next;
   }

   return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


