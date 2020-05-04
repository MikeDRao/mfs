
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

FILE *fp;
char NextDir[12];
struct __attribute__((__packed__)) DirectoryEntry 
{
    char     DIR_Name[11];
    uint8_t  DIR_Attr;
    uint8_t  Unused1[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t  Unused2[4];
    uint16_t DIR_FirstClusterLow;
    uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];

char     BS_OEMName[8];
int16_t  BPB_BytesPerSec;
int8_t   BPB_SecPerClus;
int16_t  BPB_RsvdSecCnt;
int8_t   BPB_NumFATs;
int16_t  BPB_RootEntCnt;
char     BS_VolLab[11];
int32_t  BPB_FATSz32;
int32_t  BPB_RootClus;

int32_t  CWD; // Current Working Directory
int32_t  RootDirSectors = 0;
int32_t  FirstDataSector = 0;
int32_t  FirstSectorofCluster = 0;
int ext = 0;

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case white space
                                // will seperate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

/*
 *Function    : LBAToOffset
 *Parameters  : The current sector number that points to a block of data
 *Returns     : The value of the address for that block of data
 *Descriotion : Finds the starting address of a block of data given the sector number
 *corresponding to that data block.
*/ 
int LBAToOffset(int32_t sector)
{
    return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec); 
}

/*
 * Function    : NextLB
 * Parameters  : Logical block address
 * Returns     : The next logical block address
 * Description : Given a logical block address, look up into the first FAT and retrun the 
 * logical block address of the block in the file. If there is no further blocks then it returns -1
*/ 
int16_t NextLB( uint32_t sector )
{
    uint32_t FATAddress = ( BPB_BytesPerSec * BPB_RsvdSecCnt ) + ( sector * 4 );
    int16_t val;
    fseek( fp, FATAddress, SEEK_SET );
    fread( &val, 2, 1, fp );
    return val;
}

/*
 * Function    : FormatDirName
 * Parameters  : char *dir_name
 * Returns     : void
 * Description : Correctly fomats the name of the directory and file extentsion
*/ 
void FormatDirName(char *dir_name)
{
    ext = 0;
    char expanded_name[12];
    memset( expanded_name, ' ', 12 );

    char *token = strtok( dir_name, "." );

    strncpy( expanded_name, token, strlen( token ) );

    token = strtok( NULL, "." );

    if( token )
    {
        strncpy( (char*)(expanded_name+8), token, strlen(token) );
        ext = 1;
    }

    expanded_name[11] = '\0';
    int i;
    for( i = 0; i < 11; i++)
    {
        expanded_name[i] = toupper( expanded_name[i] );
    }
    strncpy(NextDir,expanded_name,12);
    return;
}

/*
 * Function    : newImage
 * Parameters  : FILE *fp, char *filename
 * Returns     : No return value (void)
 * Description : Opens the requested file and 
 * sets values related to the FAT file
*/ 
void newImage(char *filename)
{
    fp = fopen(filename, "r");
    if( !fp )
    {
        printf("ERRO: No such file named %s\n", filename);
        return;
    }

    fseek( fp, 11, SEEK_SET );
    fread( &BPB_BytesPerSec, 2, 1, fp );

    fseek( fp, 13, SEEK_SET );
    fread( &BPB_SecPerClus, 1, 1, fp );

    fseek( fp, 14, SEEK_SET );
    fread( &BPB_RsvdSecCnt, 2, 1, fp );

    fseek( fp, 16, SEEK_SET );
    fread( &BPB_NumFATs, 1, 1, fp );

    fseek( fp, 36, SEEK_SET );
    fread( &BPB_FATSz32, 4, 1, fp );

    // NOT NEEDED ????
    fseek( fp, 3, SEEK_SET );
    fread( &BS_OEMName, 8, 1, fp );

    fseek( fp, 71, SEEK_SET );
    fread( &BS_VolLab, 11, 1, fp );

    fseek( fp, 17, SEEK_SET );
    fread( &BPB_RootEntCnt, 1, 1, fp );

    fseek( fp, 44, SEEK_SET );
    fread( &BPB_RootClus, 4, 1, fp );
    CWD = BPB_RootClus;

    int offset = LBAToOffset(BPB_RootClus);
    fseek( fp, offset, SEEK_SET );
    fread( &dir[0], 16, sizeof( struct DirectoryEntry ), fp );
    
    return;
}

