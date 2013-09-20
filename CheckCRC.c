/*============================================================================
 *
 *  Program:      CheckCRC.c
 *
 *  Version:      1.5 (22.11.01)
 *
 *  Purpose:      Checks CRC of listfile(s)
 *
 *                #define either SYS_AMIGA or SYS_UNIX (see below)
 *
 *                AMIGA-Commandline-Options:
 *
 *                   LIST/A      path of the listfiles or filename of listfile
 *                   NOSTATS/S   don't print statistics
 *                   QUIET/S     don't show progress
 *                   LOGFILE/N   name of logfile
 *
 *
 *                UNIX-Commandline-Options:
 *
 *                   <list>      path of the listfiles or filename of listfile
 *                  optional:
 *                   -nostats    don't print statistics
 *                   -quiet      don't show progress
 *                   -logfile    name of logfile
 *
 *
 *  Author:       Andre Bernhardt <ab@imdb.com>
 *
 *  Copyright:    (w) Andre Bernhardt 1995 - 2001
 *                (c) Internet MovieDatabase Limited 1990 - 2001
 *
 *       This file is part of the Internet MovieDatabase project.
 *
 *  The  MovieDatabase  FAQ contains more information on the whole project.
 *  For   a   copy   send  an  e-mail   with  the  subject  "HELP  FAQ"  to
 *  <mail-server@imdb.com>.
 *
 *  Permission  is  granted  to make and distribute verbatim copies of this
 *  package  provided  the  copyright notice and this permission notice are
 *  preserved  on  all  copies  and the package is distributed in unaltered
 *  archive  form only.  It is not allowed to modify the source code and/or
 *  redistribute  modified  copies  of  it  and/or  the executables without
 *  written permission of the author.
 *
 *  If  you need to make a change to the source-code in order to be able to
 *  use the package, you have to notify the author.
 *
 *  No guarantee of any kind is given that the programs and scripts in this
 *  package  are  100%  reliable.  You are using this material at your  own
 *  risk.   The  author  cannot be made responsible for any damage which is
 *  caused by using these programs.
 *
 *  This  package  is  freely  distributable,  but still copyright by  IMDb
 *  Ltd.
 *
 *  None  of  the programs or scripts nor the source code (nor parts of it)
 *  may  be  included  or  used  in  commercial  programs unless by written
 *  permission from the author.
 *
 *============================================================================
 */

/* some defines (specified by the Makefile) */
/*#define SYS_AMIGA */
/*#define SYS_UNIX  */
/*#define IMDB_DEBUG*/

/* ************************* */

#include "IMDB.h"

#ifdef SYS_AMIGA
#include <clib/exec_protos.h>       
#include <dos/dos.h>
#include <clib/dos_protos.h> 
#include <Exec/Memory.h>
#endif /* SYS_AMIGA */

#ifdef SYS_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef NEXT
#include <sys/dir.h>
#else
#include "dirent.h"
#endif /* NEXT */

#endif /* SYS_UNIX*/

#define VERSION "CheckCRC 1.5 (22.11.01)"
static const char version[] ="$VER: "VERSION;

/* Return values */
#define RET_OK              0
#define RET_WARNING        10
#define RET_ERROR          20

/* buffer sizes */
#define ADV_BUFFER_SIZE    512 * 1024
#define ADV_MAX_LINESIZE     8 * 1024

/* Information on a diff */
#define STATUS_OK    0  /* No error */
#define STATUS_CRC   1  /* CRC-Checksum error */
#define STATUS_IO    2  /* IO-Error */
#define STATUS_VER   3  /* wrong file-diffile-combination */
#define STATUS_SYN   4  /* Syntax-Error in Diff-File */
#define STATUS_NEW   5  /* New file */
#define STATUS_NOCRC 6  /* file contains no CRC-line */

/* types */
#define DIFF_TYPE_UNKNOWN     0
#define DIFF_TYPE_ORIGINAL    1
#define DIFF_TYPE_STRIPPED    2

typedef struct DIFFINFO
 {
  struct DIFFINFO *next;
  char  fname_list[256];           /* filename */
  char  filedate[40];
  LONG  filesize;
  LONG  status;
 } DiffInfo;

typedef struct
 {
  char *p_list;
  LONG  f_nostats;
  LONG  f_quiet;
  char *p_logfile;
 } AD_Commands;

