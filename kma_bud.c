/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
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
 *    Revision 1.3  2004/12/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *

 ************************************************************************/
 
 /************************************************************************
 Project Group: NetID1, NetID2, NetID3
 
 ***************************************************************************/

#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/
typedef struct freeBlock{
	struct freeBlock* nextFree ;
	int size;
}free_blk;
//Header  32,64,128,256,512,1024,2048,4096
free_blk* freeListHeader[8];
kma_page_t* g_pageList;
/************Function Prototypes******************************************/
void* kma_malloc(kma_size_t size);
void kma_free(void* ptr, kma_size_t size);
/************External Declaration*****************************************/
void addToFreeList(free_blk* ptr, int size,int index);
void* findFreeBlk(int size);
void splitMem(free_blk* father,int index);
void delFromFreeList(free_blk* ptr);
void coalesce();
int roundSize(int size);
//void* findPage(ptr);
void addToPageList(kma_page_t* newPage);
void setBitMapToOne(free_blk* blkptr, int size);
void printBitMap();
void printFreeList();
//int powof2(int base, int b);
/**************Implementation***********************************************/
// int powof2(int base, int b){
// 	if(base==0){
// 		printf("base can't be zeor\n");
// 		exit(2);
// 	}
// 	int i;
// 	base<<b;
//}

void addToPageList(kma_page_t* newPage){
	kma_page_t* current;
	current = g_pageList;
	if (current==NULL){
		g_pageList = newPage;
		g_pageList->next = NULL;
		return;
	}
	while (current->next!=NULL){
		current = current->next;
	}
	newPage->next = NULL;
	current->next = newPage;
}

int findEntry(sizeRound){
	int index;
	if (sizeRound==1<<13){
		return -2;
	}
	for(index=0; index<8;index++){
		if((2<<(index+4))==sizeRound){
			return index;
		}
	}
	return -1;
}
void initializeBitMap(kma_page_t* page){
	int i;
	for (i=0;i<(256/sizeof(int));i++)
	{
		*((int*)page->ptr + i) = 0;
	}
	// for (i=0;i<(256/sizeof(int)+1);i++)
	// {
	// 	printf("%d",*((int*)page->ptr + i));
	// }
	// printf("\n");
}

void setBitMapToOne(free_blk* blkptr, int size){
	printf("------------------in setBitMapToOne--------------------------\n");
	kma_page_t* currentPage = BASEADDR(blkptr);
	int start = (int)(((void*)blkptr - (void*)currentPage)/32);
	int end = start + size/32;
	int i;	
	//debug code--------begin-----------------------------------------
	printf("before set bitmap  currentPage = %p blkptr = %p \n", currentPage, blkptr);

// 	for (i=0;i<(256/sizeof(int)+1);i++)
// 	{
// 		printf("[%d]",i);
// //		printf("%d",*((int*)currentPage->ptr + i));
// 	}
// 	printf("\n");
	//debug code-------end--------------------------------------------
	// from start address to end address , set them to 1
	for (i=0; i<(end-start);i++){	
		*((int*)blkptr + i )= 1; 		
	}
	//debug code--------begin-----------------------------------------
	// printf("after set bitmap\n");
	// for (i=0;i<(256/sizeof(int)+1);i++)
	// {
	// 	printf("%d",*((int*)currentPage->ptr + i));
	// }
	// printf("\n");
	//debug code-------end--------------------------------------------
}
// void setBitMapToZero(free_blk* blkptr, int size){
// 	kma_page_t* currentPage = BASEADDR(blkptr);
// 	int start = int(((void*)blkptr - (void*)currentPage)/32);
// 	int end = start + size/32;
// 	int i;	
// 	// from start address to end address , set them to 1
// 	blkptr = 

// }

void splitMem(free_blk* father,int index){
	printf("\n-----------------------------in splitMem-----------------------------\n");
	void* temptr;
	int newspaceSize;
	temptr = (void*)father;
	father = father->nextFree;
	newspaceSize = 2<<(index+3);
	printf("new size===============================%d\n",newspaceSize);

	addToFreeList((free_blk*)temptr,newspaceSize,index-1);
	addToFreeList((free_blk*)(temptr + newspaceSize),newspaceSize,index-1);
	//printFreeList();
}

