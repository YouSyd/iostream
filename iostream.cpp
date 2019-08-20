#include "iostream.h"

pIOStream malloc_iostream(void* lpstream) //�Լ����Լ���Ӧ�û�������ʱ�쳣�������
{
    pIOStream stream=NULL;
    stream=(pIOStream)malloc(sizeof(RIOStream));
    if(stream==NULL) return NULL;
    stream->stream_mem=(char*)malloc(sizeof(char)*STREAM_MEMINITIALSIZE);
    if(stream->stream_mem==NULL) {
        free(stream);
        return NULL;
    }
    stream->stream_mem_maxsize=STREAM_MEMMAXSIZE;
    stream->stream_filelist=NULL;
    stream->stream_file_maxsize=STREAM_FILEMAXSIZE;
    stream->stream_filenamelist=NULL;
    stream->stream_index=0;
    stream->file_index=-1;
    stream->file_read_index=0;//�ļ�����ȡ��ָ��λ��
    
    stream->expend_stream_mem=expend_mem;
    stream->export_mem2file=export_mem2file;
    stream->new_streamfile=new_file;
    stream->delete_streamfile=delete_file;
    stream->free_stream=free_iostream;
    stream->check_mem=mem_check;
    stream->push_io=writestream;
    stream->pull_io=readstream;
    
    InitializeCriticalSection(&stream->io_lock);
    
    return stream;
}

int __stdcall expend_mem(void* stream)
{
    int curr_memsize=0;
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    curr_memsize=_msize(io->stream_mem)/sizeof(char);
    if(curr_memsize < STREAM_MEMMAXSIZE) {
        io->stream_mem=(char*)realloc(io->stream_mem,curr_memsize<<1);
        if(io->stream_mem==NULL) return -1;
    }
    else if(curr_memsize>=STREAM_MEMMAXSIZE) {
        return 0;
    }
    return 0;
}