/******************************************************************************
 * Functions dealing with CRC-sum
 ******************************************************************************
 */

 /* global variable for tab */
 ULONG *pCrcTab = NULL;
 ULONG nCrc = 0xFFFFFFFFL;
 int nIndex;
 static char old_crc[16];
 static char new_crc[16];


/*-----------------------------------------------------------------------------
 * Procedure:   InitCRC
 *
 * Purpose:     This function initializes the CRC-32 algorithm by setting a private
 *              pre  calculation  table  in the memory.  This table is used
 *              for all further CRC-32 calculations.
 *
 * Parameters:  none
 *
 * Returns:     Pointer to CRC-Table
 *-----------------------------------------------------------------------------
 */

u_long *InitCRC(void)
{
  u_long *p_crc_tab = NULL;
  int i;
  int j;

  u_char ib[8];
  u_char lb[32];


  /* allocate memory for table */
  if ((p_crc_tab = malloc(256*sizeof(u_long))))
   {  
    /* initialize table with calculated bits */
    for (i=0; i<256; i++)
     {
       /* reset bits */
       for (j=0; j<8;j++)
	 ib[j]=(i>>(7-j))&1;
       for (j=0; j<32;j++)
	 lb[j]=0;

       /* calculate values */
       lb[31]=      ib[1]                              ^ib[7];
       lb[30]=ib[0]^ib[1]                        ^ib[6]^ib[7];
       lb[29]=ib[0]^ib[1]                  ^ib[5]^ib[6]^ib[7];
       lb[28]=ib[0]                  ^ib[4]^ib[5]^ib[6];
       lb[27]=      ib[1]      ^ib[3]^ib[4]^ib[5]      ^ib[7];
       lb[26]=ib[0]^ib[1]^ib[2]^ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[25]=ib[0]^ib[1]^ib[2]^ib[3]      ^ib[5]^ib[6];
       lb[24]=ib[0]      ^ib[2]      ^ib[4]^ib[5]      ^ib[7];
       lb[23]=                  ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[22]=            ib[2]^ib[3]      ^ib[5]^ib[6];
       lb[21]=            ib[2]      ^ib[4]^ib[5]      ^ib[7];
       lb[20]=                  ib[3]^ib[4]      ^ib[6]^ib[7];
       lb[19]=      ib[1]^ib[2]^ib[3]      ^ib[5]^ib[6]^ib[7];
       lb[18]=ib[0]^ib[1]^ib[2]      ^ib[4]^ib[5]^ib[6];
       lb[17]=ib[0]^ib[1]      ^ib[3]^ib[4]^ib[5];
       lb[16]=ib[0]      ^ib[2]^ib[3]^ib[4];
       lb[15]=            ib[2]^ib[3]                  ^ib[7];
       lb[14]=      ib[1]^ib[2]                  ^ib[6];
       lb[13]=ib[0]^ib[1]                  ^ib[5];
       lb[12]=ib[0]                  ^ib[4];
       lb[11]=                  ib[3];
       lb[10]=            ib[2];
       lb[ 9]=                                          ib[7];
       lb[ 8]=      ib[1]                        ^ib[6]^ib[7];
       lb[ 7]=ib[0]                        ^ib[5]^ib[6];
       lb[ 6]=                        ib[4]^ib[5];
       lb[ 5]=      ib[1]      ^ib[3]^ib[4]            ^ib[7];
       lb[ 4]=ib[0]      ^ib[2]^ib[3]            ^ib[6];
       lb[ 3]=      ib[1]^ib[2]            ^ib[5];
       lb[ 2]=ib[0]^ib[1]            ^ib[4];
       lb[ 1]=ib[0]            ^ib[3];
       lb[ 0]=            ib[2];
      
       /* store value */
       p_crc_tab[i]=0;
       for (j=0; j<32;j++)
	 p_crc_tab[i]|=lb[j]<<(31-j);
     }
   }
  return (p_crc_tab);
}

/*-----------------------------------------------------------------------------
 * Procedure:   calc_crc
 *
 * Purpose:     calc crc of string. Automatically adds crc for '\n'
 *
 * Parameters:  
 *
 * Returns:     
 *-----------------------------------------------------------------------------
 */

