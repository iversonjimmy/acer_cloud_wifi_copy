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

#include <stdio.h>
#include <stdlib.h>
#include "cJSON2.h"

/* Parse text to JSON, then render back to text, and print! */
static void doit(char *text)
{
	char *out;cJSON2 *json;
	
	json=cJSON2_Parse(text);
	if (!json) {printf("Error before: [%s]\n",cJSON2_GetErrorPtr());}
	else
	{
		out=cJSON2_Print(json);
		cJSON2_Delete(json);
		printf("%s\n",out);
		free(out);
	}
        printf("================================================\n");
}

/* Read a file, parse, render back, etc. */
/*
static void dofile(char *filename)
{
	int rc;
	FILE *f=fopen(filename,"rb");fseek(f,0,SEEK_END);long len=ftell(f);fseek(f,0,SEEK_SET);
	char *data=malloc(len+1);rc=fread(data,1,len,f);fclose(f);
	doit(data);
	free(data);
}
*/

/* Used by some code below as an example datatype. */
struct record {const char *precision;double lat,lon;const char *address,*city,*state,*zip,*country; };

/* Create a bunch of objects as demonstration. */
static void create_objects()
{
	cJSON2 *root,*fmt,*img,*thm,*fld;char *out;int i;	/* declare a few. */

	/* Here we construct some JSON standards, from the JSON site. */
	
	/* Our "Video" datatype: */
	root=cJSON2_CreateObject();	
	cJSON2_AddItemToObject(root, "name", cJSON2_CreateString("Jack (\"Bee\") Nimble"));
	cJSON2_AddItemToObject(root, "format", fmt=cJSON2_CreateObject());
	cJSON2_AddStringToObject(fmt,"type",		"rect");
	cJSON2_AddNumberToObject(fmt,"width",		1920);
	cJSON2_AddNumberToObject(fmt,"height",		1080);
	cJSON2_AddFalseToObject (fmt,"interlace");
	cJSON2_AddNumberToObject(fmt,"frame rate",	24);
	
	out=cJSON2_Print(root);	cJSON2_Delete(root);	printf("%s\n",out);	free(out);	/* Print to text, Delete the cJSON2, print it, release the string. */

	/* Our "days of the week" array: */
	const char *strings[7]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	root=cJSON2_CreateStringArray(strings,7);

	out=cJSON2_Print(root);	cJSON2_Delete(root);	printf("%s\n",out);	free(out);

	/* Our matrix: */
	int numbers[3][3]={{0,-1,0},{1,0,0},{0,0,1}};
	root=cJSON2_CreateArray();
	for (i=0;i<3;i++) cJSON2_AddItemToArray(root,cJSON2_CreateIntArray(numbers[i],3));

/*	cJSON2_ReplaceItemInArray(root,1,cJSON2_CreateString("Replacement")); */
	
	out=cJSON2_Print(root);	cJSON2_Delete(root);	printf("%s\n",out);	free(out);


	/* Our "gallery" item: */
	int ids[4]={116,943,234,38793};
	root=cJSON2_CreateObject();
	cJSON2_AddItemToObject(root, "Image", img=cJSON2_CreateObject());
	cJSON2_AddNumberToObject(img,"Width",800);
	cJSON2_AddNumberToObject(img,"Height",600);
	cJSON2_AddStringToObject(img,"Title","View from 15th Floor");
	cJSON2_AddItemToObject(img, "Thumbnail", thm=cJSON2_CreateObject());
	cJSON2_AddStringToObject(thm, "Url", "http:/*www.example.com/image/481989943");
	cJSON2_AddNumberToObject(thm,"Height",125);
	cJSON2_AddStringToObject(thm,"Width","100");
	cJSON2_AddItemToObject(img,"IDs", cJSON2_CreateIntArray(ids,4));

	out=cJSON2_Print(root);	cJSON2_Delete(root);	printf("%s\n",out);	free(out);

	/* Our array of "records": */
	struct record fields[2]={
		{"zip",37.7668,-1.223959e+2,"","SAN FRANCISCO","CA","94107","US"},
		{"zip",37.371991,-1.22026e+2,"","SUNNYVALE","CA","94085","US"}};

	root=cJSON2_CreateArray();
	for (i=0;i<2;i++)
	{
		cJSON2_AddItemToArray(root,fld=cJSON2_CreateObject());
		cJSON2_AddStringToObject(fld, "precision", fields[i].precision);
		cJSON2_AddNumberToObject(fld, "Latitude", fields[i].lat);
		cJSON2_AddNumberToObject(fld, "Longitude", fields[i].lon);
		cJSON2_AddStringToObject(fld, "Address", fields[i].address);
		cJSON2_AddStringToObject(fld, "City", fields[i].city);
		cJSON2_AddStringToObject(fld, "State", fields[i].state);
		cJSON2_AddStringToObject(fld, "Zip", fields[i].zip);
		cJSON2_AddStringToObject(fld, "Country", fields[i].country);
	}
	
/*	cJSON2_ReplaceItemInObject(cJSON2_GetArrayItem(root,1),"City",cJSON2_CreateIntArray(ids,4)); */
	
	out=cJSON2_Print(root);	cJSON2_Delete(root);	printf("%s\n",out);	free(out);

}

int main (int argc, const char * argv[]) {
	/* a bunch of json: */
	char text1[]="{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}";	
	char text2[]="[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	char text3[]="[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	char text4[]="{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}";
	char text5[]="[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]";
	char text6[]="[{\"n\":\"docs\",\"f\":[{\"n\":\"accounts\",\"f\":[{\"n\":\"bigAccount.doc\"},{\"n\":\"rushOrder.doc\"}]},{\"n\":\"projects\",\"f\":[{\"n\":\"apple\",\"f\":[{\"n\":\"schedule.xls\"},{\"n\":\"overview.doc\"}]},{\"n\":\"banana\",\"f\":[{\"n\":\"schedule.xls\"},{\"n\":\"overview.doc\"},{\"n\":\"contract.pdf\"}]}]}]}]";
	char text7[]="[{\"n\":\"docs\"},{\"n\":\"photos\"},{\"n\":\"music\",\"f\":[{\"n\":\"Abbey Road\"},{\"n\":\"Rumors\"},{\"n\":\"Happy Birthday.mp3\"}]}]";
	char text8[]="[]";
	char text9[]="[{\"n\":\"directory\",\"f\":[]}]";
        //	char text10[]="";  An error, emtpy string is invalid JSON.

	/* Process each json textblock by parsing, then rebuilding: */
	doit(text1);
	doit(text2);	
	doit(text3);
	doit(text4);
	doit(text5);
	doit(text6);
	doit(text7);
	doit(text8);
	doit(text9);
        //	doit(text10);

	/* Parse standard testfiles: */
/*	dofile("../../tests/test1"); */
/*	dofile("../../tests/test2"); */
/*	dofile("../../tests/test3"); */
/*	dofile("../../tests/test4"); */
/*	dofile("../../tests/test5"); */

	/* Now some samplecode for building objects concisely: */
	create_objects();
	
	return 0;
}
