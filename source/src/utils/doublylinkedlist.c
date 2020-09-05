/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include "doublylinkedlist.h"

#include "opener_user_conf.h"
#include <stdio.h>  // Needed to define NULL
#include <assert.h>

void DoublyLinkedListInitialize(DoublyLinkedList *list,
                                NodeMemoryAllocator allocator,
                                NodeMemoryDeallocator deallocator) {
  list->first = NULL;
  list->last = NULL;
  list->allocator = allocator;
  list->deallocator = deallocator;
}

void DoublyLinkedListDestroy(DoublyLinkedList *list) {
  DoublyLinkedListNode *iterator = list->first;
  while(NULL != iterator) {
    DoublyLinkedListNode *to_delete = iterator;
    iterator = iterator->next;
    DoublyLinkedListNodeDestroy(list, &to_delete);
  }

  list->first = NULL;
  list->last = NULL;
  list->allocator = NULL;
  list->deallocator = NULL;
}

DoublyLinkedListNode *DoublyLinkedListNodeCreate(
  const void *const data,
  NodeMemoryAllocator const
  allocator) {
  DoublyLinkedListNode *new_node = (DoublyLinkedListNode *)allocator();
  new_node->data = (void *)data;
  return new_node;
}

void DoublyLinkedListNodeDestroy(const DoublyLinkedList *const list,
                                 DoublyLinkedListNode **node) {
  OPENER_ASSERT(list->deallocator != NULL);
  list->deallocator(node);
}

void DoublyLinkedListInsertAtHead(DoublyLinkedList *const list,
                                  const void *const data) {
  OPENER_ASSERT(list->allocator != NULL);
  DoublyLinkedListNode *new_node = DoublyLinkedListNodeCreate(data,
                                                              list->allocator);
  if(NULL == list->first) {
    list->first = new_node;
    list->last = new_node;
  } else {
    new_node->next = list->first;
    list->first->previous = new_node;
    list->first = new_node;
  }
}

void DoublyLinkedListInsertAtTail(DoublyLinkedList *const list,
                                  const void *const data) {
  OPENER_ASSERT(list->allocator != NULL);
  DoublyLinkedListNode *new_node = DoublyLinkedListNodeCreate(data,
                                                              list->allocator);
  if(NULL == list->last) {
    list->first = new_node;
    list->last = new_node;
  } else {
    new_node->previous = list->last;
    list->last->next = new_node;
    list->last = new_node;
  }
}

void DoublyLinkedListInsertBeforeNode(DoublyLinkedList *const list,
                                      DoublyLinkedListNode *node,
                                      void *data) {
  OPENER_ASSERT(list->allocator != NULL);
  if(list->first == node) {
    DoublyLinkedListInsertAtHead(list, data);
  } else {
    DoublyLinkedListNode *new_node = DoublyLinkedListNodeCreate(data,
                                                                list->allocator);
    new_node->previous = node->previous;
    new_node->next = node;
    node->previous = new_node;
    new_node->previous->next = new_node;
  }
}

void DoublyLinkedListInsertAfterNode(DoublyLinkedList *const list,
                                     DoublyLinkedListNode *node,
                                     void *data) {
  OPENER_ASSERT(list->allocator != NULL);
  if(list->last == node) {
    DoublyLinkedListInsertAtTail(list, data);
  } else {
    DoublyLinkedListNode *new_node = DoublyLinkedListNodeCreate(data,
                                                                list->allocator);
    new_node->previous = node;
    new_node->next = node->next;
    node->next->previous = new_node;
    node->next = new_node;
  }
}

void DoublyLinkedListRemoveNode(DoublyLinkedList *const list,
                                DoublyLinkedListNode **pointer_to_node_pointer)
{
  DoublyLinkedListNode *node = *pointer_to_node_pointer;
  DoublyLinkedListNode *previous = node->previous;
  DoublyLinkedListNode *next = node->next;

  if (node == list->first && node == list->last) {
    list->first = NULL;
    list->last = NULL;
  } else {
    if(node == list->first) {
      list->first = next;
    }
    if(node == list->last) {
      list->last = previous;
    }
    if(NULL != previous) {
      previous->next = next;
    }
    if(NULL != next) {
      next->previous = previous;
    }

  }


  DoublyLinkedListNodeDestroy(list, pointer_to_node_pointer);
}