void calc_crc (char *str, ULONG *nCrc)
 {
  while (*str)
   {
    nIndex = (int) ((*nCrc ^ *str++) & 0x000000FFL);
    *nCrc = ((*nCrc >> 8) & 0x00FFFFFFL) ^ pCrcTab[nIndex];
   }

  /* add '\n' */
  nIndex = (int) ((*nCrc ^ '\n') & 0x000000FFL);
  *nCrc = ((*nCrc >> 8) & 0x00FFFFFFL) ^ pCrcTab[nIndex];
 }

/******************************************************************************
 *  
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:   checkfile_crc
 *
 * Parameters:  listfile
 *
 * Comments:
 *-----------------------------------------------------------------------------
 */

void checkfile_crc(char *p_path, DiffInfo *p_diffinfo, BOOL flag_verbose)
 {
  static  char listfile[256];
  IMDB_Buffer *list_buffer = NULL;
  char        *p_list_line;
  char        *p_str;
  LONG         status      = STATUS_OK;
  LONG         progress    = 0;
  LONG         tprogress   = 0;

/* Example
CRC: 0x408ADC16  File: crazy-credits.list  Date: Fri Sep 27 01:00:00 1996
*/

  /* Filename */   
  if (p_path)
   strcpy (listfile, p_path);
  else
   listfile[0] = '\0';
  strncat(listfile, p_diffinfo->fname_list, 255-strlen(listfile));

  /* Reset CRC */
  nCrc = 0xFFFFFFFFL;
  new_crc [0] = '\0';
  old_crc [0] = '\0';

  /* open old listfile */
  if ((list_buffer = IMDBOpenBuffer (listfile, IMDBV_FILE_READ|IMDBV_FILE_GETSIZE, ADV_BUFFER_SIZE))
    &&(0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE)))
   {
    p_diffinfo->filesize = list_buffer->filesize;

    if (0 == strncmp (p_list_line, "CRC: ", strlen("CRC: ")))
     {
      /* Merke CRC */
      strncpy (old_crc, p_list_line, 15);

      /*Merke Datum */
      if (p_str = strstr(p_list_line, "Date: "))
       {
        p_str += 6;
        strncpy(p_diffinfo->filedate, p_str, 39);
       }
      else
        strcpy(p_diffinfo->filedate, "---- not  available ----");

      /* calculate CRC */
      while (0 == IMDBReadBufferLine (list_buffer, &p_list_line, ADV_MAX_LINESIZE))
       {
        if ((flag_verbose) && (progress != (tprogress = ((list_buffer->filepos>>7)*100/(list_buffer->filesize>>7)))))
         {
          progress = tprogress;
          printf ("\b\b\b\b\b\b(%03i%%)", progress);
          fflush (stdout);
         }

        /* calculate CRC for next line */
        calc_crc (p_list_line, &nCrc);
       }

      /* compare CRC */
      sprintf(new_crc, "CRC: 0x%08X", nCrc);
      if (0 == strcmp (old_crc, new_crc))
       {
        if (flag_verbose)
         printf ("\b\b\b\b\b\b- CRC O.K.\n");
       }
      else
       {
        if (flag_verbose)
         printf ("\b\b\b\b\b\b- CRC Error\n");
        status = STATUS_CRC;
       }
     }
    else
     {
      if (flag_verbose)
       printf ("\b\b\b\b\b\b- CRC not available\n");
      strcpy(p_diffinfo->filedate, "---- not  available ----");
      status = STATUS_NOCRC;
     }

    /* Close Buffer */
    IMDBCloseBuffer (list_buffer);
   }

  p_diffinfo->status = status;
 }

/******************************************************************************
 *  Main - Procedure
 ******************************************************************************
 */

/*-----------------------------------------------------------------------------
 * Procedure:   main
 *
 * Parameters:  nb_args, filename
 *
 * Returns:     
 * 
 * Comments:    
 *-----------------------------------------------------------------------------
 */

