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
 Project Group: NetID1, NetID2, NetID3
 
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
 #define PTRSIZE 4
/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
typedef struct freeBlock{
	struct freeBlock* nextFree ;
}free_blk;

// typedef struct listArray{
// 	free_blk* fb32 = NULL;
// 	free_blk* fb64 = NULL;
// 	free_blk* fb128 = NULL;
// 	free_blk* fb256 = NULL;
// 	free_blk* fb512 = NULL;
// 	free_blk* fb1024 = NULL;
// 	free_blk* fb2048 = NULL;
// 	free_blk* fb4096 = NULL;
// }list_array;

struct free_blk* freeArray[8];
/************Global Variables*********************************************/
kma_page_t* g_pageList = NULL;
list_array freeList;
/************Function Prototypes******************************************/
void cutPage(kma_size_t, kma_page_t*);
void roundBlkSize(kma_size_t)
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void* kma_malloc(kma_size_t size)
{	
	// if (g_pageList==NULL){
	// 	g_pageList = get_page();
	// 	// add a pointer to the page structure at the beginning of the page
	// 	*((kma_page_t**)g_pageList->ptr) = g_pageList;
	// 	if ((size + sizeof(kma_page_t*)) > g_pageList->size)
	// 	{ // requested size too large
	// 	  free_page(g_pageList);
	// 	  return NULL;
	// 	}
	// 	cutPage(size, g_pageList);                            //need to maintain pagelist
	// 	g_pageList->num_in_use=1;
	// }
	int index = roundBlkSize(size);
	int blkSize = BASE * (2 ^ index);
	if(freeList[index] == NULL) cutPage(size, get_page());
	free_blk* newBlk = freeList[index];
	freeList[index] = freeList[index]->nextFree;
	*((free_blk**)newBlk->nextFree) = (void*)size;           // need check (void*)
	return newBlk + PTRSIZE;
}

void cutPage(kma_size_t size, kma_page_t* currentPage)
{
	int index = roundBlkSize(size);
	int blkSize = BASE * (2 ^ index);
	freeList[index] = (free_blk*) (g_pageList->ptr + sizeof(kma_page_t*)); 
	free_blk* curr = freeList[index];
	for(int i=0; i < (PAGESIZE - sizeof(kma_page_t*)) / blkSize - 1; i++)
	{
		*((free_blk**)curr->nextFree) = (free_blk*)(curr + blkSize);
		curr += blkSize;
	}
	*((free_blk**)curr->nextFree) = NULL;
	return freeList[index];
}

void roundBlkSize(kma_size_t size)
{
	int blkSize = BASE;
	int totalSize = size+4;
	int index = 0;
	while(totalSize>blkSize){
		blkSize *= 2;
		index++;
	}
	return index;
}

void kma_free(void* ptr, kma_size_t size)
{
  ;
}

#endif // KMA_P2FL