/*
 * Function    : ListFiles
 * Parameters  : N/A
 * Returns     : No return value (void)
 * Description : List all file and directorys in the current directory  
*/ 
void ListFiles()
{
    int offset = LBAToOffset(CWD);
    fseek( fp, offset, SEEK_SET );
    fread( &dir[0], 16, sizeof( struct DirectoryEntry ), fp);

    int i;
    for(i = 0; i < 16; i++)
    {
        if(dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 ||
           dir[i].DIR_Attr == 0x20 || dir[i].DIR_Attr == 0x30 )
        {   
            if((unsigned char)dir[i].DIR_Name[0] == 0xe5)
            {
                continue;
            }
            /*
            if ((unsigned char)dir[i].DIR_Name[0] == 0x05)
            {
                dir[i].DIR_
            }
            */
            char *file_name = malloc(11);
            memset( file_name, '\0', 11 );
            memcpy( file_name, dir[i].DIR_Name,11 ); 
            printf( "File Name: %s\n",file_name );
        }
    }
}
/*
 * Function    : chang_dir
 * Parameters  : char * dir_name
 * Returns     : No return value (void)
 * Description : Changes into the directory specified by the user  
*/ 
void change_dir(char * dir_name)
{
     
    int i; 
    int offset;
    for(i = 0; i < 16; i++)
    {
        if(!strncmp(dir[i].DIR_Name,dir_name,strlen(dir_name)))
        {
            
           if( dir[i].DIR_FirstClusterLow == 0 )
           {
               offset = LBAToOffset(2);
               CWD = offset;
           }
           else
           {
               offset = LBAToOffset(dir[i].DIR_FirstClusterLow);
               CWD = dir[i].DIR_FirstClusterLow;
           }

           fseek( fp, offset, SEEK_SET );
           fread(&dir[0], 16, sizeof( struct DirectoryEntry ), fp);
           return;
        }
    }
}
/*
 * Function    : stat
 * Parameters  : char * dir_name
 * Returns     : No return value (void)
 * Description : Displays Attribute, starting cluster number of the file or the dir name  
*/ 
void stat(char * dir_name)
{
    int i;
    
    for(i = 0; i < 16; i++)
    {
        if(!strncmp(dir[i].DIR_Name,dir_name,strlen(dir_name)))
        {
            
            printf("Attribute: %d\n",dir[i].DIR_Attr);
            printf("Cluster Start: %d\n",dir[i].DIR_FirstClusterLow);
            char *file_name = malloc(11);
            memset( file_name, '\0', 11 );
            memcpy( file_name, dir[i].DIR_Name,11 ); 
            printf( "Name: %s\n",file_name );
            
            return;
        }
    }
    printf("ERROR: File not found.\n");
    return;
}

/*
 * Function    : get
 * Parameters  : char * dir_name
 * Returns     : No return value (void)
 * Description : Retrieves a file that is on the FAT32 and saves it to the 
 * current working directory  
*/ 
void get(char * dir_name)
{
    int i; 
    uint offset;
    int size;
    int16_t nxt_clst;
    char *file_name = malloc(12);
    memset( file_name, '\0', 12 );
    memcpy( file_name, dir_name, 12);

    FormatDirName(dir_name);
    
    FILE *local_file;
    for(i = 0; i < 16; i++)
    {
        if(!strncmp(dir[i].DIR_Name,NextDir,strlen(NextDir)))
        {
            local_file = fopen(file_name, "w");
            size = dir[i].DIR_FileSize;
            offset = LBAToOffset(dir[i].DIR_FirstClusterLow);
            nxt_clst = dir[i].DIR_FirstClusterLow;
            uint8_t value[size];

            while(nxt_clst != -1)
            {
                fseek( fp, offset, SEEK_SET );
                fread(&value, 512, 1, fp);
                fwrite(&value, 512, 1, local_file);
                size -= 512;                
                nxt_clst = NextLB(nxt_clst);
                offset = LBAToOffset(nxt_clst);
            }

            fseek( fp, offset, SEEK_SET );
            fread(&value, size, 1, fp);
            fwrite(&value, size, 1, local_file);
            
            
            
            
            fclose(local_file);
            
            return;
        }
    }
    printf("ERROR: File not found.\n");
    return;
}

/*
 * Function    : read__
 * Parameters  : char * dir_name, int position, int num_bytes
 * Returns     : No return value (void)
 * Description : Finds the indicated file, finds the indicated position
 * reads and displays the disgnated amount of bytes
*/ 
void read__(char * dir_name, int position, int num_bytes)
{
    int i;
    uint8_t val; 
    int offset;
    int nxt_clst;
    FormatDirName(dir_name);

    for(i = 0; i < 16; i++)
    {
        if(!strncmp(dir[i].DIR_Name,NextDir,strlen(NextDir)))
        {
           
            nxt_clst = dir[i].DIR_FirstClusterLow;
            int num_blocks = position / 512;
            int in_block = position % 512;

            while(nxt_clst != -1 && num_blocks)
            {   
                nxt_clst = NextLB(nxt_clst);
                num_blocks--;
            }
            offset = LBAToOffset(nxt_clst) + in_block;
            fseek(fp, offset, SEEK_SET);
            int j;
            for(j = 0; j < num_bytes; j++)
            {
                if(j + in_block > 512)
                {
                    nxt_clst = NextLB(nxt_clst);
                    offset = LBAToOffset(nxt_clst);
                    fseek(fp, offset, SEEK_SET);
                    in_block = 0;
                }
                fread(&val, 1, 1, fp);
                printf("0x%x ", val);
            }

            printf("\n");
            return;
        }
    }
    printf("ERROR: File not found.\n");
    return;
}

