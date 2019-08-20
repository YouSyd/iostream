/*
 为IOCP 定制 stream 结构
 功能：管理内存流和文件流结构，对外提供统一接口对流进行无缝的读取、写入等访问功能。
       定制 stream 需要支持流的自增长，有效释放。使用过程中需要减少资源消耗，同是需要考虑流的使用（访问、写入）效率。
 
 */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <windows.h>

#define IO_STREAM_FILETITLE "IO_STREAM_TMP"

#define STREAM_MEMINITIALSIZE 4*1024     // 4K
#define STREAM_MEMMAXSIZE 32*1024        // 32K
#define STREAM_FILEMAXSIZE 1024*1024*200 // 200M

typedef void*(__stdcall *call_back_iointial_proc)(void*);
typedef int(__stdcall *call_back_iostream_proc)(void*);
typedef int(__stdcall *call_back_iofile_proc)(void*,int);
typedef int(__stdcall *call_back_iostream_io_proc)(void*,void*,int);
typedef int(__stdcall *call_back_memcheck_proc)(void*,int,int*);
typedef struct _STRUCT_IOSTREAM_ {
    char* stream_mem;//内存缓存
    int stream_mem_maxsize;//内存最大缓存
    FILE** stream_filelist;//文件缓存
    char** stream_filenamelist;
    int stream_file_maxsize;//文件最大缓存；
    int stream_index;//流的当前索引位置
    int file_index;  //文件索引
    int file_read_index;//读取流的时候，读取文件的流的位置//读取是一条一条读，所以他是顺序的，
                        //同一时间只有一个取，而且永远从file_index=0的位置取，取完了删除，并且重新排序
    //锁
    CRITICAL_SECTION io_lock;
    
    call_back_iostream_proc expend_stream_mem;
    call_back_iostream_proc new_streamfile;
    call_back_iofile_proc delete_streamfile;
    call_back_iostream_proc free_stream;
    call_back_memcheck_proc check_mem;
    call_back_iostream_io_proc export_mem2file;
    call_back_iostream_io_proc push_io;
    call_back_iostream_io_proc pull_io;
}RIOStream,*pIOStream;

pIOStream malloc_iostream(void* lpstream);
int __stdcall expend_mem(void* stream);
int __stdcall export_mem2file(void* stream,void* mem,int size);
int __stdcall new_file(void* stream);
int __stdcall delete_file(void* stream,int file_index);
int __stdcall free_iostream(void* stream);
int __stdcall mem_check(void* stream,int in_size,int* access_size);
int __stdcall writestream(void* stream,void* mem,int size);
int __stdcall readstream(void* stream,void* mem,int size);

int io_file_sequence(pIOStream io);