int main(int argc, char *argv[])
 {
  AD_Commands  ad_cmds  = {NULL, FALSE, FALSE, NULL};
  DiffInfo    *diffinfo = NULL;
  DiffInfo    *t_diffinfo = NULL;
  DiffInfo    *a_diffinfo = NULL;
  int  ret_val        = RET_OK;
  int  ret            = RET_OK;
  static char filename[255];

 
#ifdef SYS_AMIGA
  printf (VERSION" written by André Bernhardt; © 1996-2001 IMDb Ltd.\n"); 
#else
  printf (VERSION" written by André Bernhardt; (c) 1996-2001 IMDb Ltd.\n"); 
#endif

  /* Parse command line parameters */
#ifdef SYS_AMIGA
  {
   static const char Template[]    = "LIST/A,NOSTATS/S,QUIET/S,LOGFILE/K";
   AD_Commands       cmdlineparams = {NULL, FALSE, FALSE, NULL};
   struct RDArgs    *rda;
   LONG              len;
   char              c;

   rda = ReadArgs((char*) Template, (LONG *) &cmdlineparams, NULL);
 
   /* Get values */
   if (cmdlineparams.p_list)
    if (ad_cmds.p_list = IMDBAllocMemory (2+(len = strlen(cmdlineparams.p_list))))
     {
      strcpy(filename, cmdlineparams.p_list);
      strcpy(ad_cmds.p_list, cmdlineparams.p_list);
      c = ad_cmds.p_list[len-1];
      if ((c != ':') && (c != '/'))
       strcat (ad_cmds.p_list,"/");
     }

   ad_cmds.f_nostats  = cmdlineparams.f_nostats ;
   ad_cmds.f_quiet    = cmdlineparams.f_quiet   ;

   if (cmdlineparams.p_logfile)
    if (ad_cmds.p_logfile = IMDBAllocMemory (1+ strlen(cmdlineparams.p_logfile)))
     strcpy(ad_cmds.p_logfile, cmdlineparams.p_logfile);

   /* Free ReadArgs parameters */
   if (NULL == rda)
    {
     printf ("Template: %s\n",Template); 
     exit (RET_ERROR);
    }
   else
    FreeArgs(rda);
  }
#endif

#ifdef SYS_UNIX
  {
   static const char Template[] = "usage: CheckCRC <list(s)> [-nostats][-quiet][-logfile <filename>]";
   LONG              i;

   if (argc <2)
    {
     puts (Template);
     exit (10);
    }

   /* list-path */
   if (ad_cmds.p_list = IMDBAllocMemory (2 + strlen(argv[1])))
    {
     strcpy(filename, argv[1]);
     strcpy(ad_cmds.p_list, argv[1]);
     if ('/' != ad_cmds.p_list[strlen(ad_cmds.p_list)-1])
      strcat (ad_cmds.p_list,"/");
    }

   /* Parse Command Line Parameters */
   for (i=2; i < argc; i++)
    {
     if (!strcmp(argv[i], "-nostats"))
      ad_cmds.f_nostats  = TRUE;
     else
     if (!strcmp(argv[i], "-quiet"))
      ad_cmds.f_quiet    = TRUE;
     else
     if (!strcmp(argv[i], "-logfile"))
      {
       if (ad_cmds.p_logfile = IMDBAllocMemory (2 + strlen(argv[++i])))
        strcpy(ad_cmds.p_logfile, argv[i]);
      }
     else
      {
       puts (Template);
       exit (10);
      }
    }

   if (NULL == ad_cmds.p_list)
    {
     puts (Template);
     exit (10);
    }
  }
#endif

  
#ifdef IMDB_DEBUG
  printf ("Listdir: %s\nnostats %i\nquiet %i\n", ad_cmds.p_list, ad_cmds.f_nostats, ad_cmds.f_quiet);
  if (ad_cmds.p_logfile)
   printf ("Logfile: %s \n", ad_cmds.p_logfile);
  else
   printf ("Logfile: -none-\n");
#endif

  /* Create CRC-Table */
  if (!(pCrcTab = InitCRC()))
   {
    printf("Can't Create CRC-Table!\n");
    exit (RET_ERROR);
   }

  /* Do we want check a single file or a whole directory? */
#ifdef SYS_AMIGA
  if ((strlen(filename) > 5) && (0 == strnicmp(&filename[strlen(filename)-5], ".list", 5)))
#else
  if ((strlen(filename) > 5) && (0 == strncmp(&filename[strlen(filename)-5], ".list", 5)))
#endif
   {
    IMDB_Buffer *list_buffer;

    /* Does this File exist? */
    if (list_buffer = IMDBOpenBuffer (filename, IMDBV_FILE_READ, ADV_BUFFER_SIZE))
     {
      IMDBCloseBuffer (list_buffer);
      if (diffinfo = IMDBAllocMemory (sizeof (DiffInfo)))
       {
        diffinfo->next = NULL;
        strcpy (diffinfo->fname_list, filename);
        diffinfo->filesize = 0;
        diffinfo->filedate[0] = '\0';;
        diffinfo->status = STATUS_OK;
       }
      ad_cmds.p_list = NULL; /* there is only one file with the complete pathname */ 
     }
    else
     {
      printf("Failed to open %s!\n", filename);
      ret_val = RET_ERROR;
     }
   }

  else

  /* get all filenames in the diff-files-directory */
#ifdef SYS_AMIGA
  {
   BPTR                  lock;
   struct FileInfoBlock *fib;

   if (fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_PUBLIC))
    {
     if (lock = Lock(ad_cmds.p_list,SHARED_LOCK))
      {
       if ((Examine(lock,fib))&&(fib->fib_DirEntryType > 0))
        {
         while ((ExNext(lock,fib)) && (ret_val != RET_ERROR))
          {
           if ((fib->fib_DirEntryType <= 0) && (strlen(fib->fib_FileName) > 5)
            && (0 == strnicmp(&fib->fib_FileName[strlen(fib->fib_FileName)-5], ".list", 5)))
            {
             /* Create and initialize DiffInfo */ 
             if (a_diffinfo = IMDBAllocMemory (sizeof (DiffInfo)))
              {
               a_diffinfo->next = NULL;
               strcpy (a_diffinfo->fname_list, fib->fib_FileName);
               a_diffinfo->filesize = 0;
               a_diffinfo->filedate[0] = '\0';;
               a_diffinfo->status = STATUS_OK;
              }
             else
              {
               printf("Can't allocate memory for DiffInfo\n");
               ret_val = RET_ERROR;
              }
             if (diffinfo)
              {
               t_diffinfo->next = a_diffinfo;
               if (a_diffinfo)
                t_diffinfo = a_diffinfo;
              }
             else
              diffinfo = t_diffinfo = a_diffinfo;
            }
          }
        }
       else
        {
         printf("%s is no directory!\n", ad_cmds.p_list);
         ret_val = RET_ERROR;
        }
       UnLock(lock);
      }
     else
      {
       printf("Can't get shared lock on %s\n", ad_cmds.p_list);
       ret_val = RET_ERROR;
      }
     FreeMem(fib,sizeof(struct FileInfoBlock));
    }
   else
    {
     printf("Can't allocate memory for FileInfoBlock\n");
     ret_val = RET_ERROR;
    }
  }
