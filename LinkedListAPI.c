/****************************************
* CIS 2750 Fall 2017                    *
* Assigment 2                           *
* Adjowa Karikari(0914271)              *
* This file contains the implementation *
* of the Linked List API                *
*****************************************/

#include "LinkedListAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


List initializeList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second))
{
	List makeList;

	makeList.head = NULL;
	makeList.tail = NULL;
	makeList.length = 0;
	/* initlizes list with the print data function */
	makeList.printData = printFunction;
	/* initilizes list with the data function */
	makeList.deleteData = deleteFunction;
	/* initilizes list with the compare function */
	makeList.compare = compareFunction;
	/* returns the list struct value */
	return makeList;
}//end of initializeList

Node* initializeNode(void* data)
{
	/* malloc memory for the node */
	Node *initNode = malloc(sizeof(Node));

	/* Error trap */
	if(initNode == NULL)
	{
		printf("List error.\n");
		return NULL;
	}//end of if

	initNode -> data = (void*) data;
	initNode -> previous = NULL;
	initNode -> next = NULL;
	/* returns node of data to be added to the list */
	return initNode;
}//end of initializeNode

void insertFront(List* list, void* toBeAdded)
{
	Node *insertNodeFront = initializeNode(toBeAdded);

	/*Error trap */
	if(list == NULL)
	{
		printf("List errror.\n");
	}//end of f
	else
	{
		if(list -> head == NULL)
		{
			list -> head = insertNodeFront;
			list -> tail = insertNodeFront;
		}//end of if
		else
		{
			list -> head -> previous = insertNodeFront;
			insertNodeFront -> next = list -> head;
			list -> head = insertNodeFront;
		}//end of else
		list->length++;
	}//end of else
}//end of insertFront

void insertBack(List* list, void* toBeAdded)
{
	Node *insertNodeBack = initializeNode(toBeAdded);

	/* error trap */
	if(list == NULL)
	{
		printf("List error.\n");
	}//end of if
	else
	{
		if(list -> head == NULL)
		{
			list -> head = insertNodeBack;
			list -> tail = insertNodeBack;
		}//end of if
		else
		{
			list -> tail -> next = insertNodeBack;
			insertNodeBack -> previous = list -> tail;
			list -> tail = insertNodeBack;
		}//end of else
		list->length++;
	}//end of else
}//end of insertBack

void clearList(List* list)
{
	if(list == NULL)
	{
		printf("List error.\n");
		return;
	}//end of if
	Node * nodePtr = list-> head;
	Node * next;

	/* error trap */

	while(nodePtr != NULL)
	{
		next = nodePtr -> next;
		list->deleteData(nodePtr->data);
		free(nodePtr);
		nodePtr = next;
	}//end of whileew

	list -> head = NULL;
	list -> tail = NULL;
	list -> length = 0;
}//end of clearList

void insertSorted(List* list, void* toBeAdded)
{
	if (!list)
	{
		return;
	}
	Node * nodePtr = list-> head;
	Node* newNode = malloc(sizeof(Node));
	newNode->data = toBeAdded;

	while(nodePtr != NULL)
	{
		if (list->compare(nodePtr->data, toBeAdded) > 0)
		{
			// insert before nodePtr
			Node* prev = nodePtr->previous;
			if (prev)
			{
				prev->next = newNode;
				nodePtr->previous = newNode;
				newNode->previous = prev;
				newNode->next = nodePtr;
			}
			else
			{
				list->head = newNode;
				newNode->next = nodePtr;
				newNode->previous = NULL;
				nodePtr->previous = newNode;				
			}
			list->length++;
			return;
		}
		nodePtr = nodePtr->next;
	}
	insertBack(list, toBeAdded);
}//end of insertSorted

void* deleteDataFromList(List* list, void* toBeDeleted)
{
	/* Error Trap */
	if(list == NULL || toBeDeleted == NULL)
	{
		printf("List Error.\n");
		return NULL;
	}//end of if
	Node * nodePtr = list-> head;
	while(nodePtr != NULL)
	{
		if (!list->compare(nodePtr->data, toBeDeleted))
		{
			Node* prev = nodePtr->previous;
			Node* next = nodePtr->next;
			if (next)
			{
				next->previous = prev;
			}
			else
			{
				list->tail = prev;
			}
			if (prev)
			{
				prev->next = next;
			}
			else
			{
				list->head = next;
			}
			void* res = nodePtr->data;
			free(nodePtr);
			list->length--;
			return res;
		}
		nodePtr = nodePtr->next;
	}
	return NULL;
}//end of deleteDataFromList

void* getFromFront(List list)
{
	return list.head ? list.head->data : NULL;
}//end of getFromFront

void* getFromBack(List list)
{
	return list.tail ? list.tail->data : NULL;
}//end of getFromBack

ListIterator createIterator(List list)
{
	ListIterator itr;
	itr.current = list.head;

	return itr;
}//end of createIterator

void* nextElement(ListIterator* iter)
{
	Node *node = iter -> current;
	if (node) {
        iter->current = node->next;
        return node->data;
	} else {
	    return NULL;
	}
}//end of nextElement

char* toString(List list)
{
		printf ("toString::1\n");
	if (!list.length)
	{
		char* res = malloc(64);
		strcpy(res, "empty");
		res[7] = 0;
		printf ("return empty\n");
		return res;
	}
	Node * temp = list.head;

	int size = 0;

	while(temp != NULL)
	{
		char* data = list.printData(temp->data);
		printf ("add data: %s\n", data);
		size += strlen(data);
		free(data);
		temp = temp -> next;
	}

	temp = list.head;

	char *str = malloc(sizeof(char)*size + 1);
	str[0] = 0;

	printf ("total length: %d\n", size);

	while(temp != NULL)
	{
		char* data = list.printData(temp->data);
		strcat(str, data);
		free(data);
		temp = temp->next;
	}
	return str;
}//end of toString

int getLength(List list)
{
    return list.length;
}//End of getLength

void* findElement(List list, bool (*customCompare)(const void* first,const void* second), const void* searchRecord)
{
    ListIterator iter = createIterator(list);
    for (void* data = nextElement(&iter); data; data = nextElement(&iter))
    {
        if (customCompare(data, searchRecord))
        {
            return data;
        }//end of if
    }//end of for
    return NULL;
}//end of findElement


//END OF LINKEDLISTAPI
