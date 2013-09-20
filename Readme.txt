
                    Documentation DiffTools (22.11.01)
                    ==================================

                    (w) 1994 - 2001 by André Bernhardt
              (c) 1996 - 2001  by Internet MovieDatabase Ltd

          Send E-Mail with comments, suggestions, bugreports to:

                       Andre Bernhardt <ab@imdb.com>


 The DiffTools-Package consists of the following programs for use with the
 Internet MovieDatabase Listfiles.

  * ApplyDiffs V 2.5

  * CheckCRC V 1.5

 These programs have been successfully tested on the following systems:

  - HP-UX 9.5
  - SunOS 
  - Linux
  - AmigaOS
  - Next

 Please check out the Makefile and specify the correct compiler options. 
 Default is: UNIX - GCC

If   you   do  not  know  what  the  Internet  MovieDatabase  is,  get  the
MovieDatabase  FAQ  which  may be obtained by anonymous ftp to rtfm.mit.edu
under  /pub/usenet/news.answers/movies/movie-database-faq,  or  by  sending
e-mail to <movie@ibmpcug.co.uk> with the subject "HELP FAQ"


===============================================================================

                         ApplyDiffs 2.5 (22.11.01)
                         =========================

TEMPLATE
========


Amiga:
 ApplyDiffs LISTDIR/A,DIFFDIR/A,CHECKCRC/S,FORCE/S,KEEP/S,NOSTATS/S,QUIET/S,
            LOGFILE/K

Unix:
 ApplyDiffs <listpath> <diffpath> [-checkcrc][-force]
            [-keep][-nostats][-quiet][-logfile <filename>]

 - LISTDIR  directory where the moviedatabase listfiles are located
 - DIFFDIR  directory where the diffiles are located
 - KEEP     option. If  present, a copy of the old listfiles as well as the
            successfully applied diff-files will be kept.
 - FORCE    option. Skip wrong/corrupted diffs, but apply all others
 - QUIET    option. If present, don't print any progress-information, 
            only stats
 - CHECKCRC option. Check CRC-sum before applying diffs (takes some time)
 - NOSTATS  option. If present, don't print the stats.
 - LOGFILE  option. Filename where to store stats-information


PURPOSE
=======


The Internet MovieDatabase files are updated every Friday.  The latest full
versions  of  all  the  files  are  held in the 'database' directory of the
MovieDatabase  FTP-servers.   The  'diffs'  directory contains sets of diff
files for each week's database files with respect to the previous week's.

Each  file  in this directory is a tar archive which unpacks to a directory
called  'diffs'.   The  tar archives are named according to the week's full
data  to  which  they  should be applied in order to generate the following
week's data.  The filename format is:

  diffs-YYMMDD.tar.gz

For  example,  diffs-941230.tar.Z  contains  the diffs to be applied to the
30th  December  1994  files  in order to generate the files for 6th January
1995.

NOTE:   A  copy  of  the  Internet  MovieDatabase Diff-Files is uploaded on
AMINET  every  week  as  lha-archive.  You should be able to get the latest
diffs from your AMINET-mirror-site.

The  program  'ApplyDiffs'  applies all files of the weekly diff-archive to
the  listfiles  of the Internet MovieDatabase and checks the consistency of
the data.

NOTE:   if  you miss more than one weeks worth of updates you need to apply
the  patches  for  all  the missing weeks in succession to bring your local
copies up to date.

In  order  to  check  that  the  diffs have been applied correctly, all the
database files include a CRC on their first line.  The program 'ApplyDiffs'
will automatically verify that this checksum is correct.

After  having  applied  all  diffs  to  the  listfiles,  a  summary  of all
diff-files  together  with  the status information and number of lines that
have been added and deleted is shown.


USAGE
=====


- Uncompress the tar-archive

   gzip -d diffs-YYMMDD.tar.gz

  where  YYMMDD  is  the  date  of last week's full data to which the diffs
  shall be applied in order to generate this week's data.

- Un-tar the archive using the tar-program:

   tar -xvf diffs-YYMMDD.tar
  or
   gnutar -xvf diffs-YYMMDD.tar

