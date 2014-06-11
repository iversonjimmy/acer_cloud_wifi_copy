/*
  Copyright (c) 2009 Dave Gamble

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

// Found from the site: http://www.json.org  (cJSON)
// http://sourceforge.net/projects/cjson/
// (Added to source June 17, 2011)

#ifndef cJSON2__h
#define cJSON2__h

#include <stddef.h>
#include <vplex_plat.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* cJSON Types: */
#define cJSON2_False 0
#define cJSON2_True 1
#define cJSON2_NULL 2
#define cJSON2_Number 3
#define cJSON2_String 4
#define cJSON2_Array 5
#define cJSON2_Object 6
	
#define cJSON2_IsReference 256

/* The cJSON structure: */
typedef struct cJSON2 {
	struct cJSON2 *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct cJSON2 *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

	int type;					/* The type of the item, as above. */

	char *valuestring;			/* The item's string, if type==cJSON_String */
	s64 valueint;				/* The item's number, if type==cJSON_Number */
	double valuedouble;			/* The item's number, if type==cJSON_Number */

	char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} cJSON2;

typedef struct cJSON2_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON2_Hooks;

/* Supply malloc, realloc and free functions to cJSON */
extern void cJSON2_InitHooks(cJSON2_Hooks* hooks);


/* Supply a block of JSON, and this returns a cJSON object you can interrogate. Call cJSON_Delete when finished. */
extern cJSON2 *cJSON2_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. Free the char* when finished. */
extern char  *cJSON2_Print(cJSON2 *item);
/* Render a cJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern char  *cJSON2_PrintUnformatted(cJSON2 *item);
/* Delete a cJSON entity and all subentities. */
extern void   cJSON2_Delete(cJSON2 *c);

/* Returns the number of items in an array (or object). */
extern int	  cJSON2_GetArraySize(cJSON2 *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern cJSON2 *cJSON2_GetArrayItem(cJSON2 *array,int item);
/* Get item "string" from object. Case insensitive. */
extern cJSON2 *cJSON2_GetObjectItem(cJSON2 *object,const char *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
extern const char *cJSON2_GetErrorPtr();

/* These calls create a cJSON item of the appropriate type. */
extern cJSON2 *cJSON2_CreateNull();
extern cJSON2 *cJSON2_CreateTrue();
extern cJSON2 *cJSON2_CreateFalse();
extern cJSON2 *cJSON2_CreateBool(int b);
extern cJSON2 *cJSON2_CreateNumber(double num);
extern cJSON2 *cJSON2_CreateString(const char *string);
extern cJSON2 *cJSON2_CreateArray();
extern cJSON2 *cJSON2_CreateObject();

/* These utilities create an Array of count items. */
extern cJSON2 *cJSON2_CreateIntArray(int *numbers,int count);
extern cJSON2 *cJSON2_CreateFloatArray(float *numbers,int count);
extern cJSON2 *cJSON2_CreateDoubleArray(double *numbers,int count);
extern cJSON2 *cJSON2_CreateStringArray(const char **strings,int count);

/* Append item to the specified array/object. */
extern void cJSON2_AddItemToArray(cJSON2 *array, cJSON2 *item);
extern void	cJSON2_AddItemToObject(cJSON2 *object,const char *string,cJSON2 *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
extern void cJSON2_AddItemReferenceToArray(cJSON2 *array, cJSON2 *item);
extern void	cJSON2_AddItemReferenceToObject(cJSON2 *object,const char *string,cJSON2 *item);

/* Remove/Detatch items from Arrays/Objects. */
extern cJSON2 *cJSON2_DetachItemFromArray(cJSON2 *array,int which);
extern void   cJSON2_DeleteItemFromArray(cJSON2 *array,int which);
extern cJSON2 *cJSON2_DetachItemFromObject(cJSON2 *object,const char *string);
extern void   cJSON2_DeleteItemFromObject(cJSON2 *object,const char *string);
	
/* Update array items. */
extern void cJSON2_ReplaceItemInArray(cJSON2 *array,int which,cJSON2 *newitem);
extern void cJSON2_ReplaceItemInObject(cJSON2 *object,const char *string,cJSON2 *newitem);

#define cJSON2_AddNullToObject(object,name)	cJSON2_AddItemToObject(object, name, cJSON2_CreateNull())
#define cJSON2_AddTrueToObject(object,name)	cJSON2_AddItemToObject(object, name, cJSON2_CreateTrue())
#define cJSON2_AddFalseToObject(object,name)		cJSON2_AddItemToObject(object, name, cJSON2_CreateFalse())
#define cJSON2_AddNumberToObject(object,name,n)	cJSON2_AddItemToObject(object, name, cJSON2_CreateNumber(n))
#define cJSON2_AddStringToObject(object,name,s)	cJSON2_AddItemToObject(object, name, cJSON2_CreateString(s))

#ifdef __cplusplus
}
#endif

#endif