int __stdcall export_mem2file(void* stream,void* mem,int size)
{
    int curr_memsize=0;
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    curr_memsize=_msize(io->stream_mem)/sizeof(char);
    if(io->stream_index+size>=STREAM_MEMMAXSIZE) {//�ڴ�������
        //��鵱ǰ�ļ��ߴ�+mem�ߴ硣
        int curr_filesize=0;
        int pre_filewritesize=0;
        int write_bytes=0;
        if(io->file_index>=0) {
            if(*(io->stream_filelist+io->file_index)!=NULL) curr_filesize=ftell(*(io->stream_filelist+io->file_index));
        }
        
        //���ļ������ļ���������Ҫ�������ļ�
        if(io->file_index<0||curr_memsize>=STREAM_FILEMAXSIZE-curr_filesize) {
            
            if(curr_memsize>=STREAM_FILEMAXSIZE-curr_filesize&&io->file_index>=0) {
                //��ǰ��+��д���� �ű��� ��ǰ���ļ���+��ǰ�ļ��ļ��ޣ�
                // ���Խ���ǰ��+ ��д�����Ĳ��� д�� ��ǰ�ļ�
                if(io->stream_index>0) {
                    pre_filewritesize=(STREAM_FILEMAXSIZE-curr_filesize);
                    write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
                    if(write_bytes != io->stream_index) {
                        printf("[0x%08X][0-%d]д��ʧ��(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
                    }
                    write_bytes=fwrite((char*)mem,sizeof(char),pre_filewritesize,*(io->stream_filelist+io->file_index));
                    if(write_bytes != pre_filewritesize) {
                        printf("[0x%08X][1-%d]д��ʧ��(%d/%d)\n",GetCurrentThreadId(),GetLastError(),pre_filewritesize,write_bytes);
                    }
                    fflush(*(io->stream_filelist+io->file_index));
                }
                io->stream_index=0;                
            }
            
            if(io->new_streamfile(io)<0) {
                {printf("io new file failed\n");}
                return -1;
            }
        }
        
        //���ļ�      
        if(*(io->stream_filelist+io->file_index)==NULL) {
            char file_name[256]="";
            //�����������⣬��Ҫ��֤����Ψһ�ԣ����ڵ��� 20190819
            sprintf(file_name,"%s_%d.dat",IO_STREAM_FILETITLE,io->file_index);
            memcpy(*(io->stream_filenamelist+io->file_index),file_name,sizeof(file_name));
            *(io->stream_filelist+io->file_index)=fopen(*(io->stream_filenamelist+io->file_index),"ab+");
            if(*(io->stream_filelist+io->file_index)==NULL) return -1;
        }
        
        //д���ļ�
        if(io->stream_index>0) {
            write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
            if(write_bytes!=io->stream_index) {
                printf("[0x%08X][2-%d]д��ʧ��(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
            }
            io->stream_index=0;
        }
        write_bytes=fwrite((char*)mem+pre_filewritesize,sizeof(char),size-pre_filewritesize,*(io->stream_filelist+io->file_index));
        if(write_bytes!=size-pre_filewritesize) {
            printf("[0x%08X][3-%d]д��ʧ��(%d/%d)\n",GetCurrentThreadId(),GetLastError(),size-pre_filewritesize,write_bytes);
        }
        fflush(*(io->stream_filelist+io->file_index));
        io->stream_index=0;
    }
    else {
        //�ڴ滹δд����ǿ��д���ļ�        
        int write_bytes=0;
        
        if(size==0&&io->stream_index>0) {           
            if(io->file_index<0) {
                if(io->new_streamfile(io)<0) {
                    {printf("io new file failed\n");}
                    return -1;
                }
            }
            
            if(*(io->stream_filelist+io->file_index)==NULL) {
                char file_name[256]="";
                //�����������⣬��Ҫ��֤����Ψһ�ԣ����ڵ��� 20190819
                sprintf(file_name,"%s_%d.dat",IO_STREAM_FILETITLE,io->file_index);
                memcpy(*(io->stream_filenamelist+io->file_index),file_name,sizeof(file_name));
                *(io->stream_filelist+io->file_index)=fopen(*(io->stream_filenamelist+io->file_index),"ab+");
                if(*(io->stream_filelist+io->file_index)==NULL) return -1;
            }
            
            write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
            if(write_bytes!=io->stream_index) {
                printf("[0x%08X][4-%d]д��ʧ��(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
            }
            fflush(*(io->stream_filelist+io->file_index));
            io->stream_index=0;
        }
    }
    
    return 0;
}

int __stdcall new_file(void* stream)
{
    //�գ��������ǲ���
    //�������ļ������Ƕ���Ӧ�����ܹ�ѭ�����ã��Ѿ��ͷŵ�Ӧ�ÿ���ʹ��
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    if(io->stream_filelist==NULL) {
        io->stream_filelist=(FILE**)malloc(sizeof(FILE*));
        if(io->stream_filelist==NULL) return -1;
        else {
            io->file_index=0;
            *(io->stream_filelist+io->file_index)=NULL;//���ļ�
            io->stream_filenamelist=(char**)malloc(sizeof(char*));
            if(io->stream_filenamelist==NULL) return -1;
            *(io->stream_filenamelist+io->file_index)=(char*)malloc(sizeof(char)*256);
            if(*(io->stream_filenamelist+io->file_index)==NULL) return -1;
            memset(*(io->stream_filenamelist+io->file_index),0x00,256);
        }
    }
    else {
        int curr_filelistcount=_msize(io->stream_filelist)/sizeof(FILE*);
        if(curr_filelistcount<=io->file_index+1) {
            io->stream_filelist=(FILE**)realloc(io->stream_filelist,(curr_filelistcount+1)*sizeof(FILE*));
            io->stream_filenamelist=(char**)realloc(io->stream_filenamelist,(curr_filelistcount+1)*sizeof(char*));
            if(io->stream_filelist==NULL) return -1;
            if(io->stream_filenamelist==NULL) return -1;
        }
        
        io->file_index+=1;
        io->stream_filelist[io->file_index]=NULL;
        if(*(io->stream_filenamelist+io->file_index)==NULL) *(io->stream_filenamelist+io->file_index)=(char*)malloc(sizeof(char)*256);
        if(*(io->stream_filenamelist+io->file_index)==NULL) return -1;
        memset(*(io->stream_filenamelist+io->file_index),0x00,256);
    }
    
    return 0;
}

int io_file_sequence(pIOStream io)
{
    if(!io) return -1;
    int filecount=_msize(io->stream_filelist)/sizeof(FILE*);
    int index=0;
    FILE* index_file=NULL;
    FILE* index_read_file=NULL;

    if(io->file_index>=0) {
        index_file=*(io->stream_filelist+io->file_index);
    }
    
    do{
        if(*(io->stream_filelist+index)==NULL) {
            for(int i=index+1;i<filecount;i++) {
                if(*(io->stream_filelist+i)) {
                    *(io->stream_filelist+index)=*(io->stream_filelist+i);
                    memset(*(io->stream_filenamelist+index),0x00,_msize(*(io->stream_filenamelist+index))/sizeof(char));
                    strcpy(*(io->stream_filenamelist+index),*(io->stream_filenamelist+i));
                    *(io->stream_filelist+i)=NULL;
                    memset(*(io->stream_filenamelist+i),0x00,_msize(*(io->stream_filenamelist+i))/sizeof(char));
                    if(index_file!=NULL&&index_file==*(io->stream_filelist+index)) {
                        io->file_index=index;
                    }
                    break;
                }
            }
        }
    }while(++index<filecount);
    
    return 0; 
}

/*
  -------curr_memsize---------------STREAM_MEMMAXSIZE----------
  -------|����----------------------|д�ļ�--------------------
 */
int __stdcall mem_check(void* stream,int in_size,int* access_size)
{
    int curr_memsize=0;
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    curr_memsize=_msize(io->stream_mem)/sizeof(char);
    if(io->stream_index+in_size<curr_memsize) {
        *access_size=in_size;
        return 0;
    }
    else if(io->stream_index+in_size>=curr_memsize && curr_memsize < STREAM_MEMMAXSIZE) {
        *access_size=in_size;
        return 1;
    }
    else/* if(io->stream_index+in_size>=STREAM_MEMMAXSIZE) */{
        *access_size=in_size;
        return 2;
    }
    return 0;
}

/*
ֻ�ж�ȡ��ʱ�����ɾ���ļ��������Σ�ɾ����ļ���������������
�����߼�����Զֻ��ɾ����һ���ļ������˴���file_index��Զ����0
 */
int __stdcall delete_file(void* stream,int file_index)
{
    FILE* file_plt=NULL;
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    file_plt=*(io->stream_filelist+file_index); 
    if(file_plt) {
        fclose(file_plt);
        *(io->stream_filelist+file_index)=NULL;
        
        //ɾ���ļ�
        //strcpy(file_name,*(io->stream_filenamelist+file_index));
        if(!DeleteFile(*(io->stream_filenamelist+file_index))) {
            io->file_read_index=0;//ˢ�¶�ȡ�ļ�ָ��λ��
            io->file_index--;
            return io_file_sequence(io);
        }
        else return -1;
    }
    
    //���´���ն��ģ�������䣬����file_index.
    //delete_file filelist����ƴ������⣬��Ӱ��file_index...
    //�߼��ϣ��ȴ������ļ�����ɾ��.���Լ��������
    //if(file) fclose(file);
    
    //�������봦��
    //...
    
    return 0;
}

int __stdcall free_iostream(void* stream) //���û�������⣬�Լ�ɾ�Լ�...����ͬ�ϣ������
{
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    if(io->stream_mem) {
        free(io->stream_mem);
        io->stream_mem=NULL;
    }
    
    if(io->stream_filelist) {
        int count=_msize(io->stream_filelist)/sizeof(FILE*);
        int index=0;
        while(index < count) {
            if(*(io->stream_filelist+index)) {
                fclose(*(io->stream_filelist+index));
                *(io->stream_filelist+index)=NULL;
            }
            if(*(io->stream_filenamelist+index)) {
                free(*(io->stream_filenamelist+index));
                *(io->stream_filenamelist+index)=NULL;
            }
            index++;
        }
        free(io->stream_filenamelist);
        io->stream_filenamelist=NULL;
        free(io->stream_filelist);
        io->stream_filelist=NULL;
    }
    DeleteCriticalSection(&io->io_lock);
    //IO������ͷ�
    //free(io);
    
    return 0;
}

int __stdcall writestream(void* stream,void* mem,int size)
{
    pIOStream io=(pIOStream)stream;
    int io_count=0;//ʵ��д��ߴ�
    
    if(io==NULL) return -1;
    /*
    1����鵱ǰ�����ڲ���װ?
    1.1��װ�£�ֱ��д������
    1.2���ܣ���������ܲ���������
    1.2.1��������������
    1.3д�룬�������
    1.3.1��������0��ˢ���ڴ棬�������
    1.4����Ϊ0������
    */
    EnterCriticalSection(&io->io_lock);
    while(io_count<size) {
        int mem_avaliable=0;
        int io_check_result=io->check_mem(io,size-io_count,&mem_avaliable);
        //{
        //    if(io_check_result!=0)
        //    printf("[0x%08X]io_check-result:%d,io->stream_index:%d,write size:%d\n",
        //           GetCurrentThreadId(),io_check_result,io->stream_index,mem_avaliable);
        //}
        switch(io_check_result) {
            case 0:
            {
                //�ڴ濽��
                memcpy(io->stream_mem+io->stream_index,((char*)mem)+io_count,mem_avaliable);
                io->stream_index+=mem_avaliable;
                io_count += mem_avaliable;  
            }
            break;
            case 1:
            {
                //io_countָ�벻����ֻ�������ڴ�
                if(io->expend_stream_mem(io)<0) {
                    LeaveCriticalSection(&io->io_lock);
                    return -1;
                }
            }
            break;
            case 2:
            {
                //�ļ�����
                int result=io->export_mem2file(io,mem,size);
                LeaveCriticalSection(&io->io_lock);
                return result;
            }
            break;
            case -1:
            {
                LeaveCriticalSection(&io->io_lock);
                return -1;
            }
            break;
        }
    }
    LeaveCriticalSection(&io->io_lock);
    return 0;
}

/*
����д������̣���д�ڴ���������д���ļ����������ٴ����µ��ļ���
��ô����˳�����һ����
1����û���ļ�����
   û�У����ڴ������ڴ������������������Ͳ�����������ô����
   �У����ļ������ļ��������������ͷ��ļ����ٶ�����һ���ļ����ļ��������ˣ���Ȼ���������ٶ��ڴ�����
 */
int __stdcall readstream(void* stream,void* mem,int size)
{
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    //���ڴ���
    EnterCriticalSection(&io->io_lock);
    int mem_avaliable_count=io->stream_index;
    //{
    //    if(io->file_index>=0&&NULL !=*(io->stream_filelist+io->file_index)) {
    //        printf("[read stream]current stream index:%d,filename(%s),file length:%d\n",
    //               io->stream_index,*(io->stream_filenamelist+io->file_index),
    //               ftell(*(io->stream_filelist+io->file_index)));
    //    }
    //}
    if(io->file_index!=-1) {
        //Ҫ�ӵ�һ���ļ���ȡ
        int read_bytes=0;
        int io_count=0;
        int fileindex=0;
        int file_count=_msize(io->stream_filelist)/sizeof(FILE*);
        while(io_count<size&&fileindex<file_count&&(*(io->stream_filelist+fileindex)!=NULL)) {
            int file_writeof=ftell(*(io->stream_filelist+fileindex));//дָ��,д���ʱ���ƶ�������ȡ�����λ
            int file_size=file_writeof;
            fseek(*(io->stream_filelist+fileindex),io->file_read_index,SEEK_SET);//�ƶ�����ȡָ��
            int file_readeof=ftell(*(io->stream_filelist+fileindex));//��ָ�룬�����¼��ָ��λ�ã�����ʱ���ֹ�������ָ��λ��
            file_size=file_size-file_readeof;//�ļ��ɶ��ĳߴ�
            //{
            //    printf("[read stream]file index:%d,write plt:%d,avaliable file read size:%d\n"
            //           "             read plt:%d(%d)\n"
            //           "             REQUEST size:%d\n",
            //           fileindex,file_writeof,file_size,file_readeof,io->file_read_index,size);
            //}
            
            
            if(file_size+io_count < size) {
                read_bytes=fread(((char*)mem)+io_count,sizeof(char),file_size,*(io->stream_filelist+fileindex));//....
                if(read_bytes!=file_size) {
                    {printf("[read stream/1]fread %d/%d,error:%d\n",file_size,read_bytes,GetLastError());}
                }
                io_count+=file_size;
                //ɾ���ļ�
                io->delete_streamfile(io,fileindex);
                io->file_read_index=0;//��һ���ļ�δ��ȡ��λ��Ϊ0
            }
            else {
                read_bytes=fread(((char*)mem)+io_count,sizeof(char),size-io_count,*(io->stream_filelist+fileindex));
                if(read_bytes!=size-io_count) {
                    {printf("[read stream/2]fread %d/%d,error:%d\n",size-io_count,read_bytes,GetLastError());}
                }
                io_count +=size-io_count;
                io->file_read_index=ftell(*(io->stream_filelist+fileindex));
                
                fseek(*(io->stream_filelist+fileindex),file_writeof,SEEK_SET);//�ָ�дָ��
                LeaveCriticalSection(&io->io_lock);
                return io_count;
            }
            
            fileindex++;
        }
        if(io_count<size) {
            if(io_count+mem_avaliable_count<size) {
                memcpy(((char*)mem)+io_count,io->stream_mem,mem_avaliable_count);
                io_count +=mem_avaliable_count;
                io->stream_index=0;
                //����
                LeaveCriticalSection(&io->io_lock);
                return io_count;
            }
            else {
                memcpy(((char*)mem)+io_count,io->stream_mem,size-io_count);
                for(int index=0;index<io->stream_index-(size-io_count);index++) io->stream_mem[index]=io->stream_mem[size-io_count];
                io->stream_index -=size-io_count;
                //����
                LeaveCriticalSection(&io->io_lock);
                return size;
            }
        }
    }
    else {
        if(size<mem_avaliable_count) {
            memcpy(mem,io->stream_mem,size);//��ûд���ļ��ͱ�ȡ����
            io->stream_index-=size;//
            //���ڴ���
            LeaveCriticalSection(&io->io_lock);
            return size;
        }
        else {
            if(mem_avaliable_count>0) {
                memcpy(mem,io->stream_mem,mem_avaliable_count);
                io->stream_index=0;
            }
            else {
                //����ȫ������
                Sleep(10);
            }
            
            //�����ڴ�
            LeaveCriticalSection(&io->io_lock);
            return mem_avaliable_count;
        }
    }
    LeaveCriticalSection(&io->io_lock);
    return 0;
}