/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the resource map algorithm
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
 Project Group: hlv624 ywb017
 
 ***************************************************************************/

#ifdef KMA_RM
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/************Private include**********************************************/
#include "kma_page.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
#define FREECOUNT 1
typedef struct freeBlock{
	struct freeBlock* nextFree ;
	int size;
}free_blk;

/************Global Variables*********************************************/
kma_page_t* g_pageList;
free_blk* freeBlkHead;
int count=0;
/************Function Prototypes******************************************/
void* findFreeBlk(kma_size_t size);
void checkForFreePage();
void deleteFreeBlk(void* pageStart);
kma_page_t* checkPage(free_blk* blk);
void Coalesce();
void addToPageList(kma_page_t* newPage);
void addToFreeBlk(free_blk* newBlk,kma_size_t size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/
void addToPageList(kma_page_t* newPage){
	kma_page_t* current;
	current = g_pageList;
	if (current==NULL){
		g_pageList = newPage;
		g_pageList -> next = NULL;
		return;
	}
	while (current->next!=NULL){
		current = current->next;
	}
	newPage->next = NULL;
	current->next = newPage;
}
void addToFreeBlk(free_blk* newBlk,kma_size_t size){
	//add new freed memory to the list
	free_blk* current;
	current = freeBlkHead;
	newBlk->size = size;

	if (freeBlkHead==NULL){
		freeBlkHead = newBlk;
		newBlk->nextFree =NULL;
		return;		
	}
	if (newBlk < current){
		newBlk->nextFree = current;
		freeBlkHead=newBlk;
	}else{
		if (current->nextFree ==NULL){
			current->nextFree = newBlk;
		}else{
			free_blk* prev=freeBlkHead;		
			current = current->nextFree;
			while (current!=NULL){
				if (newBlk<current){
					newBlk->nextFree = current;
					prev->nextFree = newBlk;
					return;
				}
				prev = current;
				current = current->nextFree;
			}
			prev->nextFree = newBlk;
			newBlk->nextFree=NULL;
		}
	}
}
void* findFreeBlk(kma_size_t size){
	if (freeBlkHead == NULL){
		printf("ERROR, findFreeBlk, freeBlkHead == NULL\n");
		exit(2);
	}
	//iterate through the freeblklist to check free spaces
	free_blk* currentBlk = freeBlkHead;
	free_blk* prev = NULL; 
	//check the frist element in the linked list
	if (currentBlk->size >= size){         //2 byte   
		freeBlkHead = currentBlk->nextFree;

		if (currentBlk->size - size >=sizeof(free_blk)-4){    //sizeof(free_blk)=16   int=8 ,free_blk*=12
			addToFreeBlk((void*)currentBlk+size, currentBlk->size - size);
		}
		return (void*)currentBlk;
	}
	currentBlk = currentBlk->nextFree;
	prev = freeBlkHead;
	while(currentBlk!=NULL){
		if (currentBlk->size >= size){
			//After found the freeblk, delete the blk in the freelist and add the rest into the freelist
			prev->nextFree = currentBlk->nextFree;
			if (currentBlk->size-size >=sizeof(free_blk)-4){
				addToFreeBlk((void*) currentBlk+size,currentBlk->size - size);				
			}
			return (void*)currentBlk;    
		}else{
			prev = currentBlk;
			currentBlk = currentBlk->nextFree;

		}
	}
	//if there is no suitable block for the size, ask for a new page.	
	kma_page_t* newPage = get_page();
	newPage->num_in_use=0;
	newPage->next = NULL;
	*((kma_page_t**)newPage->ptr) = newPage;
	if ((size + sizeof(kma_page_t*)) > newPage->size)
	{ // requested size too large
	  free_page(newPage);
	  return NULL;
	}
	addToPageList(newPage);
	int offset = sizeof(kma_page_t*) + size;
	addToFreeBlk(newPage->ptr+ offset, newPage->size-sizeof(kma_page_t*)-size);
	return newPage->ptr+sizeof(kma_page_t*);

}
void* kma_malloc(kma_size_t size)
{
	//PrintFreeList();
	if (g_pageList==NULL){
		g_pageList = get_page();
		// add a pointer to the page structure at the beginning of the page
		*((kma_page_t**)g_pageList->ptr) = g_pageList;
		if ((size + sizeof(kma_page_t*)) > g_pageList->size)
		{ // requested size too large
		  free_page(g_pageList);
		  return NULL;
		}
		freeBlkHead = (free_blk*) (g_pageList->ptr + sizeof(kma_page_t*)+ size);   
		freeBlkHead->size = g_pageList->size - sizeof(kma_page_t*)-size;
		freeBlkHead->nextFree = NULL;
		g_pageList->num_in_use=1;
		return g_pageList->ptr + sizeof(kma_page_t*);
	}
	void* newFreeBlk= findFreeBlk(size);
	kma_page_t* newBlkPage;
	newBlkPage =  (kma_page_t*)checkPage(newFreeBlk);
	if (newBlkPage!=NULL)
		newBlkPage->num_in_use+=1;
	return newFreeBlk;
}

kma_page_t* checkPage(free_blk* blk){
// walk through the pagelist and compare the address of blk  with the address of the adress
// check the blk is in which page.
// every time after free and malloc, find the page in which it located. add or minus 1 on num_in_use. 
	if (g_pageList==NULL){
		return NULL;
	}
	int size=0;
	kma_page_t* currentPage;
	void* currentSpace;
	size = g_pageList->size;
	currentPage = g_pageList;
	currentSpace = currentPage->ptr;
	while(currentPage!=NULL){
		if( currentSpace<=((void*)blk) && ((void*)blk) <= currentSpace+size){
			return currentPage;
		}
		currentPage = currentPage->next;
		if(currentPage!=NULL){
			currentSpace = currentPage->ptr;
		}
	}
	printf("Error! The blk doesn't belong to any exist pages.\n");
	exit(2);
}

void kma_free(void* ptr, kma_size_t size)
{
	Coalesce();
	if (ptr==NULL){
		return;
	}
	void* iter=ptr;
	memset(iter, 0 , size);
	kma_page_t* freeBlkPage;
	freeBlkPage =  checkPage(ptr);
	freeBlkPage->num_in_use-=1;
	if (size >= sizeof(free_blk)){
		addToFreeBlk(ptr,size);
	}
		checkForFreePage();
}
void deleteFreeBlk(void* pageStart){
	if (g_pageList==NULL){
		return;
	}
	int size; 
	size = g_pageList->size;
	free_blk* current = freeBlkHead;
	free_blk* prev = NULL;
	int flag = 0;  // set a flag to find the first node to be freeBlkHead
	while (current!=NULL){
		if((void*)current>= pageStart && ((void*)current <= pageStart+size)){
			//if the blk locate in the page we gonna free. we delete the blk
			if(flag ==1){
				prev->nextFree = current->nextFree;
				current = prev;
			}
		}else{
			if ( flag == 0 ){
				flag = 1;
				freeBlkHead = current;
			}
		}
		prev = current;
		current = current->nextFree;
	}
}
void checkForFreePage(){
	kma_page_t* current = g_pageList;
	if (current==NULL){
		return;
	}
	if(current->num_in_use == 0){
		if (current->next!=NULL){
			g_pageList= g_pageList->next;
		}else{
			g_pageList=NULL;			
		}
		deleteFreeBlk(current->ptr);
		free_page(current);
		return;
	}
	current=current->next;
	kma_page_t* prev = g_pageList;
	while(current!=NULL){
		if(current->num_in_use==0){
			prev->next = current->next;
			deleteFreeBlk(current->ptr);
			free_page(current);
			return;
		}
		prev = current;
		current=current->next;
	}
}

void Coalesce(){
	 free_blk* current;
	free_blk* ptr;
	if (freeBlkHead==NULL){
		return;
	}
	current = freeBlkHead;
	ptr = current;
	while(current->nextFree!=NULL){
	 	ptr = current;
	 	if((void*)ptr+current->size == current->nextFree){
	 	 	current->size = current->size + current->nextFree->size;
	 	 	current->nextFree = current->nextFree->nextFree;
	 	 	continue;
	 	 } 
	 	current = current->nextFree;		
	 	}
}
#endif // KMA_RM
