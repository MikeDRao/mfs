/* Michael Rao
 * 04/17/2020
 * FAT32 Assignment
*/

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

int32_t  RootDirSectors = 0;
int32_t  FirstDataSector = 0;
int32_t  FirstSectorofCluster = 0;

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
 * Description : Given a logical block address, look uo into the first FAT and retrun the 
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
 * Returns     : No return value (void)
 * Description : Correctly fomats the name of the directory and file extentsion
*/ 
void FormatDirName(char *dir_name)
{
    char expanded_name[12];
    memset( expanded_name, ' ', 12 );

    char *token = strtok( dir_name, "." );

    strncpy( expanded_name, token, strlen( token ) );

    token = strtok( NULL, "." );

    if( token )
    {
        strncpy( (char*)(expanded_name+8), token, strlen(token) );
    }

    expanded_name[11] = '\0';
    int i;
    for( i = 0; i < 11; i++)
    {
        expanded_name[i] = toupper( expanded_name[i] );
    }

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
        printf("ERRO: No such file named %s", filename);
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

    // NOT NEEDED 
    fseek( fp, 3, SEEK_SET );
    fread( &BS_OEMName, 8, 1, fp );

    fseek( fp, 71, SEEK_SET );
    fread( &BS_VolLab, 11, 1, fp );

    fseek( fp, 17, SEEK_SET );
    fread( &BPB_RootEntCnt, 1, 1, fp );

    fseek( fp, 44, SEEK_SET );
    fread( &BPB_RootClus, 4, 1, fp );

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

        if( !strcmp(token[0], "open") )
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
            printf("close\n");
        }
        else if( !strcmp(token[0], "info") )
        {
            //PROVIDE INFO ON FILE (hex and base 10)
            printf("BPB_BytesPerSec: %d\n",BPB_BytesPerSec);
            printf("BPB_SecPerClus: %d\n",BPB_SecPerClus);
            printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
            printf("BPB_NumFATs: %d\n",BPB_NumFATs);
            printf("BPB_FATSz32: %d\n", BPB_FATSz32);

        }
        else if( !strcmp(token[0], "stat") )
        {
            //Print attributes
            //stat <filename> or <directory name>
            printf("stat\n");
        }
        else if( !strcmp(token[0], "get") )
        {
            //Retrieve file from FAT 32 img
            //get <filename>
            printf("get\n");
        }
        else if( !strcmp(token[0], "cd") )
        {
            //change current working dir to requested dir
            //cd <directory>
            //support .. and .
            printf("cd\n");
        }
        else if( !strcmp(token[0], "ls") )
        {
            //list directory contents
            //shall support .. and .
            //shall not list delted files or system volume names
            printf("ls\n");
        }
        else if( !strcmp(token[0], "read") )
        {
            //READS FROM THE GIVEN FILE at the position, in bytes, specified 
            //OUTPUTS the number of bytes specified
            //read <filename> <position> <number of bytes>
            printf("read\n");
        }

        free(working_root);

    }
    return 0;
}
