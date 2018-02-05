/*******************************************************************************
 * Copyright (c) 2017, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/

#include <CppUTest/TestHarness.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include <doublylinkedlist.h>
}

TEST_GROUP(DoublyLinkedList) {

};

const size_t kNodesAmount = 5;

static DoublyLinkedListNode nodes[kNodesAmount] = { 0 };

DoublyLinkedListNode *CallocAllocator() {
  return (DoublyLinkedListNode *) calloc( 1, sizeof( DoublyLinkedListNode) );
}

void CallocDeallocator(DoublyLinkedListNode **node) {
  free(*node);
  *node = NULL;
}

DoublyLinkedListNode *ArrayAllocator() {
  for(size_t i = 0; i < kNodesAmount; ++i) {
    if(nodes[i].previous == NULL && nodes[i].next == NULL && nodes[i].data ==
       NULL) {
      return &nodes[i];
    }
  }
  return NULL;
}

void ArrayFree(DoublyLinkedListNode **node) {
  if(*node != NULL) {
    memset( *node, 0, sizeof(DoublyLinkedListNode) );
    *node = NULL;
  }
}

TEST(DoublyLinkedList, CallocAllocatorCreateTest) {
  int test_data = 8;
  DoublyLinkedListNode *node = DoublyLinkedListNodeCreate(&test_data,
                                                          CallocAllocator);
  CHECK_EQUAL( test_data, *( (int *)node->data ) );
  CallocDeallocator(&node);
}

TEST(DoublyLinkedList, CallocFreeTest) {
  int test_data = 8;
  DoublyLinkedListNode *node = DoublyLinkedListNodeCreate(&test_data,
                                                          CallocAllocator);
  CallocDeallocator(&node);
  POINTERS_EQUAL(NULL, node);
}

TEST(DoublyLinkedList, ArrayAllocatorCreateTest) {
  int test_data = 8;
  DoublyLinkedListNode *node = DoublyLinkedListNodeCreate(&test_data,
                                                          ArrayAllocator);
  CHECK_EQUAL( test_data, *( (int *)node->data ) );
  ArrayFree(&node);
}

TEST(DoublyLinkedList, ArrayAllocatorDeleteTest) {
  int test_data = 8;
  DoublyLinkedListNode *node = DoublyLinkedListNodeCreate(&test_data,
                                                          ArrayAllocator);
  DoublyLinkedListNode *assigned_array_slot = node;
  ArrayFree(&node);
  CHECK_EQUAL(8, test_data);
  POINTERS_EQUAL(NULL, assigned_array_slot->data);
  POINTERS_EQUAL(NULL, assigned_array_slot->previous);
  POINTERS_EQUAL(NULL, assigned_array_slot->next);
  POINTERS_EQUAL(NULL, node);
}

TEST(DoublyLinkedList, InitializeList) {
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
  POINTERS_EQUAL(CallocAllocator, list.allocator);
  POINTERS_EQUAL(CallocDeallocator, list.deallocator);
}

TEST(DoublyLinkedList, CheckDestroyListCleansInteralVaribles) {
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
  POINTERS_EQUAL(NULL, list.allocator);
  POINTERS_EQUAL(NULL, list.deallocator);
}

TEST(DoublyLinkedList, InsertFirstAtHead) {
  int test_data = 42;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data);
  CHECK_EQUAL( 42, *(int *)(list.first->data) );
  CHECK_EQUAL( 42, *(int *)(list.last->data) );
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
}

TEST(DoublyLinkedList, InsertSecondAtHead) {
  int test_data = 42;
  int test_data_2 = 42 * 2;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  CHECK_EQUAL( 84, *(int *)(list.first->data) );
  CHECK_EQUAL( 42, *(int *)(list.last->data) );
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
}

TEST(DoublyLinkedList, CheckDestroyListRemovesAllNodes) {
  int test_data = 42;
  int test_data_2 = 84;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
}

TEST(DoublyLinkedList, InsertFirstAtTail) {
  int test_data = 42;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtTail(&list, &test_data);
  CHECK_EQUAL( 42, *(int *)(list.first->data) );
  CHECK_EQUAL( 42, *(int *)(list.last->data) );
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
}

TEST(DoublyLinkedList, InsertSecondAtTail) {
  int test_data = 42;
  int test_data_2 = 84;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtTail(&list, &test_data);
  DoublyLinkedListInsertAtTail(&list, &test_data_2);
  CHECK_EQUAL( 42, *(int *)(list.first->data) );
  CHECK_EQUAL( 84, *(int *)(list.last->data) );
  DoublyLinkedListDestroy(&list);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
}

TEST(DoublyLinkedList, InsertAfterNode) {
  int test_data_1 = 2;
  int test_data_2 = 4;
  int test_data_3 = 8;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data_1);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListInsertAfterNode(&list, list.first, &test_data_3);
  CHECK_EQUAL( 8, *( (int *)list.first->next->data ) )
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, InsertAfterLastNode) {
  int test_data_1 = 2;
  int test_data_2 = 4;
  int test_data_3 = 8;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data_1);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListInsertAfterNode(&list, list.last, &test_data_3);
  CHECK_EQUAL( 8, *( (int *)list.last->data ) )
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, InsertBeforeNode) {
  int test_data_1 = 2;
  int test_data_2 = 4;
  int test_data_3 = 8;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data_1);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListInsertBeforeNode(&list, list.last, &test_data_3);
  CHECK_EQUAL( 8, *( (int *)list.last->previous->data ) )
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, InsertBeforeFirstNode) {
  int test_data_1 = 2;
  int test_data_2 = 4;
  int test_data_3 = 8;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data_1);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListInsertBeforeNode(&list, list.first, &test_data_3);
  CHECK_EQUAL( 8, *( (int *)list.first->data ) )
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, RemoveFirstElementInList) {
  int test_data_1 = 2;
  int test_data_2 = 4;
  int test_data_3 = 8;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtTail(&list, &test_data_1);
  DoublyLinkedListInsertAtTail(&list, &test_data_2);
  DoublyLinkedListInsertAtTail(&list, &test_data_3);
  DoublyLinkedListNode *node_to_be_deleted = list.first;
  DoublyLinkedListRemoveNode(&list, &node_to_be_deleted);
  CHECK_EQUAL(2, test_data_1);
  CHECK_EQUAL(4, *( (int *)list.first->data ) );
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, RemoveFirstElementInOtherwiseEmptyList) {
  int test_data_1 = 2;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtTail(&list, &test_data_1);
  DoublyLinkedListNode *node_to_be_deleted = list.first;
  DoublyLinkedListRemoveNode(&list, &node_to_be_deleted);
  CHECK_EQUAL(2, test_data_1);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, RemoveLastElementInOtherwiseEmptyList) {
  int test_data_1 = 2;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtTail(&list, &test_data_1);
  DoublyLinkedListNode *node_to_be_deleted = list.last;
  DoublyLinkedListRemoveNode(&list, &node_to_be_deleted);
  CHECK_EQUAL(2, test_data_1);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
  DoublyLinkedListDestroy(&list);
}

TEST(DoublyLinkedList, CheckDeleteAllNodesResultsInEmptyList) {
  int test_data = 42;
  int test_data_2 = 84;
  DoublyLinkedList list;
  DoublyLinkedListInitialize(&list, CallocAllocator, CallocDeallocator);
  DoublyLinkedListInsertAtHead(&list, &test_data);
  DoublyLinkedListInsertAtHead(&list, &test_data_2);
  DoublyLinkedListNode *node_to_delete = list.first;
  DoublyLinkedListRemoveNode(&list, &node_to_delete);
  POINTERS_EQUAL(NULL, node_to_delete);
  node_to_delete = list.first;
  DoublyLinkedListRemoveNode(&list, &node_to_delete);
  POINTERS_EQUAL(NULL, list.first);
  POINTERS_EQUAL(NULL, list.last);
  DoublyLinkedListDestroy(&list);
}
