/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#ifndef SRC_UTILS_DOUBLYLINKEDLIST_H_
#define SRC_UTILS_DOUBLYLINKEDLIST_H_

/**
 * @file doublelinkedlist.h
 *
 * The public interface for a reference type doubly linked list
 */

typedef struct doubly_linked_list_node DoublyLinkedListNode;

typedef DoublyLinkedListNode * (*NodeMemoryAllocator)();

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

DoublyLinkedListNode *DoublyLinkedListNodeCreate(void *data,
                                                 NodeMemoryAllocator allocator);

void DoublyLinkedListNodeDestroy(DoublyLinkedList *list,
                                 DoublyLinkedListNode **node);

void DoublyLinkedListInsertAtHead(DoublyLinkedList *const list,
                                  void *data);

void DoublyLinkedListInsertAtTail(DoublyLinkedList *const list,
                                  void *data);

void DoublyLinkedListInsertBeforeNode(DoublyLinkedList *const list,
                                      DoublyLinkedListNode *node,
                                      void *data);

void DoublyLinkedListInsertAfterNode(DoublyLinkedList *const list,
                                     DoublyLinkedListNode *node,
                                     void *data);

#endif /* SRC_UTILS_DOUBLYLINKEDLIST_H_ */