- Start  'ApplyDiffs'  by  giving  the  directory-path where you keep  your
  listfiles  and the directory-path where the diffs are.  'ApplyDiffs' will
  scan the diffs-directory for diff-files and try to apply them.
  If  you  use  the  option  "KEEP",  a  copy  of  the old listfile and the
  apropriate  diff-file  will  be  kept  in case anything goes wrong.  If a
  diff-file  cannot  be  applied,  an  error  message  (e.g.  "CRC-Checksum
  Error") will be issued and the file will be skipped.

  E.g. Assuming that you keep your listfiles in the directory 
  "dh0:MovieDatabase/lists/" and you have un'tar'ed the diffs to "t:"
  you need to run 'ApplyDiffs':

   ApplyDiffs dh0:MovieDatabase/lists/ t:diffs/
  or
   ApplyDiffs dh0:MovieDatabase/lists/ t:diffs/ KEEP


STATS-INFORMATION
=================


After  having  applied  all  diffs  to  the  listfiles,  a  summary  of all
diff-files  together  with  the status information and number of lines that
have been added and deleted is shown.

You can store this information by using the LOGFILE-option with a filename.

The text in the status column means:

 - OK             The diffs have been applied allright.

 - New File       This is in fact no error, but to inform you that a new 
                  listfile has become available.

 - CRC-Error      The CRC-Sum is not correct.
                  Should not occur unless anyone has tampered with the file.

 - IO-Error       ApplyDiffs failed to open, read or write a file.

 - Wrong Diffs    You are trying to apply the diffs to a wrong listfile
                  Please check that you do only apply the correct pairs of
                  diffs-listfiles.

 - Syntax Error   The diff-file contains commands that are unknown to
                  Applydiff. Maybe this is not a diff-file at all!?

IMPORTANT:
 If  the  status  is  other  than  OK,  the  original listfile will be left
 unchanged.   The other files however may have been changed.  You might run
 into  trouble  when  applying the following week's diffs if you don't find
 the cause of the trouble.
 If you can't get ApplyDiffs to apply the diffs to a certain file, the best
 thing  to  do  is  to  get  this  listfile  from  one of the MovieDatabase
 FTP-sites.


RETURN-VALUES
=============


ApplyDiffs will return:

   0 if everything was O.K.

  10 as a warning that some minor problems occurred. 
     E.g. A listfile was missing somewhere, or an CRC-sum was wrong

  20 if a serious error has occurred

You  can use these values for a shell-script that automagically fetches the
latest diff-file, applies the data and updates the MovieDatabase.


WHY APPLYDIFFS?
===============


Instead  of  Applydiffs,  you could as well use the common program 'patch'.
Unfortunately,  this program creates very big temporary files when updating
the  actors-  &  actresses lists.  It is also very slow and can't check the
CRC-sums.   ApplyDiffs  on  the  other  hand  can only be used to apply the
MovieDatabase-diffs to the listfiles.  It cannot be used to apply any diffs
created by the program 'diff'!



===============================================================================

                          CheckCRC 1.5 (22.11.01)
                          ======================


TEMPLATE
========


Amiga:
 CheckCRC   LIST/A,NOSTATS/S,QUIET/S,LOGFILE/K

Unix:
 CheckCRC   <list(path)>[-nostats][-quiet][-logfile <filename>]

 - LIST     directory where the moviedatabase listfiles are located
            or listfile
 - QUIET    option. If present, don't print any progress-information, 
            only stats
 - NOSTATS  option. If present, don't print the stats.
 - LOGFILE  option. Filename where to store stats-information


PURPOSE
=======

CheckCRC checks if the listfile(s) contain the "CRC:"-tag in the first line
and tests if the CRC-sum is correct.


USAGE
=====

Run  CheckCRC  over  a  listfile  to  see  if  the  file is in sync with the
IMDB-reference listfiles.

   CheckCRC dh0:MovieDatabase/lists/actors.list
  or
   CheckCRC dh0:MovieDatabase/lists/ QUIET



STATS-INFORMATION
=================


After having tested every listfile, a summary will be presented.

You can store this information by using the LOGFILE-option with a filename.

The text in the status column means:

 - CRC OK         CRC sum is correct.

 - CRC-Error      The CRC-Sum is not correct.
                  Should not occur unless anyone has tampered with the file.

 - CRC n/a        The listfile does not contain a CRC line. This is not an
                  error, but it should never occur.

 - IO-Error       CheckCRC failed to open, read or write a file.

 - Syntax Error   The diff-file contains commands that are unknown to
                  CheckCRC. Should never occur.



RETURN-VALUES
=============


CheckCRC will return:

   0 if everything was O.K.

  10 as a warning that some minor problems occurred. 
     E.g. A listfile contains no CRC-line, or an CRC-sum was wrong

  20 if a serious error has occurred


===============================================================================


DISCLAIMER
==========


   THERE  IS  NO  WARRANTY  FOR  THE  PROGRAM,  TO  THE EXTENT PERMITTED BY
APPLICABLE  LAW.   EXCEPT  WHEN  OTHERWISE  STATED IN WRITING THE COPYRIGHT
HOLDER  AND/OR OTHER PARTIES PROVIDE THE PROGRAM ''AS IS'' WITHOUT WARRANTY
OF  ANY  KIND,  EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
THE  IMPLIED  WARRANTIES  OF  MERCHANTABILITY  AND FITNESS FOR A PARTICULAR
PURPOSE.   THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE PROGRAM
IS  WITH  YOU.   SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF
ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

   IN  NO  EVENT  UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
WILL  ANY  COPYRIGHT  HOLDER,  OR  ANY OTHER PARTY WHO MAY REDISTRIBUTE THE
PROGRAM  AS  PERMITTED  ABOVE,  BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY
GENERAL,  SPECIAL,  INCIDENTAL  OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE  OR  INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF
DATA  OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD
PARTIES  OR  A  FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS),
EVEN  IF  SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.


