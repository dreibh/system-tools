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

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include <redblacktree.h>

#if !defined(HAVE_C23_NULLPTR)
#define nullptr ((void*)0)
#endif


#ifdef __cplusplus
extern "C" {
#endif


static struct RedBlackTreeNode* redBlackTreeInternalFindPrev(
                                    const struct RedBlackTree*     rbt,
                                    const struct RedBlackTreeNode* cmpNode);
static struct RedBlackTreeNode* redBlackTreeInternalFindNext(
                                    const struct RedBlackTree*     rbt,
                                    const struct RedBlackTreeNode* cmpNode);


// ###### Initialize ########################################################
void redBlackTreeNodeNew(struct RedBlackTreeNode* node)
{
#if defined(RBT_LEAFLINKED)
   doubleLinkedRingListNodeNew(&node->ListNode);
#endif
   node->Parent       = nullptr;
   node->LeftSubtree  = nullptr;
   node->RightSubtree = nullptr;
   node->Color        = Black;
   node->Value        = 0;
   node->ValueSum     = 0;
}


// ###### Invalidate ########################################################
void redBlackTreeNodeDelete(struct RedBlackTreeNode* node)
{
   node->Parent       = nullptr;
   node->LeftSubtree  = nullptr;
   node->RightSubtree = nullptr;
   node->Color        = Black;
   node->Value        = 0;
   node->ValueSum     = 0;
#if defined(RBT_LEAFLINKED)
   doubleLinkedRingListNodeDelete(&node->ListNode);
#endif
}


// ###### Is node linked? ###################################################
int redBlackTreeNodeIsLinked(const struct RedBlackTreeNode* node)
{
   return node->LeftSubtree != nullptr;
}


// ##### Initialize #########################################################
void redBlackTreeNew(
        struct RedBlackTree* rbt,
        void                 (*printFunction)(const void* node, FILE* fd),
        int                  (*comparisonFunction)(const void* node1, const void* node2))
{
#if defined(RBT_LEAFLINKED)
   doubleLinkedRingListNew(&rbt->List);
#endif
   rbt->PrintFunction         = printFunction;
   rbt->ComparisonFunction    = comparisonFunction;
   rbt->NullNode.Parent       = &rbt->NullNode;
   rbt->NullNode.LeftSubtree  = &rbt->NullNode;
   rbt->NullNode.RightSubtree = &rbt->NullNode;
   rbt->NullNode.Color        = Black;
   rbt->NullNode.Value        = 0;
   rbt->NullNode.ValueSum     = 0;
   rbt->Elements              = 0;
}


// ##### Invalidate #########################################################
void redBlackTreeDelete(struct RedBlackTree* rbt)
{
   rbt->Elements              = 0;
   rbt->NullNode.Parent       = nullptr;
   rbt->NullNode.LeftSubtree  = nullptr;
   rbt->NullNode.RightSubtree = nullptr;
#if defined(RBT_LEAFLINKED)
   doubleLinkedRingListDelete(&rbt->List);
#endif
}


// ##### Update value sum ###################################################
inline static void redBlackTreeUpdateValueSum(struct RedBlackTreeNode* node)
{
   node->ValueSum = node->LeftSubtree->ValueSum + node->Value + node->RightSubtree->ValueSum;
}


// ##### Update value sum for node and all parents up to tree root ##########
static void redBlackTreeUpdateValueSumsUpToRoot(
               struct RedBlackTree*     rbt,
               struct RedBlackTreeNode* node)
{
   while(node != &rbt->NullNode) {
       redBlackTreeUpdateValueSum(node);
       node = node->Parent;
   }
}


// ###### Internal method for printing a node ################################
static void redBlackTreePrintNode(
               const struct RedBlackTree*     rbt,
               const struct RedBlackTreeNode* node,
               FILE*                          fd)
{
   rbt->PrintFunction(node, fd);
#ifdef DEBUG
   fprintf(fd, " ptr=%p c=%s v=%llu vsum=%llu",
           node, ((node->Color == Red) ? "Red" : "Black"),
           node->Value, node->ValueSum);
   if(node->LeftSubtree != &rbt->NullNode) {
      fprintf(fd, " l=%p[", node->LeftSubtree);
      rbt->PrintFunction(node->LeftSubtree, fd);
      fprintf(fd, "]");
   }
   else {
      fprintf(fd, " l=()");
   }
   if(node->RightSubtree != &rbt->NullNode) {
      fprintf(fd, " r=%p[", node->RightSubtree);
      rbt->PrintFunction(node->RightSubtree, fd);
      fprintf(fd, "]");
   }
   else {
      fprintf(fd, " r=()");
   }
   if(node->Parent != &rbt->NullNode) {
      fprintf(fd, " p=%p[", node->Parent);
      rbt->PrintFunction(node->Parent, fd);
      fprintf(fd, "]   ");
   }
   else {
      fprintf(fd, " p=())   ");
   }
   fputs("\n", fd);
#endif
}


// ##### Internal printing function #########################################
void redBlackTreeInternalPrint(const struct RedBlackTree*     rbt,
                               const struct RedBlackTreeNode* node,
                               FILE*                          fd)
{
   if(node != &rbt->NullNode) {
      redBlackTreeInternalPrint(rbt, node->LeftSubtree, fd);
      redBlackTreePrintNode(rbt, node, fd);
      redBlackTreeInternalPrint(rbt, node->RightSubtree, fd);
   }
}


// ###### Print tree ########################################################
void redBlackTreePrint(const struct RedBlackTree* rbt,
                       FILE*                      fd)
{
#ifdef DEBUG
   fprintf(fd, "\n\nroot=%p[", rbt->NullNode.LeftSubtree);
   if(rbt->NullNode.LeftSubtree != &rbt->NullNode) {
      rbt->PrintFunction(rbt->NullNode.LeftSubtree, fd);
   }
   fprintf(fd, "] null=%p   \n", &rbt->NullNode);
#endif
   redBlackTreeInternalPrint(rbt, rbt->NullNode.LeftSubtree, fd);
}


// ###### Is tree empty? ####################################################
int redBlackTreeIsEmpty(const struct RedBlackTree* rbt)
{
   return rbt->NullNode.LeftSubtree == &rbt->NullNode;
}


// ###### Get first node #####################################################
struct RedBlackTreeNode* redBlackTreeGetFirst(const struct RedBlackTree* rbt)
{
#if defined(RBT_LEAFLINKED)
   struct DoubleLinkedRingListNode* node = rbt->List.Node.Next;
   if(node != rbt->List.Head) {
      return (struct RedBlackTreeNode*)node;
   }
   return nullptr;
#else
   const struct RedBlackTreeNode* node = rbt->NullNode.LeftSubtree;
   if(node == &rbt->NullNode) {
      node = rbt->NullNode.RightSubtree;
   }
   while(node->LeftSubtree != &rbt->NullNode) {
      node = node->LeftSubtree;
   }
   if(node != &rbt->NullNode) {
      return (struct RedBlackTreeNode*)node;
   }
   return nullptr;
#endif
}


// ###### Get last node ######################################################
struct RedBlackTreeNode* redBlackTreeGetLast(const struct RedBlackTree* rbt)
{
#if defined(RBT_LEAFLINKED)
   struct DoubleLinkedRingListNode* node = rbt->List.Node.Prev;
   if(node != rbt->List.Head) {
      return (struct RedBlackTreeNode*)node;
   }
   return nullptr;
#else
   const struct RedBlackTreeNode* node = rbt->NullNode.RightSubtree;
   if(node == &rbt->NullNode) {
      node = rbt->NullNode.LeftSubtree;
   }
   while(node->RightSubtree != &rbt->NullNode) {
      node = node->RightSubtree;
   }
   if(node != &rbt->NullNode) {
      return (struct RedBlackTreeNode*)node;
   }
   return nullptr;
#endif
}


// ###### Get previous node ##################################################
struct RedBlackTreeNode* redBlackTreeGetPrev(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* node)
{
#if defined(RBT_LEAFLINKED)
   struct DoubleLinkedRingListNode* prev = node->ListNode.Prev;
   if(prev != rbt->List.Head) {
      return (struct RedBlackTreeNode*)prev;
   }
   return nullptr;
#else
   struct RedBlackTreeNode* result;
   result = redBlackTreeInternalFindPrev(rbt, node);
   if(result != &rbt->NullNode) {
      return result;
   }
   return nullptr;
#endif
}


// ###### Get next node #####################################################
struct RedBlackTreeNode* redBlackTreeGetNext(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* node)
{
#if defined(RBT_LEAFLINKED)
   struct DoubleLinkedRingListNode* next = node->ListNode.Next;
   if(next != rbt->List.Head) {
      return (struct RedBlackTreeNode*)next;
   }
   return nullptr;
#else
   struct RedBlackTreeNode* result;
   result = redBlackTreeInternalFindNext(rbt, node);
   if(result != &rbt->NullNode) {
      return result;
   }
   return nullptr;
#endif
}


// ###### Find nearest previous node ########################################
struct RedBlackTreeNode* redBlackTreeGetNearestPrev(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* cmpNode)
{
   struct RedBlackTreeNode*const* nodePtr;
   struct RedBlackTreeNode*const* parentPtr;
   const struct RedBlackTreeNode* node;
   const struct RedBlackTreeNode* parent;
   int                                           cmpResult = 0;

#ifdef DEBUG
   printf("nearest prev: ");
   rbt->PrintFunction(cmpNode, stdout);
   printf("\n");
   redBlackTreePrint(rbt, stdout);
#endif

   parentPtr = nullptr;
   nodePtr   = &rbt->NullNode.LeftSubtree;
   while(*nodePtr != &rbt->NullNode) {
      cmpResult = rbt->ComparisonFunction(cmpNode, *nodePtr);
      if(cmpResult < 0) {
         parentPtr = nodePtr;
         nodePtr   = &(*nodePtr)->LeftSubtree;
      }
      else if(cmpResult > 0) {
         parentPtr = nodePtr;
         nodePtr   = &(*nodePtr)->RightSubtree;
      }
      if(cmpResult == 0) {
         return redBlackTreeGetPrev(rbt, *nodePtr);
      }
   }

   if(parentPtr == nullptr) {
      if(cmpResult > 0) {
         return rbt->NullNode.LeftSubtree;
      }
      return nullptr;
   }
   else {
      /* The new node would be the right child of its parent.
         => The parent is the nearest previous node! */
      if(nodePtr == &(*parentPtr)->RightSubtree) {
         return *parentPtr;
      }
      else {
         parent = *parentPtr;

         /* If there is a left subtree, the nearest previous node is the
            rightmost child of the left subtree. */
         if(parent->LeftSubtree != &rbt->NullNode) {
            node = parent->LeftSubtree;
            while(node->RightSubtree != &rbt->NullNode) {
               node = node->RightSubtree;
            }
            if(node != &rbt->NullNode) {
               return (struct RedBlackTreeNode*)node;
            }
         }

         /* If there is no left subtree, the nearest previous node is an
            ancestor node which has the node on its right side. */
         else {
            node   = parent;
            parent = node->Parent;
            while((parent != &rbt->NullNode) && (node == parent->LeftSubtree)) {
               node   = parent;
               parent = parent->Parent;
            }
            if(parent != &rbt->NullNode) {
               return (struct RedBlackTreeNode*)parent;
            }
         }
      }
   }
   return nullptr;
}


// ###### Find nearest next node ############################################
struct RedBlackTreeNode* redBlackTreeGetNearestNext(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* cmpNode)
{
   struct RedBlackTreeNode*const* nodePtr;
   struct RedBlackTreeNode*const* parentPtr;
   const struct RedBlackTreeNode* node;
   const struct RedBlackTreeNode* parent;
   int                            cmpResult = 0;

#ifdef DEBUG
   printf("nearest next: ");
   rbt->PrintFunction(cmpNode, stdout);
   printf("\n");
   redBlackTreePrint(bt, stdout);
#endif

   parentPtr = nullptr;
   nodePtr   = &rbt->NullNode.LeftSubtree;
   while(*nodePtr != &rbt->NullNode) {
      cmpResult = rbt->ComparisonFunction(cmpNode, *nodePtr);
      if(cmpResult < 0) {
         parentPtr = nodePtr;
         nodePtr   = &(*nodePtr)->LeftSubtree;
      }
      else if(cmpResult > 0) {
         parentPtr = nodePtr;
         nodePtr   = &(*nodePtr)->RightSubtree;
      }
      if(cmpResult == 0) {
         return redBlackTreeGetNext(rbt, *nodePtr);
      }
   }

   if(parentPtr == nullptr) {
      if(cmpResult < 0) {
         return rbt->NullNode.LeftSubtree;
      }
      return nullptr;
   }
   else {
      /* The new node would be the left child of its parent.
         => The parent is the nearest next node! */
      if(nodePtr == &(*parentPtr)->LeftSubtree) {
         return *parentPtr;
      }
      else {
         parent = *parentPtr;

         /* If there is a right subtree, the nearest next node is the
            leftmost child of the right subtree. */
         if(parent->RightSubtree != &rbt->NullNode) {
            node = parent->RightSubtree;
            while(node->LeftSubtree != &rbt->NullNode) {
               node = node->LeftSubtree;
            }
            if(node != &rbt->NullNode) {
               return (struct RedBlackTreeNode*)node;
            }
         }

         /* If there is no right subtree, the nearest next node is an
            ancestor node which has the node on its left side. */
         else {
            node   = parent;
            parent = node->Parent;
            while((parent != &rbt->NullNode) && (node == parent->RightSubtree)) {
               node   = parent;
               parent = parent->Parent;
            }
            if(parent != &rbt->NullNode) {
               return (struct RedBlackTreeNode*)parent;
            }
         }
      }
   }
   return nullptr;
}


// ###### Get number of elements #############################################
size_t redBlackTreeGetElements(const struct RedBlackTree* rbt)
{
   return rbt->Elements;
}


// ###### Get prev node by walking through the tree (does *not* use list!) */
struct RedBlackTreeNode* redBlackTreeInternalFindPrev(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* cmpNode)
{
   const struct RedBlackTreeNode* node = cmpNode->LeftSubtree;
   const struct RedBlackTreeNode* parent;

   if(node != &rbt->NullNode) {
      while(node->RightSubtree != &rbt->NullNode) {
         node = node->RightSubtree;
      }
      return (struct RedBlackTreeNode*)node;
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &rbt->NullNode) && (node == parent->LeftSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return (struct RedBlackTreeNode*)parent;
   }
}


// ###### Get next node by walking through the tree (does *not* use list!) */
struct RedBlackTreeNode* redBlackTreeInternalFindNext(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* cmpNode)
{
   const struct RedBlackTreeNode* node = cmpNode->RightSubtree;
   const struct RedBlackTreeNode* parent;

   if(node != &rbt->NullNode) {
      while(node->LeftSubtree != &rbt->NullNode) {
         node = node->LeftSubtree;
      }
      return (struct RedBlackTreeNode*)node;
   }
   else {
      node   = cmpNode;
      parent = cmpNode->Parent;
      while((parent != &rbt->NullNode) && (node == parent->RightSubtree)) {
         node   = parent;
         parent = parent->Parent;
      }
      return (struct RedBlackTreeNode*)parent;
   }
}


// ###### Find node ##########################################################
struct RedBlackTreeNode* redBlackTreeFind(
                            const struct RedBlackTree*     rbt,
                            const struct RedBlackTreeNode* cmpNode)
{
#ifdef DEBUG
   printf("find: ");
   rbt->PrintFunction(cmpNode, stdout);
   printf("\n");
#endif

   struct RedBlackTreeNode* node = rbt->NullNode.LeftSubtree;
   while(node != &rbt->NullNode) {
      const int cmpResult = rbt->ComparisonFunction(cmpNode, node);
      if(cmpResult == 0) {
         return node;
      }
      else if(cmpResult < 0) {
         node = node->LeftSubtree;
      }
      else {
         node = node->RightSubtree;
      }
   }
   return nullptr;
}


// ###### Get value sum from root node ######################################
RedBlackTreeNodeValueType redBlackTreeGetValueSum(
                             const struct RedBlackTree* rbt)
{
   return rbt->NullNode.LeftSubtree->ValueSum;
}


// ##### Rotation with left subtree #########################################
static void redBlackTreeRotateLeft(
               struct RedBlackTreeNode* node)
{
   struct RedBlackTreeNode* lower;
   struct RedBlackTreeNode* lowleft;
   struct RedBlackTreeNode* upparent;

   lower = node->RightSubtree;
   node->RightSubtree = lowleft = lower->LeftSubtree;
   lowleft->Parent = node;
   lower->Parent = upparent = node->Parent;

   if(node == upparent->LeftSubtree) {
      upparent->LeftSubtree = lower;
   } else {
      assert(node == upparent->RightSubtree);
      upparent->RightSubtree = lower;
   }

   lower->LeftSubtree = node;
   node->Parent = lower;

   redBlackTreeUpdateValueSum(node);
   redBlackTreeUpdateValueSum(node->Parent);
}


// ##### Rotation with ripht subtree ########################################
static void redBlackTreeRotateRight(
               struct RedBlackTreeNode* node)
{
   struct RedBlackTreeNode* lower;
   struct RedBlackTreeNode* lowright;
   struct RedBlackTreeNode* upparent;

   lower = node->LeftSubtree;
   node->LeftSubtree = lowright = lower->RightSubtree;
   lowright->Parent = node;
   lower->Parent = upparent = node->Parent;

   if(node == upparent->RightSubtree) {
      upparent->RightSubtree = lower;
   } else {
      assert(node == upparent->LeftSubtree);
      upparent->LeftSubtree = lower;
   }

   lower->RightSubtree = node;
   node->Parent = lower;

   redBlackTreeUpdateValueSum(node);
   redBlackTreeUpdateValueSum(node->Parent);
}


// ###### Insert ############################################################
struct RedBlackTreeNode* redBlackTreeInsert(struct RedBlackTree*     rbt,
                                            struct RedBlackTreeNode* node)
{
   int                      cmpResult = -1;
   struct RedBlackTreeNode* where     = rbt->NullNode.LeftSubtree;
   struct RedBlackTreeNode* parent    = &rbt->NullNode;
   struct RedBlackTreeNode* result;
   struct RedBlackTreeNode* uncle;
   struct RedBlackTreeNode* grandpa;
#if defined(RBT_LEAFLINKED)
   struct RedBlackTreeNode* prev;
#endif
#ifdef DEBUG
   printf("insert: ");
   rbt->PrintFunction(node, stdout);
   printf("\n");
#endif


   // ====== Find location of new node ======================================
   while(where != &rbt->NullNode) {
      parent = where;
      cmpResult = rbt->ComparisonFunction(node, where);
      if(cmpResult < 0) {
         where = where->LeftSubtree;
      }
      else if(cmpResult > 0) {
         where = where->RightSubtree;
      }
      else {
         /* Node with same key is already available -> return. */
         result = where;
         goto finished;
      }
   }
   assert(where == &rbt->NullNode);

   if(cmpResult < 0) {
      parent->LeftSubtree = node;
   }
   else {
      parent->RightSubtree = node;
   }


   // ====== Link node ======================================================
   node->Parent       = parent;
   node->LeftSubtree  = &rbt->NullNode;
   node->RightSubtree = &rbt->NullNode;
   node->ValueSum     = node->Value;
#if defined(RBT_LEAFLINKED)
   prev = redBlackTreeInternalFindPrev(rbt, node);
   if(prev != &rbt->NullNode) {
      doubleLinkedRingListAddAfter(&prev->ListNode, &node->ListNode);
   }
   else {
      doubleLinkedRingListAddHead(&rbt->List, &node->ListNode);
   }
#endif
   rbt->Elements++;
   result = node;


   // ====== Update parent's value sum ======================================
   redBlackTreeUpdateValueSumsUpToRoot(rbt, node->Parent);


   // ====== Ensure red-black tree properties ===============================
   node->Color = Red;
   while (parent->Color == Red) {
      grandpa = parent->Parent;
      if(parent == grandpa->LeftSubtree) {
         uncle = grandpa->RightSubtree;
         if(uncle->Color == Red) {
            parent->Color  = Black;
            uncle->Color   = Black;
            grandpa->Color = Red;
            node           = grandpa;
            parent         = grandpa->Parent;
         } else {
            if(node == parent->RightSubtree) {
               redBlackTreeRotateLeft(parent);
               parent = node;
               assert(grandpa == parent->Parent);
            }
            parent->Color  = Black;
            grandpa->Color = Red;
            redBlackTreeRotateRight(grandpa);
            break;
         }
      } else {
         uncle = grandpa->LeftSubtree;
         if(uncle->Color == Red) {
            parent->Color  = Black;
            uncle->Color   = Black;
            grandpa->Color = Red;
            node           = grandpa;
            parent         = grandpa->Parent;
         } else {
            if(node == parent->LeftSubtree) {
               redBlackTreeRotateRight(parent);
               parent = node;
               assert(grandpa == parent->Parent);
            }
            parent->Color  = Black;
            grandpa->Color = Red;
            redBlackTreeRotateLeft(grandpa);
            break;
         }
      }
   }
   rbt->NullNode.LeftSubtree->Color = Black;


finished:
#ifdef DEBUG
   redBlackTreePrint(rbt, stdout);
#endif
#ifdef VERIFY
   redBlackTreeVerify(rbt);
#endif
   return result;
}


// ###### Remove ############################################################
struct RedBlackTreeNode* redBlackTreeRemove(struct RedBlackTree*     rbt,
                                            struct RedBlackTreeNode* node)
{
   struct RedBlackTreeNode* child;
   struct RedBlackTreeNode* delParent;
   struct RedBlackTreeNode* parent;
   struct RedBlackTreeNode* sister;
   struct RedBlackTreeNode* next;
   struct RedBlackTreeNode* nextParent;
   enum RedBlackTreeNodeColorType          nextColor;
#ifdef DEBUG
   printf("remove: ");
   rbt->PrintFunction(node, stdout);
   printf("\n");
#endif

   assert(redBlackTreeNodeIsLinked(node));

   // ====== Unlink node ====================================================
   if((node->LeftSubtree != &rbt->NullNode) && (node->RightSubtree != &rbt->NullNode)) {
      next       = redBlackTreeGetNext(rbt, node);
      nextParent = next->Parent;
      nextColor  = next->Color;

      assert(next != &rbt->NullNode);
      assert(next->Parent != &rbt->NullNode);
      assert(next->LeftSubtree == &rbt->NullNode);

      child         = next->RightSubtree;
      child->Parent = nextParent;
      if(nextParent->LeftSubtree == next) {
         nextParent->LeftSubtree = child;
      } else {
         assert(nextParent->RightSubtree == next);
         nextParent->RightSubtree = child;
      }


      delParent                  = node->Parent;
      next->Parent               = delParent;
      next->LeftSubtree          = node->LeftSubtree;
      next->RightSubtree         = node->RightSubtree;
      next->LeftSubtree->Parent  = next;
      next->RightSubtree->Parent = next;
      next->Color                = node->Color;
      node->Color                = nextColor;

      if(delParent->LeftSubtree == node) {
         delParent->LeftSubtree = next;
      } else {
         assert(delParent->RightSubtree == node);
         delParent->RightSubtree = next;
      }

      // ====== Update parent's value sum ===================================
      redBlackTreeUpdateValueSumsUpToRoot(rbt, next);
      redBlackTreeUpdateValueSumsUpToRoot(rbt, nextParent);
   } else {
      assert(node != &rbt->NullNode);
      assert((node->LeftSubtree == &rbt->NullNode) || (node->RightSubtree == &rbt->NullNode));

      child         = (node->LeftSubtree != &rbt->NullNode) ? node->LeftSubtree : node->RightSubtree;
      child->Parent = delParent = node->Parent;

      if(node == delParent->LeftSubtree) {
         delParent->LeftSubtree = child;
      } else {
         assert(node == delParent->RightSubtree);
         delParent->RightSubtree = child;
      }

      // ====== Update parent's value sum ===================================
      redBlackTreeUpdateValueSumsUpToRoot(rbt, delParent);
   }


   // ====== Unlink node from list and invalidate pointers ==================
   node->Parent       = nullptr;
   node->RightSubtree = nullptr;
   node->LeftSubtree  = nullptr;
#if defined(RBT_LEAFLINKED)
   doubleLinkedRingListRemNode(&node->ListNode);
   node->ListNode.Prev = nullptr;
   node->ListNode.Next = nullptr;
#endif
   assert(rbt->Elements > 0);
   rbt->Elements--;


   // ====== Ensure red-black properties ====================================
   if(node->Color == Black) {
      rbt->NullNode.LeftSubtree->Color = Red;

      while (child->Color == Black) {
         parent = child->Parent;
         if(child == parent->LeftSubtree) {
            sister = parent->RightSubtree;
            assert(sister != &rbt->NullNode);
            if(sister->Color == Red) {
               sister->Color = Black;
               parent->Color = Red;
               redBlackTreeRotateLeft(parent);
               sister = parent->RightSubtree;
               assert(sister != &rbt->NullNode);
            }
            if((sister->LeftSubtree->Color == Black) &&
               (sister->RightSubtree->Color == Black)) {
               sister->Color = Red;
               child = parent;
            } else {
               if(sister->RightSubtree->Color == Black) {
                  assert(sister->LeftSubtree->Color == Red);
                  sister->LeftSubtree->Color = Black;
                  sister->Color = Red;
                  redBlackTreeRotateRight(sister);
                  sister = parent->RightSubtree;
                  assert(sister != &rbt->NullNode);
               }
               sister->Color = parent->Color;
               sister->RightSubtree->Color = Black;
               parent->Color = Black;
               redBlackTreeRotateLeft(parent);
               break;
            }
         } else {
            assert(child == parent->RightSubtree);
            sister = parent->LeftSubtree;
            assert(sister != &rbt->NullNode);
            if(sister->Color == Red) {
               sister->Color = Black;
               parent->Color = Red;
               redBlackTreeRotateRight(parent);
               sister = parent->LeftSubtree;
               assert(sister != &rbt->NullNode);
            }
            if((sister->RightSubtree->Color == Black) &&
               (sister->LeftSubtree->Color == Black)) {
               sister->Color = Red;
               child = parent;
            } else {
               if(sister->LeftSubtree->Color == Black) {
                  assert(sister->RightSubtree->Color == Red);
                  sister->RightSubtree->Color = Black;
                  sister->Color = Red;
                  redBlackTreeRotateLeft(sister);
                  sister = parent->LeftSubtree;
                  assert(sister != &rbt->NullNode);
               }
               sister->Color = parent->Color;
               sister->LeftSubtree->Color = Black;
               parent->Color = Black;
               redBlackTreeRotateRight(parent);
               break;
            }
         }
      }
      child->Color = Black;
      rbt->NullNode.LeftSubtree->Color = Black;
   }


#ifdef DEBUG
    redBlackTreePrint(rbt, stdout);
#endif
#ifdef VERIFY
    redBlackTreeVerify(rbt);
#endif
   return node;
}


// ##### Get node by value ##################################################
struct RedBlackTreeNode* redBlackTreeGetNodeByValue(
                            const struct RedBlackTree* rbt,
                            RedBlackTreeNodeValueType  value)
{
   const struct RedBlackTreeNode* node = rbt->NullNode.LeftSubtree;
   for(;;) {
      if(value < node->LeftSubtree->ValueSum) {
         if(node->LeftSubtree != &rbt->NullNode) {
            node = node->LeftSubtree;
         }
         else {
            break;
         }
      }
      else if(value < node->LeftSubtree->ValueSum + node->Value) {
         break;
      }
      else {
         if(node->RightSubtree != &rbt->NullNode) {
            value -= node->LeftSubtree->ValueSum + node->Value;
            node = node->RightSubtree;
         }
         else {
            break;
         }
      }
   }

   if(node !=  &rbt->NullNode) {
      return (struct RedBlackTreeNode*)node;
   }
   return nullptr;
}


// ##### Internal verification function #####################################
static size_t redBlackTreeInternalVerify(
                 struct RedBlackTree*              rbt,
                 struct RedBlackTreeNode*          parent,
                 struct RedBlackTreeNode*          node,
                 struct RedBlackTreeNode**         lastRedBlackTreeNode,
#if defined(RBT_LEAFLINKED)
                 struct DoubleLinkedRingListNode** lastListNode,
#endif
                 size_t*                           counter)
{
#if defined(RBT_LEAFLINKED)
   struct RedBlackTreeNode* prev;
   struct RedBlackTreeNode* next;
#endif
   size_t                   leftHeight;
   size_t                   rightHeight;

   if(node != &rbt->NullNode) {
      // ====== Print node ==================================================
#ifdef DEBUG
      printf("verifying ");
      redBlackTreePrintNode(rbt, node, stdout);
      puts("");
#endif

      // ====== Correct parent? =============================================
      assert(node->Parent == parent);

      // ====== Correct tree and heap properties? ===========================
      if(node->LeftSubtree != &rbt->NullNode) {
         assert(rbt->ComparisonFunction(node, node->LeftSubtree) > 0);
      }
      if(node->RightSubtree != &rbt->NullNode) {
         assert(rbt->ComparisonFunction(node, node->RightSubtree) < 0);
      }

      // ====== Is value sum okay? ==========================================
      assert(node->ValueSum == node->LeftSubtree->ValueSum +
                              node->Value +
                              node->RightSubtree->ValueSum);

      // ====== Is left subtree okay? =======================================
      leftHeight = redBlackTreeInternalVerify(
                      rbt, node, node->LeftSubtree, lastRedBlackTreeNode,
#if defined(RBT_LEAFLINKED)
                      lastListNode,
#endif
                      counter);

#if defined(RBT_LEAFLINKED)
      // ====== Is ring list okay? ==========================================
      assert((*lastListNode)->Next != rbt->List.Head);
      *lastListNode = (*lastListNode)->Next;
      assert(*lastListNode == &node->ListNode);
#endif

#if defined(RBT_LEAFLINKED)
      // ====== Is linking working correctly? ===============================
      prev = redBlackTreeInternalFindPrev(rbt, node);
      if(prev != &rbt->NullNode) {
         assert((*lastListNode)->Prev == &prev->ListNode);
      }
      else {
         assert((*lastListNode)->Prev == rbt->List.Head);
      }

      next = redBlackTreeInternalFindNext(rbt, node);
      if(next != &rbt->NullNode) {
         assert((*lastListNode)->Next == &next->ListNode);
      }
      else {
         assert((*lastListNode)->Next == rbt->List.Head);
      }
#endif

      // ====== Count elements ==============================================
      (*counter)++;

      // ====== Is right subtree okay? ======================================
      rightHeight = redBlackTreeInternalVerify(
                       rbt, node, node->RightSubtree, lastRedBlackTreeNode,
#if defined(RBT_LEAFLINKED)
                       lastListNode,
#endif
                       counter);

      // ====== Verify red-black property ===================================
      assert((leftHeight != 0) || (rightHeight != 0));
      assert(leftHeight == rightHeight);
      if(node->Color == Red) {
         assert(node->LeftSubtree->Color == Black);
         assert(node->RightSubtree->Color == Black);
         return leftHeight;
      }
      assert(node->Color == Black);
      return leftHeight + 1;
   }
   return 1;
}


// ##### Verify structures ##################################################
void redBlackTreeVerify(struct RedBlackTree* rbt)
{
   size_t                           counter              = 0;
   struct RedBlackTreeNode*         lastRedBlackTreeNode = nullptr;
#if defined(RBT_LEAFLINKED)
   struct DoubleLinkedRingListNode* lastListNode         = &rbt->List.Node;
#endif

   assert(rbt->NullNode.Color == Black);
   assert(rbt->NullNode.Value == 0);
   assert(rbt->NullNode.ValueSum == 0);

#if defined(RBT_LEAFLINKED)
   assert(redBlackTreeInternalVerify(rbt, &rbt->NullNode,
                                     rbt->NullNode.LeftSubtree,
                                     &lastRedBlackTreeNode,
                                     &lastListNode,
                                     &counter) != 0);
#else
   assert(redBlackTreeInternalVerify(rbt, &rbt->NullNode,
                                     rbt->NullNode.LeftSubtree,
                                     &lastRedBlackTreeNode,
                                     &counter) != 0);
#endif
   assert(counter == rbt->Elements);
}


#ifdef __cplusplus
}
#endif
