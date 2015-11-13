/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 *    Revision 1.3  2004/12/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 
 ************************************************************************/

/************************************************************************
 Project Group: hlv624,ywb017
 
 ***************************************************************************/

#ifdef KMA_P2FL
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

#define BASE 32
#define ARRAYSIZE 9
/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */




typedef struct freeBlock{
	struct freeBlock* nextFree;
}free_blk;



typedef struct pageLeft{
	int sizeLeft;
	void* headPtr;
}page_left;



typedef struct allHeader{
	free_blk* array[9];
	page_left pageLeftHead;
	int num_of_page_in_use;

	int num_of_malloc;
	int num_of_freed;
}all_header;





/************Global Variables*********************************************/
kma_page_t* zero_page = NULL;

/************Function Prototypes******************************************/
free_blk* findFreeBlk(kma_size_t);
int roundBlkSize(kma_size_t);
void addToArray(free_blk*, int);
void kma_free(void*, kma_size_t);
void PrintHead();
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{	
	if ((size + sizeof(kma_page_t*)) > PAGESIZE)
	{ // requested size too large
	  return NULL;
	}

	if(zero_page == NULL)
	{
		//initialize
		zero_page = get_page();
		*((kma_page_t**)zero_page->ptr) = zero_page;
		all_header* Header = (all_header*)((void*)zero_page->ptr + sizeof(kma_page_t*));
		int i;
		for(i=0;i<ARRAYSIZE;i++)
		{
			Header->array[i] = NULL;
		}		
		Header->pageLeftHead.headPtr = (void*)((void*)zero_page->ptr + sizeof(kma_page_t*) + sizeof(all_header));
		Header->pageLeftHead.sizeLeft = PAGESIZE - sizeof(kma_page_t) - sizeof(all_header);
		Header->num_of_page_in_use = 1;
		Header->num_of_freed = 0;	
		Header->num_of_malloc = 0;
	}
	all_header* Header = (all_header*)((void*)zero_page->ptr + sizeof(kma_page_t*));
	free_blk* thisBlk = findFreeBlk(size);
	Header->num_of_malloc+=1;
	return thisBlk;	
}

free_blk* findFreeBlk(kma_size_t size)
{
	all_header* Header = (all_header*)((void*)zero_page->ptr + sizeof(kma_page_t*));

	int index = roundBlkSize(size);
	int blkSize = BASE * (1<<index);

	free_blk* sizeHead = Header->array[index];
	free_blk* thisBlk;
	if(sizeHead == NULL)
	{
		page_left* pageLeftS = &(Header->pageLeftHead);
		if(blkSize > pageLeftS->sizeLeft)
		{
			int i = 0;
			int arraysize = 32;
			while(pageLeftS->sizeLeft > arraysize)
			{
				arraysize *= 2;
				i++;
			}
			if(i > 0)
			{
				i--;
				addToArray(pageLeftS->headPtr, i);
			}
			kma_page_t* new_page = get_page();
			*((kma_page_t**)new_page->ptr) = new_page;		
			Header->pageLeftHead.headPtr = (void*)((void*)new_page->ptr + sizeof(kma_page_t*));
			Header->pageLeftHead.sizeLeft = PAGESIZE - sizeof(kma_page_t*);			
			Header->num_of_page_in_use += 1;
		}
		thisBlk = (free_blk*)(pageLeftS->headPtr);
		pageLeftS->headPtr = (void*)((void*)(pageLeftS->headPtr) + blkSize);
		pageLeftS->sizeLeft -= blkSize;
		
	}else{
		thisBlk = sizeHead;
		Header->array[index] = sizeHead->nextFree;
		
	}
	return thisBlk;
}

int roundBlkSize(kma_size_t size)
{
	int blkSize = BASE;
	int totalSize = size + sizeof(free_blk*);
	int index = 0;
	while(totalSize>blkSize){
		blkSize *= 2;
		index++;
	}
	return index;
}

void addToArray(free_blk* freedBlk, int index)
{
	all_header* Header = (all_header*)((void*)zero_page->ptr + sizeof(kma_page_t*));
	free_blk* sizeHead = Header->array[index];
	freedBlk->nextFree = sizeHead;
	Header->array[index] = freedBlk;
	return;
}

void kma_free(void* ptr, kma_size_t size)
{
	all_header* Header = (all_header*)((void*)zero_page->ptr + sizeof(kma_page_t*));
	free_blk* blkToFree = (free_blk*)ptr;
	int index = roundBlkSize(size);
	addToArray(blkToFree, index);
	Header->num_of_freed +=1;
	if(Header->num_of_malloc == Header->num_of_freed)
	{
		void* last_page_addr = BASEADDR(Header->pageLeftHead.headPtr);
		int count = Header->num_of_page_in_use;
		while(count>0)
		{
			free_page(*((kma_page_t**)last_page_addr));
			last_page_addr = (void*)((void*)last_page_addr - PAGESIZE);
			count --;
		}
		zero_page = NULL;		
	}
	return;
}

#endif // KMA_P2FL