int main()
{

    
    char *filename;
    char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

    while( 1 )
    {
        // Print out the mfs prompt
        printf("mfs> ");

        // Read the command from the commandline. The
        // maximum command that will be rea is MAX_COMMAND_SIZE
        // This while command will wait here until the user
        // inputs something since fgets returns NULL when there
        // is no input
        while( !fgets(cmd_str, MAX_COMMAND_SIZE, stdin) );

        /* Parse input */
        char *token[MAX_NUM_ARGUMENTS];

        int token_count = 0;

        // Pointer to point to the token
        // parsed by strsep
        char *arg_ptr;

        char *working_str = strdup( cmd_str );

        // we are going to move the working_str pointer so 
        // keep track of its original calue so we can deallocate
        // the correct amount at the end
        char *working_root = working_str;

        // Tokenize the input strings with whitespace used as the delimeter
        while( ( ( arg_ptr = strsep(&working_str, WHITESPACE) ) != NULL) &&
                    (token_count<MAX_NUM_ARGUMENTS))
        {
            token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
            if( strlen( token[token_count] ) == 0 )
            {
                token[token_count] = NULL;
            }

            token_count++;

        }

        //My functionality of prog
        // \TODO Remove this code and replace with your shell functionality 
        if(fp == NULL && strcmp(token[0],"open"))
        {
            printf("ERROR: No file currently open, Please open a file.\n");
        } 
        else if( !strcmp(token[0], "open") )
        {
            // OPEN FILE
            
            if(fp != NULL)
            {
                printf("ERROR: File system image already open.\n");
            }
            else if (token[1] != NULL)
            {
                if( !strcmp(token[1], filename) )
                {
                    printf("ERROR: File %s is already open.\n",filename);    
                }
                else
                {
                    newImage(token[1]);
                }
            }
            else
            {
                printf("ERROR: No File name provided.\n");
            }
            
        }
        else if( !strcmp(token[0], "close") )
        {
            //CLOSE FILE
            if(fp == NULL)
            {
                printf("ERROR: No File currently open.\n");
            }
            else
            {
               fclose(fp);
               fp = NULL;
               printf("File has been closed\n");
            }
            
        }
        else if( !strcmp(token[0], "info") )
        {
            //PROVIDE INFO ON FILE (hex and base 10)
            printf("BPB_BytesPerSec: %d\n",BPB_BytesPerSec);
            printf("BPB_BytesPerSec: %x\n\n",BPB_BytesPerSec);
            printf("BPB_SecPerClus: %d\n",BPB_SecPerClus);
            printf("BPB_SecPerClus: %x\n\n",BPB_SecPerClus);
            printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
            printf("BPB_RsvdSecCnt: %x\n\n", BPB_RsvdSecCnt);
            printf("BPB_NumFATs: %d\n",BPB_NumFATs);
            printf("BPB_NumFATs: %x\n\n",BPB_NumFATs);
            printf("BPB_FATSz32: %d\n", BPB_FATSz32);
            printf("BPB_FATSz32: %x\n\n", BPB_FATSz32);

        }
        else if( !strcmp(token[0], "stat") )
        {
            //Print attributes
            //stat <filename> or <directory name>

            if(!strcmp(token[1],".."))
            {
                stat(token[1]);
            }
            else
            {
                FormatDirName(token[1]);
                stat(NextDir);
            }
            
            
        }
        else if( !strcmp(token[0], "get") )
        {
            //Retrieve file from FAT 32 img
            //get <filename>
            if(token[1] == NULL)
            {
                printf("ERROR: invalid argument 1.\n");
            }
            else
            {
                get(token[1]);
            }
            
        }
        else if( !strcmp(token[0], "cd") )
        {
            //change current working dir to requested dir
            //cd <directory>
            //support .. and .
            if(token[1] == NULL)
            {
                printf("ERROR: invalid argument 1.\n");
            }
            else
            {
                char *path_token = malloc(strlen(token[1]));
                memset( path_token, '\0', strlen(token[1]) );
                memcpy( path_token, token[1], strlen(token[1]));
                
                char *ptr_path;
                char *ptr;
                while( ( ptr_path = strsep(&path_token, "/") ) != NULL)
                {

                    ptr = strndup( ptr_path, MAX_COMMAND_SIZE );
                    if( !strcmp(ptr_path,"..") )
                    {
                        change_dir(ptr_path);
                    }
                    else
                    {
                        FormatDirName(ptr_path);
                        if(ext)
                        {
                            printf("Can not cd into file\n");
                        }
                        else
                        {
                            change_dir(NextDir);
                        }
                    }
                   
                }
            }
        }
        else if( !strcmp(token[0], "ls") )
        {
            //list directory contents
            //shall support .. and .
            //shall not list delted files or system volume names
            ListFiles();
        }
        else if( !strcmp(token[0], "read") )
        {
            //READS FROM THE GIVEN FILE at the position, in bytes, specified 
            //OUTPUTS the number of bytes specified
            //read <filename> <position> <number of bytes>

            if(token_count < 4)
            {
                printf("ERROR: invalid number of arguments.\n");
            }
            else
            {
                read__(token[1], atoi(token[2]), atoi(token[3]));
            }
            
        }

        free(working_root);

    }
    return 0;
}
