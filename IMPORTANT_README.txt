IMPORTANT
=========

In order to properly apply the IMDB-Diffs called

diffs-011102.tar.gz
diffs-011109.tar.gz
diffs-011116.tar.gz

you need to run ApplyDiffs with the option "-force".

ApplyDiffs will ignore the CRC-sum, despite the fact that it claims the CRC
to be correct.

After applying the "diffs-011116.tar.gz"-file you should use CheckCRC to
make sure that the lists are still in sync.

Please keep a backup of the original (pre-"diffs-011102.tar.gz") listfiles
in case you run into problems. If anything goes wrong and the original
listfiles are lost you need to reload all complete listfiles from scratch!
