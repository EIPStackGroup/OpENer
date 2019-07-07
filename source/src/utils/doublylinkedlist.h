/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_UTILS_DOUBLYLINKEDLIST_H_
#define SRC_UTILS_DOUBLYLINKEDLIST_H_

/**
 * @file doublylinkedlist.h
 *
 * The public interface for a reference type doubly linked list
 */

typedef struct doubly_linked_list_node DoublyLinkedListNode;

typedef DoublyLinkedListNode * (*NodeMemoryAllocator)(
  );

typedef void (*NodeMemoryDeallocator)(DoublyLinkedListNode **node);

typedef struct doubly_linked_list_node {
  DoublyLinkedListNode *previous;
  DoublyLinkedListNode *next;
  void *data;
} DoublyLinkedListNode;

typedef struct {
  DoublyLinkedListNode *first;
  DoublyLinkedListNode *last;

  NodeMemoryAllocator allocator;
  NodeMemoryDeallocator deallocator;

} DoublyLinkedList;

void DoublyLinkedListInitialize(DoublyLinkedList *list,
                                NodeMemoryAllocator allocator,
                                NodeMemoryDeallocator deallocator);

void DoublyLinkedListDestroy(DoublyLinkedList *list);

DoublyLinkedListNode *DoublyLinkedListNodeCreate(const void *const data,
                                                 NodeMemoryAllocator const allocator);

void DoublyLinkedListNodeDestroy(const DoublyLinkedList *const list,
                                 DoublyLinkedListNode **node);

void DoublyLinkedListInsertAtHead(DoublyLinkedList *const list,
                                  const void *const data);

void DoublyLinkedListInsertAtTail(DoublyLinkedList *const list,
                                  const void *const data);

void DoublyLinkedListInsertBeforeNode(DoublyLinkedList *const list,
                                      DoublyLinkedListNode *node,
                                      void *data);

void DoublyLinkedListInsertAfterNode(DoublyLinkedList *const list,
                                     DoublyLinkedListNode *node,
                                     void *data);

void DoublyLinkedListRemoveNode(DoublyLinkedList *const list,
                                DoublyLinkedListNode **pointer_to_node_pointer);

#endif /* SRC_UTILS_DOUBLYLINKEDLIST_H_ */
