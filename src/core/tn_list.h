/*******************************************************************************
 *
 * TNeoKernel: real-time kernel initially based on TNKernel
 *
 *    TNKernel:                  copyright © 2004, 2013 Yuri Tiomkin.
 *    PIC32-specific routines:   copyright © 2013, 2014 Anders Montonen.
 *    TNeoKernel:                copyright © 2014       Dmitry Frank.
 *
 *    TNeoKernel was born as a thorough review and re-implementation of
 *    TNKernel. The new kernel has well-formed code, inherited bugs are fixed
 *    as well as new features being added, and it is tested carefully with
 *    unit-tests.
 *
 *    API is changed somewhat, so it's not 100% compatible with TNKernel,
 *    hence the new name: TNeoKernel.
 *
 *    Permission to use, copy, modify, and distribute this software in source
 *    and binary forms and its documentation for any purpose and without fee
 *    is hereby granted, provided that the above copyright notice appear
 *    in all copies and that both that copyright notice and this permission
 *    notice appear in supporting documentation.
 *
 *    THIS SOFTWARE IS PROVIDED BY THE DMITRY FRANK AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DMITRY FRANK OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *    THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/**
 * \file
 * Circular doubly linked list
 */



#ifndef  _TN_UTILS_H
#define  _TN_UTILS_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#include "tn_common.h"

#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

/**
 * Circular doubly linked list
 */
struct TN_ListItem {
   ///
   /// pointer to previous item
   struct TN_ListItem *prev;
   ///
   /// pointer to next item
   struct TN_ListItem *next;
};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*
 * NOTE: a lot of helper macros below were taken from Linux kernel source,
 *       from the file include/linux/list.h
 */


/**
 * tn_list_entry - get the struct for this entry
 * @ptr:	the &struct TN_ListItem pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_entry(ptr, type, member)                                 \
   container_of(ptr, type, member)

/**
 * tn_list_for_each	-	iterate over a list
 * @pos:	the &struct TN_ListItem to use as a loop cursor.
 * @head:	the head for your list.
 */
#define tn_list_for_each(pos, head)                                      \
   for (pos = (head)->next; pos != (head); pos = pos->next)


/**
 * tn_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define tn_list_first_entry(ptr, type, member)                           \
   tn_list_entry((ptr)->next, type, member)

/**
 * tn_list_first_entry_remove - remove the first element from a list
 * and return it
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define tn_list_first_entry_remove(ptr, type, member)                    \
   tn_list_entry(tn_list_remove_head(ptr), type, member)

/**
 * tn_list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define tn_list_last_entry(ptr, type, member)                            \
   tn_list_entry((ptr)->prev, type, member)

/**
 * tn_list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define tn_list_first_entry_or_null(ptr, type, member)                   \
   (!tn_list_empty(ptr) ? tn_list_first_entry(ptr, type, member) : NULL)

/**
 * tn_list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_next_entry(pos, member)                                  \
   tn_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * tn_list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_prev_entry(pos, member)                                  \
   tn_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * tn_list_for_each	-	iterate over a list
 * @pos:	the &struct TN_ListItem to use as a loop cursor.
 * @head:	the head for your list.
 */
#define tn_list_for_each(pos, head)                                      \
   for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * tn_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct TN_ListItem to use as a loop cursor.
 * @head:	the head for your list.
 */
#define tn_list_for_each_prev(pos, head)                                 \
   for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * tn_list_for_each_safe - iterate over a list safe against removal of list
 * entry
 * @pos:	the &struct TN_ListItem to use as a loop cursor.
 * @n:		another &struct TN_ListItem to use as temporary storage
 * @head:	the head for your list.
 */
#define tn_list_for_each_safe(pos, n, head)                              \
   for (pos = (head)->next, n = pos->next; pos != (head);                \
         pos = n, n = pos->next)

/**
 * tn_list_for_each_prev_safe - iterate over a list backwards safe against
 * removal of list entry
 * @pos:	the &struct TN_ListItem to use as a loop cursor.
 * @n:		another &struct TN_ListItem to use as temporary storage
 * @head:	the head for your list.
 */
#define tn_list_for_each_prev_safe(pos, n, head)                         \
   for (pos = (head)->prev, n = pos->prev;                               \
         pos != (head);                                                  \
         pos = n, n = pos->prev)

