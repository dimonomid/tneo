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

/*******************************************************************************
 *   Circular double-linked list queue
 ******************************************************************************/



#ifndef  _TN_UTILS_H
#define  _TN_UTILS_H


/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/

#ifdef __cplusplus
extern "C"  {     /*}*/
#endif

/*******************************************************************************
 *    PUBLIC TYPES
 ******************************************************************************/

struct tn_que_head {
   struct tn_que_head *prev;
   struct tn_que_head *next;
};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

/*******************************************************************************
 *    PUBLIC FUNCTION PROTOTYPES
 ******************************************************************************/

void queue_reset(struct tn_que_head *que);
int  is_queue_empty(struct tn_que_head *que);
void queue_add_head(struct tn_que_head * que, struct tn_que_head * entry);
void queue_add_tail(struct tn_que_head * que, struct tn_que_head * entry);
struct tn_que_head * queue_remove_head(struct tn_que_head * que);
struct tn_que_head * queue_remove_tail(struct tn_que_head * que);
void queue_remove_entry(struct tn_que_head * entry);
int  queue_contains_entry(struct tn_que_head * que, struct tn_que_head * entry);

#ifdef __cplusplus
}
#endif


#endif // _TN_UTILS_H

/*******************************************************************************
 *    end of file
 ******************************************************************************/