#endif

#ifdef SYS_UNIX
  {
   struct stat stbuf;
   
   if (-1 == stat(ad_cmds.p_list, &stbuf))
    {
     printf("Can't access %s\n", ad_cmds.p_list);
     ret_val = RET_ERROR;
    }
   else
   if (S_IFDIR != (stbuf.st_mode & S_IFMT))
    {
     printf("%s is no directory.\n", ad_cmds.p_list);
     ret_val = RET_ERROR;
    }
   else
    {
#ifdef NEXT
     struct direct *dp;
#else
     struct dirent *dp;
#endif /* NEXT */
     DIR *dfd;
     
     if (NULL == (dfd = opendir(ad_cmds.p_list)))
      {
       printf("Can't open %s\n", ad_cmds.p_list);
       ret_val = RET_ERROR;          
      }
     else
      {
       while (dp = readdir(dfd))
        {
         if ((strlen (dp->d_name) <=5 ) || (0 != strncmp(&dp->d_name[strlen(dp->d_name)-5], ".list", 5)))
          continue;
                                  
         /* Create and initialize DiffInfo */ 
         if (a_diffinfo = IMDBAllocMemory (sizeof (DiffInfo)))
          {
           a_diffinfo->next = NULL;
           strcpy (a_diffinfo->fname_list, dp->d_name);
#ifdef IMDB_DEBUG
printf ("-> %s\n", a_diffinfo->fname_list);
#endif
           a_diffinfo->filesize = 0;
           a_diffinfo->filedate[0] = '\0';;
           a_diffinfo->status = STATUS_OK;
          }
         else
          {
           printf("Can't allocate memory for DiffInfo\n");
           ret_val = RET_ERROR;
          }
         if (diffinfo)
          {
           t_diffinfo->next = a_diffinfo;
           if (a_diffinfo)
            t_diffinfo = a_diffinfo;
          }
         else
          diffinfo = t_diffinfo = a_diffinfo;
        }
       closedir(dfd);
      }
    }
  }