/**
 * tn_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_for_each_entry(pos, head, member)				            \
   for (pos = tn_list_first_entry(head, typeof(*pos), member);	         \
         &pos->member != (head);					                           \
         pos = tn_list_next_entry(pos, member))

/**
 * tn_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_for_each_entry_reverse(pos, head, member)			      \
   for (pos = tn_list_last_entry(head, typeof(*pos), member);		      \
         &pos->member != (head); 					                        \
         pos = tn_list_prev_entry(pos, member))

/**
 * tn_list_prepare_entry - prepare a pos entry for use in
 * tn_list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Prepares a pos entry for use as a start point in
 * tn_list_for_each_entry_continue().
 */
#define tn_list_prepare_entry(pos, head, member)                         \
   ((pos) ? : tn_list_entry(head, typeof(*pos), member))

/**
 * tn_list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define tn_list_for_each_entry_continue(pos, head, member) 		         \
   for (pos = tn_list_next_entry(pos, member);			                  \
         &pos->member != (head);					                           \
         pos = tn_list_next_entry(pos, member))

/**
 * tn_list_for_each_entry_continue_reverse - iterate backwards from the given
 * point
 *
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define tn_list_for_each_entry_continue_reverse(pos, head, member)		\
   for (pos = tn_list_prev_entry(pos, member);			                  \
         &pos->member != (head);					                           \
         pos = tn_list_prev_entry(pos, member))

/**
 * tn_list_for_each_entry_from - iterate over list of given type from the
 * current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define tn_list_for_each_entry_from(pos, head, member) 			         \
   for (; &pos->member != (head);					                        \
         pos = tn_list_next_entry(pos, member))

/**
 * tn_list_for_each_entry_safe - iterate over list of given type safe against
 * removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 */
#define tn_list_for_each_entry_safe(pos, n, head, member)			      \
   for (pos = tn_list_first_entry(head, typeof(*pos), member),	         \
         n = tn_list_next_entry(pos, member);			                  \
         &pos->member != (head); 					                        \
         pos = n, n = tn_list_next_entry(n, member))

/**
 * tn_list_for_each_entry_safe_continue - continue list iteration safe against
 * removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define tn_list_for_each_entry_safe_continue(pos, n, head, member) 		\
   for (pos = tn_list_next_entry(pos, member), 				               \
         n = tn_list_next_entry(pos, member);				               \
         &pos->member != (head);						                        \
         pos = n, n = tn_list_next_entry(n, member))

/**
 * tn_list_for_each_entry_safe_from - iterate over list from current point safe
 * against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define tn_list_for_each_entry_safe_from(pos, n, head, member) 			\
   for (n = tn_list_next_entry(pos, member);					               \
         &pos->member != (head);						                        \
         pos = n, n = tn_list_next_entry(n, member))

/**
 * tn_list_for_each_entry_safe_reverse - iterate backwards over list safe
 * against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define tn_list_for_each_entry_safe_reverse(pos, n, head, member)		   \
   for (pos = tn_list_last_entry(head, typeof(*pos), member),		      \
         n = tn_list_prev_entry(pos, member);			                  \
         &pos->member != (head); 					                        \
         pos = n, n = tn_list_prev_entry(n, member))

/**
 * tn_list_safe_reset_next - reset a stale tn_list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the tn_list_for_each_entry_safe loop
 * @n:		temporary storage used in tn_list_for_each_entry_safe
 * @member:	the name of the TN_ListItem member within the struct.
 *
 * tn_list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and tn_list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define tn_list_safe_reset_next(pos, n, member)				               \
   n = tn_list_next_entry(pos, member)



/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

void tn_list_reset(struct TN_ListItem *list);
BOOL tn_is_list_empty(struct TN_ListItem *list);
void tn_list_add_head(struct TN_ListItem *list, struct TN_ListItem *entry);
void tn_list_add_tail(struct TN_ListItem *list, struct TN_ListItem *entry);
struct TN_ListItem *tn_list_remove_head(struct TN_ListItem *list);
struct TN_ListItem *tn_list_remove_tail(struct TN_ListItem *list);
void tn_list_remove_entry(struct TN_ListItem *entry);
BOOL tn_list_contains_entry(struct TN_ListItem *list, struct TN_ListItem *entry);

#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // _TN_UTILS_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