void initializePage(kma_page_t* page){
//put a 256 bit 0 at the begining of the page
//4096 4096
	printf("\n[---------------------Begin initializePage-----------------------------]\n");
	int index;
	int spaceSize = (page->size)/2;
	index = findEntry(spaceSize);
	printf("New page struct begin at : %p,  page->ptr->nextFree = %p\n ", page,((free_blk*)page->ptr)->nextFree);

	addToFreeList((free_blk*)page->ptr,spaceSize,index);
	addToFreeList((free_blk*)(page->ptr + spaceSize),spaceSize,index);
	printFreeList();
	printf("\n[---------------------END initializePage-----------------------------]\n");

}
void* findFreeBlk(int size){
	int index = 0;
	int sizeRound;
	void* res;
	printf("[------------------------Begin findFreeBlk %d --------------------------]\n", size);
	sizeRound = roundSize(size);
	index =	findEntry(sizeRound);
	if( index == -1){
		printf("CAN'T FIND entry!!\n");
		exit(2);
	}else if (index == -2){
		//the size > 4096 , return a whole page without bitmap;
		kma_page_t* wholePage = get_page();
		//Add the page to pagelist
		addToPageList(wholePage);
		return wholePage->ptr;
	}
	if(freeListHeader[index]==NULL){

		for(index=index;index< 8;index++){
			printf("index = %d \n", index);
			if(freeListHeader[index]!=NULL){
				printf("222222222222222222222\n");
				free_blk* temp = freeListHeader[index];
				freeListHeader[index] = freeListHeader[index]->nextFree;
				splitMem(temp,index);
				printFreeList();
				res = findFreeBlk(size);
				setBitMapToOne(res,size);
				return res;
			}			
		}
		//if all the entries are NULL, get a new page
		printf("Get new page!!!!!\n");
		kma_page_t* page;
		page = get_page();
		addToPageList(page);
		if (size > page->size)
		{ // requested size too large
			free_page(page);
			return NULL;
		}
		if(size > 4096){
			// return a whole page without initializing
		}
		//find a space to store the bitmap,split the new page.
		//should be the start 256 bit of the space
		initializePage(page);
		findFreeBlk(256);
		initializeBitMap(page);
		res = findFreeBlk(size);
		return res; 
	}else{
		void* ptr;
		ptr = freeListHeader[index];
		freeListHeader[index] = freeListHeader[index] -> nextFree;
		return ptr;
	}

}

void* kma_malloc(kma_size_t size)
{

	void* res;
	res = findFreeBlk(size);
 	printFreeList();
  return res;
}

void kma_free(void* ptr, kma_size_t size)
{

}
void addToFreeList(free_blk* ptr, int size, int index){
	printf("++++++[ In addtofreelist ] ptr: %p,  size: %d   index = %d \n",ptr, size, index);
	free_blk* current;
	free_blk* prev;
	current = freeListHeader[index];
	if (current==NULL){
		printf("    1 header[%d] == NULL, newblk %d added to %p\n", index, size, ptr);
		freeListHeader[index] = ptr;
		freeListHeader[index]->nextFree=NULL;
		freeListHeader[index]->size = size;

		printf("freeListHeader point to: %p \n",freeListHeader[index]);
//		printFreeList();
		return;
	}
	if (ptr < current){
		printf("    2  newblk %d added to header[%d] addr : %p\n",  size,index, ptr);
		freeListHeader[index] = ptr;
		freeListHeader[index]->nextFree = current;
		freeListHeader[index]->size = size;		
//		printFreeList();
		return;
	}
	prev = current;
	current = current -> nextFree;
	if(current == NULL){
		prev->nextFree = ptr;
		ptr->nextFree = NULL;
		ptr->size = size; 
	}
	while(current!=NULL){
//		printf("hahahaha\n");
		if( ptr< current){
			printf("   3  newblk %d added to header[%d] addr : %p\n",  size,index, ptr);			
			prev -> nextFree = ptr;
			ptr -> nextFree = current;
			ptr -> size = size;
		}
		prev = current;
		current = current ->nextFree;

	}
	printFreeList(); 
	printf("------------------------Leaving addToFreeList-----------------------\n");
}
void delFromFreeList(free_blk* ptr){

}
void coalesce(){

}
int roundSize(int size){
	int base = 32 ;
	int tempNum = base;
	if (size > base){
		while(size > tempNum){
			tempNum = tempNum * 2;
		}
		return tempNum;	
	}else{
		return base;
	}
}
//void* findPage(ptr){
//
//}
void printFreeList(){
	printf("--------------------printfreelist------------------------------\n");
	int i = 0;
	free_blk* current;
	for (i=0;i<8;i++){
		int j =0;
		printf("%d %d pointer %p\n",i,j,freeListHeader[i]);
		current = freeListHeader[i];	
		while(current!=NULL){
			printf("[%d][%d] size = %d  addr: %p\n",i,j,current->size, current);
			current = current->nextFree;
			j++;
		}
	}
	printf("---------------------------end --------------------------------\n");

}

//free
//coalescing

#endif // KMA_BUD
