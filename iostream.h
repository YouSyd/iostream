/*
 ΪIOCP ���� stream �ṹ
 ���ܣ������ڴ������ļ����ṹ�������ṩͳһ�ӿڶ��������޷�Ķ�ȡ��д��ȷ��ʹ��ܡ�
       ���� stream ��Ҫ֧����������������Ч�ͷš�ʹ�ù�������Ҫ������Դ���ģ�ͬ����Ҫ��������ʹ�ã����ʡ�д�룩Ч�ʡ�
 
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
    char* stream_mem;//�ڴ滺��
    int stream_mem_maxsize;//�ڴ���󻺴�
    FILE** stream_filelist;//�ļ�����
    char** stream_filenamelist;
    int stream_file_maxsize;//�ļ���󻺴棻
    int stream_index;//���ĵ�ǰ����λ��
    int file_index;  //�ļ�����
    int file_read_index;//��ȡ����ʱ�򣬶�ȡ�ļ�������λ��//��ȡ��һ��һ��������������˳��ģ�
                        //ͬһʱ��ֻ��һ��ȡ��������Զ��file_index=0��λ��ȡ��ȡ����ɾ����������������
    //��
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