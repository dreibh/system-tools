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

#ifndef DOUBLELINKEDRINGLIST_H
#define DOUBLELINKEDRINGLIST_H

#ifdef __cplusplus
extern "C" {
#endif


struct DoubleLinkedRingListNode
{
   struct DoubleLinkedRingListNode* Prev;
   struct DoubleLinkedRingListNode* Next;
};

struct DoubleLinkedRingList
{
   struct DoubleLinkedRingListNode  Node;
   struct DoubleLinkedRingListNode* Head;
};


void doubleLinkedRingListNodeNew(struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListNodeDelete(struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListNew(struct DoubleLinkedRingList* list);
void doubleLinkedRingListDelete(struct DoubleLinkedRingList* list);
void doubleLinkedRingListAddAfter(struct DoubleLinkedRingListNode* after,
                                  struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListAddHead(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListAddTail(struct DoubleLinkedRingList*     list,
                                 struct DoubleLinkedRingListNode* node);
void doubleLinkedRingListRemNode(struct DoubleLinkedRingListNode* node);


#ifdef __cplusplus
}
#endif

#endif
