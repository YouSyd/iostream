#include "iostream.h"

pIOStream malloc_iostream(void* lpstream) //自己调自己，应该会有运行时异常，后面调
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
    stream->file_read_index=0;//文件流读取的指针位置
    
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
    if(io->stream_index+size>=STREAM_MEMMAXSIZE) {//内存流爆了
        //检查当前文件尺寸+mem尺寸。
        int curr_filesize=0;
        int pre_filewritesize=0;
        int write_bytes=0;
        if(io->file_index>=0) {
            if(*(io->stream_filelist+io->file_index)!=NULL) curr_filesize=ftell(*(io->stream_filelist+io->file_index));
        }
        
        //无文件或者文件已满，需要创建新文件
        if(io->file_index<0||curr_memsize>=STREAM_FILEMAXSIZE-curr_filesize) {
            
            if(curr_memsize>=STREAM_FILEMAXSIZE-curr_filesize&&io->file_index>=0) {
                //当前流+待写入流 撑爆了 当前流的极限+当前文件的极限，
                // 所以将当前流+ 待写入流的部分 写满 当前文件
                if(io->stream_index>0) {
                    pre_filewritesize=(STREAM_FILEMAXSIZE-curr_filesize);
                    write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
                    if(write_bytes != io->stream_index) {
                        printf("[0x%08X][0-%d]写入失败(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
                    }
                    write_bytes=fwrite((char*)mem,sizeof(char),pre_filewritesize,*(io->stream_filelist+io->file_index));
                    if(write_bytes != pre_filewritesize) {
                        printf("[0x%08X][1-%d]写入失败(%d/%d)\n",GetCurrentThreadId(),GetLastError(),pre_filewritesize,write_bytes);
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
        
        //打开文件      
        if(*(io->stream_filelist+io->file_index)==NULL) {
            char file_name[256]="";
            //此命名有问题，需要保证连接唯一性，后期调整 20190819
            sprintf(file_name,"%s_%d.dat",IO_STREAM_FILETITLE,io->file_index);
            memcpy(*(io->stream_filenamelist+io->file_index),file_name,sizeof(file_name));
            *(io->stream_filelist+io->file_index)=fopen(*(io->stream_filenamelist+io->file_index),"ab+");
            if(*(io->stream_filelist+io->file_index)==NULL) return -1;
        }
        
        //写入文件
        if(io->stream_index>0) {
            write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
            if(write_bytes!=io->stream_index) {
                printf("[0x%08X][2-%d]写入失败(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
            }
            io->stream_index=0;
        }
        write_bytes=fwrite((char*)mem+pre_filewritesize,sizeof(char),size-pre_filewritesize,*(io->stream_filelist+io->file_index));
        if(write_bytes!=size-pre_filewritesize) {
            printf("[0x%08X][3-%d]写入失败(%d/%d)\n",GetCurrentThreadId(),GetLastError(),size-pre_filewritesize,write_bytes);
        }
        fflush(*(io->stream_filelist+io->file_index));
        io->stream_index=0;
    }
    else {
        //内存还未写满，强制写入文件        
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
                //此命名有问题，需要保证连接唯一性，后期调整 20190819
                sprintf(file_name,"%s_%d.dat",IO_STREAM_FILETITLE,io->file_index);
                memcpy(*(io->stream_filenamelist+io->file_index),file_name,sizeof(file_name));
                *(io->stream_filelist+io->file_index)=fopen(*(io->stream_filenamelist+io->file_index),"ab+");
                if(*(io->stream_filelist+io->file_index)==NULL) return -1;
            }
            
            write_bytes=fwrite(io->stream_mem,sizeof(char),io->stream_index,*(io->stream_filelist+io->file_index));
            if(write_bytes!=io->stream_index) {
                printf("[0x%08X][4-%d]写入失败(%d/%d)\n",GetCurrentThreadId(),GetLastError(),io->stream_index,write_bytes);
            }
            fflush(*(io->stream_filelist+io->file_index));
            io->stream_index=0;
        }
    }
    
    return 0;
}

int __stdcall new_file(void* stream)
{
    //日，创建但是不打开
    //创建新文件，但是队列应该是能够循环利用，已经释放的应该可以使用
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    if(io->stream_filelist==NULL) {
        io->stream_filelist=(FILE**)malloc(sizeof(FILE*));
        if(io->stream_filelist==NULL) return -1;
        else {
            io->file_index=0;
            *(io->stream_filelist+io->file_index)=NULL;//空文件
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
  -------|扩流----------------------|写文件--------------------
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
只有读取的时候存在删除文件流的情形，删完后文件流序列重新排序
所以逻辑上永远只会删除第一个文件。即此处的file_index永远等于0
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
        
        //删除文件
        //strcpy(file_name,*(io->stream_filenamelist+file_index));
        if(!DeleteFile(*(io->stream_filenamelist+file_index))) {
            io->file_read_index=0;//刷新读取文件指针位置
            io->file_index--;
            return io_file_sequence(io);
        }
        else return -1;
    }
    
    //重新处理空洞的，重新填充，调整file_index.
    //delete_file filelist的设计存在问题，会影响file_index...
    //逻辑上，先创建的文件会先删除.得自己处理队列
    //if(file) fclose(file);
    
    //后面想想处理
    //...
    
    return 0;
}

int __stdcall free_iostream(void* stream) //调用会存在问题，自己删自己...问题同上，后面调
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
    //IO本身的释放
    //free(io);
    
    return 0;
}

int __stdcall writestream(void* stream,void* mem,int size)
{
    pIOStream io=(pIOStream)stream;
    int io_count=0;//实际写入尺寸
    
    if(io==NULL) return -1;
    /*
    1、检查当前的流内不能装?
    1.1能装下：直接写入流；
    1.2不能：检查流程能不能扩增；
    1.2.1能扩：扩增流；
    1.3写入，检查余量
    1.3.1余量大于0：刷新内存，继续检查
    1.4余量为0：结束
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
                //内存拷贝
                memcpy(io->stream_mem+io->stream_index,((char*)mem)+io_count,mem_avaliable);
                io->stream_index+=mem_avaliable;
                io_count += mem_avaliable;  
            }
            break;
            case 1:
            {
                //io_count指针不动，只行扩增内存
                if(io->expend_stream_mem(io)<0) {
                    LeaveCriticalSection(&io->io_lock);
                    return -1;
                }
            }
            break;
            case 2:
            {
                //文件拷贝
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
分析写入的流程：先写内存流，满了写入文件流，满了再创建新的文件流
那么读的顺序就是一样：
1、有没有文件流？
   没有：读内存流，内存流不够读？不够读就不够读，就这么多了
   有：读文件流，文件流不够读？先释放文件，再读另外一个文件，文件都读完了，仍然不够读，再读内存流。
 */
int __stdcall readstream(void* stream,void* mem,int size)
{
    pIOStream io=(pIOStream)stream;
    if(io==NULL) return -1;
    
    //加内存锁
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
        //要从第一个文件中取
        int read_bytes=0;
        int io_count=0;
        int fileindex=0;
        int file_count=_msize(io->stream_filelist)/sizeof(FILE*);
        while(io_count<size&&fileindex<file_count&&(*(io->stream_filelist+fileindex)!=NULL)) {
            int file_writeof=ftell(*(io->stream_filelist+fileindex));//写指针,写入的时候移动，若读取，需归位
            int file_size=file_writeof;
            fseek(*(io->stream_filelist+fileindex),io->file_read_index,SEEK_SET);//移动至读取指针
            int file_readeof=ftell(*(io->stream_filelist+fileindex));//读指针，额外记录读指针位置，读的时候手工调整读指针位置
            file_size=file_size-file_readeof;//文件可读的尺寸
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
                //删除文件
                io->delete_streamfile(io,fileindex);
                io->file_read_index=0;//下一个文件未读取，位置为0
            }
            else {
                read_bytes=fread(((char*)mem)+io_count,sizeof(char),size-io_count,*(io->stream_filelist+fileindex));
                if(read_bytes!=size-io_count) {
                    {printf("[read stream/2]fread %d/%d,error:%d\n",size-io_count,read_bytes,GetLastError());}
                }
                io_count +=size-io_count;
                io->file_read_index=ftell(*(io->stream_filelist+fileindex));
                
                fseek(*(io->stream_filelist+fileindex),file_writeof,SEEK_SET);//恢复写指针
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
                //解锁
                LeaveCriticalSection(&io->io_lock);
                return io_count;
            }
            else {
                memcpy(((char*)mem)+io_count,io->stream_mem,size-io_count);
                for(int index=0;index<io->stream_index-(size-io_count);index++) io->stream_mem[index]=io->stream_mem[size-io_count];
                io->stream_index -=size-io_count;
                //解锁
                LeaveCriticalSection(&io->io_lock);
                return size;
            }
        }
    }
    else {
        if(size<mem_avaliable_count) {
            memcpy(mem,io->stream_mem,size);//还没写入文件就被取走了
            io->stream_index-=size;//
            //解内存锁
            LeaveCriticalSection(&io->io_lock);
            return size;
        }
        else {
            if(mem_avaliable_count>0) {
                memcpy(mem,io->stream_mem,mem_avaliable_count);
                io->stream_index=0;
            }
            else {
                //缓冲全部读完
                Sleep(10);
            }
            
            //解锁内存
            LeaveCriticalSection(&io->io_lock);
            return mem_avaliable_count;
        }
    }
    LeaveCriticalSection(&io->io_lock);
    return 0;
}