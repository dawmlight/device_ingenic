#ifndef __UTIL_H__
#define __UTIL_H__

//#include <fcntl.h>
#include <unistd.h>
//#include <stdint.h>
//#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>
#define DEFSIZE 128

using namespace std;

namespace android {
class Util {
 public:
     static ssize_t getline (int fd, char **line){
	     int i = 0, err = 0;
	     char ch = 0;
	     size_t size = DEFSIZE;
	     char *tmp = NULL;
	     
	     if (NULL == *line) {
		     *line = (char*)malloc(size);
		     if (!*line) {
			     return -ENOMEM;
		     }
	     }
	     memset(*line, 0, size);
	     
	     for (i = 0; ; i++) {
		     err = read(fd, &ch, 1);
		     if (!err) { // EOF
				break;
		     } else if (err < 0) { // ERROR
			     return -errno;
		     }
		     
		     if (i == (size - 2)) {
			     /* 
			      * realloc(3): 
			      * If the new size is larger than the old size, 
			      * the added memory will not be initialized.
			      
			      */
			     size *= 2;
			     tmp = (char *)malloc(size);
			     if (!tmp) {
				     return -ENOMEM;
				}
			     memset(tmp, 0, size);
			     memcpy(tmp, *line, size / 2);
			     free(*line);
			     *line = tmp;
			     tmp = NULL;
		     }
		     
		     //*((*line)++) = ch;
		     (*line)[i] = ch;
		     if ('\n' == ch) { // Line end
				(*line)[++i] = '\0';
				break;
		     }
	     }
	     
	     return i;
     }
     
     static void destroy(char **line){
	     free(*line);
	     *line = NULL;
     }

     static void strTrim(char* ptr){
	     char *p,*q;
	     for(p = ptr;*p!='\0';){
		     q = p;
		     if(*q == ' '){
			     while(*q!='\0'){
				     *q=*(q+1);
				     q++;
			     }
		     } else{  
			     p++;
		     }
	     }
     }

     static void formatSizeString(long long size, string & src) {
	     if (size == 0 || size == -1) {
		     src = "0KB";
		     return;
	     }
       
	     string suffix = "B";
	     double dSize = (double) size;
	     if (size >= 1024) {
		     suffix = "kB";
		     dSize /= 1024;
		     if (dSize >= 1024){
			     suffix = "MB";
			     dSize /= 1024;
			     if (dSize >= 1024) {
				     suffix = "GB";
				     dSize /= 1024;
			     }
		     }
	     }
	     char test[20] = "";
	     snprintf(test, 20, "%.1f",dSize);
	     string s = test;
	     src += s += suffix;
     }
 
};
}

#endif //__UTIL_H__

