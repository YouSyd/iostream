#include <stdio.h>
#include <process.h>

#include "iostream.h"
#pragma comment(lib,"iostream.lib")

typedef struct _STRUCT_TESTX_ {
    char buffer[256];
    int  value;
}RTextX,*pTextX;
int addr_pass_test(void* lpParam);

unsigned __stdcall io_input(LPVOID lParam);
unsigned __stdcall io_output(LPVOID lParam);

unsigned __stdcall io_input(LPVOID lParam)
{
    pIOStream io=(pIOStream)lParam;
    char message[256]="";
    DWORD threadid=0;
    int thread_sequence_no=0;
    SYSTEMTIME st={0};
    
    if(io==NULL) return -1;
    threadid=GetCurrentThreadId();  
    while(thread_sequence_no<1024*200) {
        GetLocalTime(&st);
        memset(message,0x00,sizeof(message));
        sprintf(message,
                "[THREAD ID:0x%08X-%06d][TIME:%02d%02d%02d %02d:%02d:%02d.%03d]\n",
                threadid,thread_sequence_no,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
        {
            //printf("%s",message);
        }
        if(io->push_io(io,message,strlen(message))<0) {
            printf("[IO INPUT ERROR/THREAD ID:0x%08X-%06d].\n",threadid,thread_sequence_no);
            break;
        }
        thread_sequence_no++;
        Sleep(1);
    }
    if(thread_sequence_no>=1024*2) {
        EnterCriticalSection(&io->io_lock);
        if(io->stream_index>0) io->export_mem2file(io,NULL,0);
        LeaveCriticalSection(&io->io_lock);
        printf("[THREAD ID:0x%08X-%06d]DONE.\n",threadid,thread_sequence_no);
    }
    return 0;
}

unsigned __stdcall io_output(LPVOID lParam)
{
    pIOStream io=(pIOStream)lParam;
    //FILE* file=NULL;
    char stream_out[256+1]="";
    DWORD threadid=0;
    int io_count=0;
    
    if(io==NULL) return -1;
    threadid=GetCurrentThreadId();
    //if(NULL==(file=fopen("output_test.txt","a+"))) return -1;
    //Sleep(100);
    while(TRUE) {
        int result=0;
        memset(stream_out,0x00,sizeof(stream_out));
        if((result=io->pull_io(io,stream_out,256))<0) {
            printf("[IO OUTPUT ERROR/THREAD ID:0x%08X].\n",threadid);
            break;
        }
        else {
            //printf("%s",stream_out);
            //fwrite(stream_out,sizeof(char),result,file);
            //fflush(file);
            if(result==0) printf("[IO OUTPUT(%08d) CLEAN/THREAD ID:0x%08X].\n",io_count,threadid);
            io_count+=result;
        }
    }
    
    //if(file) fclose(file);
    printf("[IO OUTPUT Bytes:%d]\n",io_count);
    return 0;
}

int io_jobschedule()
{
    HANDLE io_handle[4]={NULL};
    DWORD threadid[4]={0};
    int index=0;
    pIOStream io=malloc_iostream(NULL);
    
    if(io==NULL) {
        printf("Initial iostream failed.\n");
        return -1;
    }
    
    while(index<sizeof(io_handle)/sizeof(HANDLE)-1){
        io_handle[index]=(HANDLE)_beginthreadex(NULL,0,io_input,io,0,(unsigned int*)&threadid[index]);
        index++;
    }
    io_handle[index]=(HANDLE)_beginthreadex(NULL,0,io_output,io,0,(unsigned int*)&threadid[index]);
    WaitForMultipleObjects(sizeof(io_handle)/sizeof(HANDLE),io_handle,TRUE,INFINITE);
    
    free_iostream(io);
    return 0;
}

int addr_pass_test(void* lpParam)
{
    RTextX** p=(pTextX*)lpParam;
    pTextX addr=NULL;
    
    printf("inside function-call/param addr:%08X\n",p);
    addr=(pTextX)malloc(sizeof(RTextX));
    strcpy(addr->buffer,"what a fuck?");
    addr->value=42;
    //(*lpParam)=addr;
    
    return 0;
}

void main()
{
    //pTextX p=NULL;
    //pTextX* addr_p=&p;
    //
    //printf("plt addr:%08X\n",addr_p);
    //addr_pass_test(addr_p);
    //if(p) printf("struct{%s,%d}\n",p->buffer,p->value);
    //
    //if(p) free(p);
    //    
    //{
    //    int listcount=10;
    //    int filename_itemsize=256;
    //    char** filenamelist=NULL;
    //    
    //    filenamelist=(char**)malloc(sizeof(char*)*listcount);
    //    for(int index=0;index<listcount;index++) {
    //        *(filenamelist+index)=(char*)malloc(sizeof(char)*filename_itemsize);
    //        memset(*(filenamelist+index),0x00,filename_itemsize);
    //        sprintf(*(filenamelist+index),"file index:%d-(%d.extension.)",index,index);
    //    }
    //    
    //    {
    //        int count=_msize(filenamelist)/sizeof(char*);
    //        int file_index=0;
    //        
    //        do{
    //            printf("[%08X]:%s\n",filenamelist+file_index,*(filenamelist+file_index));
    //
    //            free(*(filenamelist+file_index));
    //            *(filenamelist+file_index)=NULL;
    //            
    //        }while(++file_index<count);
    //        
    //        printf("[%08X]\n",filenamelist);
    //        
    //        free(filenamelist);
    //        filenamelist=NULL;            
    //    }
    //    
    //}
    
    io_jobschedule();
}
