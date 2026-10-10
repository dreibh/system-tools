// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \.
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// Double-Linked Ring List Implementation
// Copyright (C) 2009-2026 by Thomas Dreibholz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Contact: thomas.dreibholz@gmail.com

#include <stddef.h>

#include <doublelinkedringlist.h>

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 202311L)
#ifndef nullptr
#define nullptr ((void*)0)
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


// ###### Initialize #########################################################
void doubleLinkedRingListNodeNew(struct DoubleLinkedRingListNode* node)
{
   node->Prev = nullptr;
   node->Next = nullptr;
}

// ###### Invalidate #########################################################
void doubleLinkedRingListNodeDelete(struct DoubleLinkedRingListNode* node)
{
   node->Prev = nullptr;
   node->Next = nullptr;
}


// ###### Initialize #########################################################
void doubleLinkedRingListNew(struct DoubleLinkedRingList* list)
{
   list->Node.Prev     = &list->Node;
   list->Node.Next     = &list->Node;
   list->Head          = &list->Node;
}


// ###### Invalidate #########################################################
void doubleLinkedRingListDelete(struct DoubleLinkedRingList* list)
{
   list->Head      = nullptr;
   list->Node.Prev = nullptr;
   list->Node.Next = nullptr;
}


// ###### Insert node after given node #######################################
void doubleLinkedRingListAddAfter(struct DoubleLinkedRingListNode* after,
                                  struct DoubleLinkedRingListNode* node)
{
   node->Next        = after->Next;
   node->Prev        = after;
   after->Next->Prev = node;
   after->Next       = node;
}


// ###### Insert node at head ################################################
void doubleLinkedRingListAddHead(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node)
{
   doubleLinkedRingListAddAfter(&list->Node, node);
}


// ###### Insert node at tail ################################################
void doubleLinkedRingListAddTail(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node)
{
   doubleLinkedRingListAddAfter(list->Node.Prev, node);
}


// ###### Remove node ########################################################
void doubleLinkedRingListRemNode(struct DoubleLinkedRingListNode* node)
{
   node->Prev->Next = node->Next;
   node->Next->Prev = node->Prev;
   node->Prev = nullptr;
   node->Next = nullptr;
}


#ifdef __cplusplus
}
#endif
