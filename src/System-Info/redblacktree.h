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
// Red-Black Tree Implementation
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

#ifndef REDBLACKTREE_H
#define REDBLACKTREE_H

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

#define USE_LEAFLINKED 1

#ifdef USE_LEAFLINKED
#include <doublelinkedringlist.h>
#endif


typedef unsigned long long RedBlackTreeNodeValueType;

enum RedBlackTreeNodeColorType
{
   Red   = 1,
   Black = 2
};


struct RedBlackTreeNode
{
#ifdef USE_LEAFLINKED
   struct DoubleLinkedRingListNode ListNode;
#endif
   struct RedBlackTreeNode*        Parent;
   struct RedBlackTreeNode*        LeftSubtree;
   struct RedBlackTreeNode*        RightSubtree;
   enum RedBlackTreeNodeColorType  Color;
   RedBlackTreeNodeValueType       Value;
   RedBlackTreeNodeValueType       ValueSum;  /* ValueSum := LeftSubtree->Value + Value + RightSubtree->Value */
};

struct RedBlackTree
{
   struct RedBlackTreeNode     NullNode;
#ifdef USE_LEAFLINKED
   struct DoubleLinkedRingList List;
#endif
   size_t                      Elements;
   void                        (*PrintFunction)(const void* node, FILE* fd);
   int                         (*ComparisonFunction)(const void* node1, const void* node2);
};


void redBlackTreeNodeNew(struct RedBlackTreeNode* node);
void redBlackTreeNodeDelete(struct RedBlackTreeNode* node);
int redBlackTreeNodeIsLinked(const struct RedBlackTreeNode* node);


void redBlackTreeNew(struct RedBlackTree*  rbt,
                     void                  (*printFunction)(const void* node, FILE* fd),
                     int                   (*comparisonFunction)(const void* node1, const void* node2));
void redBlackTreeDelete(struct RedBlackTree* rbt);
void redBlackTreeVerify(struct RedBlackTree* rbt);
void redBlackTreePrint(const struct RedBlackTree* rbt,
                       FILE*                      fd);
int redBlackTreeIsEmpty(const struct RedBlackTree* rbt);
struct RedBlackTreeNode* redBlackTreeGetFirst(const struct RedBlackTree* rbt);
struct RedBlackTreeNode* redBlackTreeGetLast(const struct RedBlackTree* rbt);
struct RedBlackTreeNode* redBlackTreeGetPrev(const struct RedBlackTree*     rbt,
                                             const struct RedBlackTreeNode* node);
struct RedBlackTreeNode* redBlackTreeGetNext(const struct RedBlackTree*     rbt,
                                             const struct RedBlackTreeNode* node);
struct RedBlackTreeNode* redBlackTreeGetNearestPrev(const struct RedBlackTree*     rbt,
                                                    const struct RedBlackTreeNode* cmpNode);
struct RedBlackTreeNode* redBlackTreeGetNearestNext(const struct RedBlackTree*     rbt,
                                                    const struct RedBlackTreeNode* cmpNode);
size_t redBlackTreeGetElements(const struct RedBlackTree* rbt);
struct RedBlackTreeNode* redBlackTreeInsert(struct RedBlackTree*     rbt,
                                            struct RedBlackTreeNode* node);
struct RedBlackTreeNode* redBlackTreeRemove(struct RedBlackTree*     rbt,
                                            struct RedBlackTreeNode* node);
struct RedBlackTreeNode* redBlackTreeFind(const struct RedBlackTree*     rbt,
                                          const struct RedBlackTreeNode* cmpNode);
RedBlackTreeNodeValueType redBlackTreeGetValueSum(const struct RedBlackTree* rbt);
struct RedBlackTreeNode* redBlackTreeGetNodeByValue(const struct RedBlackTree* rbt,
                                                    RedBlackTreeNodeValueType  value);


#ifdef __cplusplus
}
#endif

#endif