#endif

  /* sort filenames (= sort Diffinfo) */
  {
   char t_fname[256];

   t_diffinfo = diffinfo;
   while (t_diffinfo)
    {
     a_diffinfo = t_diffinfo->next;
     while (a_diffinfo)
      {
       if (strcmp(t_diffinfo->fname_list, a_diffinfo->fname_list) > 0)
        {
         strcpy(t_fname               , t_diffinfo->fname_list);
         strcpy(t_diffinfo->fname_list, a_diffinfo->fname_list);
         strcpy(a_diffinfo->fname_list, t_fname);
        }
       a_diffinfo = a_diffinfo->next;
      }
     t_diffinfo = t_diffinfo->next;
    }
  }
 
  printf ("\n");
 
  /* check CRC-sum of the listfiles  */
  if (RET_OK == ret_val)
   {
    t_diffinfo = diffinfo;
 
    while (t_diffinfo)
     {
      /* Check CRC */
      if (!ad_cmds.f_quiet)
       {
        printf ("Check CRC of File %s (000%%)", t_diffinfo->fname_list);
        fflush (stdout);
       }
      checkfile_crc (ad_cmds.p_list, t_diffinfo, !ad_cmds.f_quiet);
      t_diffinfo = t_diffinfo->next;
     }
    if (!ad_cmds.f_quiet)
     printf ("\n");
   }
 
  /* Free CRC-Tab */
  IMDBFreeMemory(pCrcTab);
 
  /* Print statistic */
  if (!ad_cmds.f_nostats)
   {
    FILE   *p_file = NULL;
    time_t timeval;
 
    if (ad_cmds.p_logfile)
     p_file = fopen(ad_cmds.p_logfile, "ab");

    if ((timeval = time (NULL)) != -1)
     {
      printf ("CheckCRC Statistics - %s", ctime (&timeval));
      if (p_file) fprintf (p_file,"\nCheckCRC Statistics - %s", ctime (&timeval));
     }
    else
     {
      printf ("CheckCRC Statistics\n");
      if (p_file) fprintf (p_file,"\nCheckCRC Statistics\n");
     }
    printf ("\nStatus   Filesize  Filedate                  Listfile\n");
    if (p_file) fprintf (p_file,"\nStatus   Filesize  Filedate                  Listfile\n");
    printf ("------------------------------------------------------------\n");
    if (p_file) fprintf (p_file,"------------------------------------------------------------\n");
    while (diffinfo)
     {
      switch (diffinfo->status)
       {
        case STATUS_OK:
         printf ("CRC OK   ");
         if (p_file) fprintf (p_file,"CRC OK   ");
         break;
        case STATUS_CRC:
         printf ("CRC-Error");
         if (p_file) fprintf (p_file,"CRC-Error");
         break;
        case STATUS_NOCRC:
         printf ("CRC n/a  ");
         if (p_file) fprintf (p_file,"CRC n/a  ");
         break;
        case STATUS_IO:
         printf ("IO-Error ");
         if (p_file) fprintf (p_file,"IO-Error ");
         break;
        case STATUS_SYN:
         printf ("Syntax Error");
         if (p_file) fprintf (p_file,"Syntax Error");
         break;
        default:
         printf ("%i", diffinfo->status);
         if (p_file) fprintf (p_file,"%i", diffinfo->status);
         break;
       }
      printf ("%8i  %s  %s\n", diffinfo->filesize, diffinfo->filedate, diffinfo->fname_list);
      if (p_file) fprintf (p_file, "%8i  %s  %s\n", diffinfo->filesize, diffinfo->filedate, diffinfo->fname_list);
      t_diffinfo = diffinfo->next;
      IMDBFreeMemory (diffinfo);
      diffinfo = t_diffinfo;
     }

    if (p_file)
     fclose (p_file);
   }
  else
   {
    while (diffinfo)
     {
      t_diffinfo = diffinfo->next;
      IMDBFreeMemory (diffinfo);
      diffinfo = t_diffinfo;
     }
   }

  /* Free memory */
  if (ad_cmds.p_list) IMDBFreeMemory(ad_cmds.p_list);
  if (ad_cmds.p_logfile) IMDBFreeMemory(ad_cmds.p_logfile);
 
  if (RET_OK != ret_val)
   printf ("\nWARNING: CheckCRC encountered errors.\n");
 
  exit (ret_val);
 }