LICENSE & COPYING POLICY: Internet Movie Database (IMDb)
========================================================
   
    This is a  database  of  movie  related  information  compiled  by
    Internet  Movie  Database  Ltd (IMDb). While every effort has been
    made to  ensure  the  accuracy  of  the  database  IMDb  gives  no
    warranty  as  to  the accuracy of the information contained in the
    database.  IMDb  reserves  the  right  to   withdraw   or   delete
    information at any time. 
       
    This service is provided for the information of users only. It  is
    not   provided  with  the  intention  that  users  rely  upon  the
    information for any purposes. Accordingly,  IMDb  shall  under  no
    circumstances  be liable for any loss or damage, including but not
    limited to loss of profits, goodwill or indirect or  consequential
    loss   arising   out   of  any  use  of  or  inaccuracies  in  the
    information. All warranties express or  implied  are  excluded  to
    the fullest extent permissible by law. 
       
    All information in this file  is  Copyright  1996  Internet  Movie
    Database  Limited.  Reproduction,  distribution or transmission by
    any means without the prior permission of IMDb is prohibited.  All
    rights reserved. 

    For further information contact <licensing@imdb.com>
       
         -------------------------------------------------------
   
    All data and software released by Internet Movie Database  Ltd  is
    freely  available  to  anyone  within certain limitations. You are
    encouraged to quote subsets of the database  in  USENET  articles,
    movie  related  FAQs,  magazine  articles etc. We do ask, however,
    that you make reference to the source of the data  and  provide  a
    pointer to the database for the benefit of the reader. 
       
    Permission is granted  by  the  copyright  holder  to  allow  free
    distribution  of  this  file  and  any  other part of the Internet
    Movie  Database  in  an  ELECTRONIC  FORM  ONLY,   providing   the
    following conditions are met: 
       
         1. NO FEE OF ANY KIND, however indirect, will be  charged
            for  its  distribution.  If  this file is being stored
            for  later  distribution  to  anyone   that   can   be
            construed   as   a   customer   of  yourself  or  your
            organisation YOU MUST contact Internet Movie  Database
            Ltd for permission. 
            
         2. Each  of  the  database  files  may   be   distributed
            individually  but  only  in an unaltered form. All the
            header and trailer information, including this  notice
            and  the  details  on how to access the database, must
            remain intact. 
            
         3. Specifically the files may NOT be  used  to  construct
            any  kind  of  on-line database (except for individual
            personal use). Clearance for  ALL  such  on-line  data
            resources   must  be  requested  from  Internet  Movie
            Database Ltd 
            
         4. In addition, copies of  the  Internet  Movie  Database
            frequently  asked  questions  list and additions guide
            must be made available in the same area / by the  same
            method as the other database files. 

         5. CD-ROM  distribution  is  prohibited  without  written
            permission from the Internet Movie Database Ltd 
            
    Distribution by e-mail, BBS and  Internet  systems  is  positively
    encouraged within these limitations. 
       
    The files and software which make up the  movie  database  may  be
    uploaded  to  commercial  BBS  systems  providing  that  the above
    conditions are met and no *additional* fees are applied above  the
    standard connect time or downloading charges. 

    For further information contact <licensing@imdb.com>
