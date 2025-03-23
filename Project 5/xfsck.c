//Program by WillDaru22
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define stat xv6_stat
#define dirent xv6_dirent
#include "fs.h"
#include "types.h"
#include "stat.h"
#undef stat
#undef dirent
//#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3) 

/*  Checks that the metadata in the superblock, the file-system size is larger
 *  than the number of blocks used by the super-block, inodes, bitmaps, and data.
 *  Prints an error if not.
 */
void sbSizeCheck(struct superblock* sblock) {
    //sb->size >= inodes
    int maxb;
    int minb;
    int bsize;
    maxb = minb = BBLOCK(0,sblock->ninodes);
    for(int i = 1; i < sblock->ninodes; i++) {
        bsize = BBLOCK(i,sblock->ninodes);
	if(bsize > maxb) {
	    maxb = bsize;
	}
	if(bsize < minb) {
	    minb = bsize;
	}
    }
    //int bitmapsize = sblock->size/BPB + 0.99;
    int bitmapsize = maxb - minb;
    if(sblock->size > sblock->nblocks+sblock->ninodes/IPB+bitmapsize+1) {
        return;
    }
    else {
        fprintf(stderr,"ERROR: superblock is corrupted.\n");
        exit(1);
    }

}

/*  Checks that each inode is either unallocated or one of the valid types(T_FILE,T_DIR,T_DEV).
 *  Prints an error if not.
 */
void typeCheck(struct superblock* block, struct dinode* dip) {
    for(int i = 0; i < block->ninodes; i++) {  //go through each inode
        //do the type check
        if(dip->type == T_FILE) {
          //passed do nothing on to next inode
        }
        else if(dip->type == T_DIR) {
          //passed do nothing on to next inode
        }
        else if(dip->type == T_DEV) {
          //passed do nothing
        }
        else if(dip->type == 0) {
          //passed do nothing
        }
        else {  //if not a valid type or unallocated, throw an error
            fprintf(stderr,"ERROR: bad inode.\n");
            exit(1);
        }
        //then add the size of the inode structure to advance the pointer
        //dip+=sizeof(struct dinode);
	dip++;
    }
    return;  //return at end of all inodes being checked
}

/* Checcks that each address from an in-use inode is valid
 * Prints an error if direct or indirect block is used and invalid
 * Prints an error if address in indirect block is in use and invalid
 */
void addrCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    uint *indirect;
    for(int i = 0; i < block->ninodes; i++) {
        if(dip->type != 0) {  //check that inode is in use
            //make sure address is valid
	    for(int j = 0; j < NDIRECT+1; j++) {  //go through each address
                if(dip->addrs[j] == 0) {  //checks address (block number) of data block
		    //unused
		}
		else if(dip->addrs[j] < (block->size - block->nblocks)) {  //if not in range, error
		    fprintf(stderr,"ERROR: bad indirect address in inode.\n");
		    exit(1);
		}
		else if(dip->addrs[j] > block->size) {  //if not in range, error
		    fprintf(stderr,"ERROR: bad direct address in inode.\n");
		    exit(1);
		}
		else {  //used and within range, valid and continue
		
		}
	    }
	    //check indirect block
	    if(dip->addrs[NDIRECT] != 0) {  //checks if in use
	        indirect = (uint*)(ptr + dip->addrs[NDIRECT] * BSIZE);
		for(int k = 0; k < NINDIRECT; k++) {
		    if(indirect[k] == 0) {
		        //unused
		    }
		    else if(indirect[k] < (block->size - block->nblocks)) {
		        fprintf(stderr,"ERROR: bad indirect address in inode.\n");
			exit(1);
		    }
		    else if(indirect[k] > block->size) {
		        fprintf(stderr,"ERROR: bad indirect address in inode.\n");
			exit(1);
		    }
		    else {
		        //used and within range
		    }
		}
	    }
        }
        dip++; 
    }
}

/* Checks that each directory contains . and .. entries and that
 * the . points to the directory.  Prints an error if not.
 */
void directoryCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    //check first two entries of each directory
    //check the dirent names for . and ..
    //starting at ROOTINO
    struct xv6_dirent *entry;
    //struct dinode *pointer;
    uint *indirect;
    int dotf = 0;
    int dotdotf = 0;
    //struct xv6_dirent *entryarray;
    for(int i = 0; i < block->ninodes; i++) {
        //check inode is a dir
        //check last possible addr which is the indirect block
        //. and .. both point to itself with root
        //inode number check
	//addr of 0 is unused
	if(dip->type != T_DIR) {  //checks if inode is a directory
	
	}
	else {
            for(int j = 0; j < NDIRECT; j++) {  //moves through the addrs checking for .
	        if(dip->addrs[j] != 0) {  //checks addr is in use
		    for(int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {
		        entry = (struct xv6_dirent *)(ptr + dip->addrs[j] * BSIZE);
			if(strcmp(entry[n].name,".") != 0) {
			
			}
		        else {
		            if(entry[n].inum != i) {  //checks that points to current dir
		                //printf("ERror here!\n");
			        fprintf(stderr,"ERROR: directory not properly formatted.\n");
			        exit(1);
			    }
			    else {
			        dotf++;  //increment to show . entry found
			    }
		        }
		    }
		} 
	    }
	    if(dip->addrs[NDIRECT] != 0) {
	        indirect = (uint*)(ptr + dip->addrs[NDIRECT] * BSIZE);
	        for(int m = 0; m < NINDIRECT; m++) {
		    for(int p = 0; p < BSIZE/sizeof(struct xv6_dirent); p++) {
		        entry = (struct xv6_dirent *)(ptr + indirect[m] * BSIZE);
			if(strcmp(entry[p].name,".") != 0) {
			
			}
			else {
			    if(entry[p].inum != i) {
			        fprintf(stderr,"ERROR: directory not properly formatted.\n");
				exit(1);
			    }
			    else {
			        dotf++;
			    }
			}
		    }
	        }
	    }
	    for(int k = 0; k < NDIRECT; k++) {  //moves through the addrs checking for ..
	       if(dip->addrs[k] != 0) {  //checks addr is in use
		   for(int e = 0; e < BSIZE/sizeof(struct xv6_dirent); e++) {
	               entry = (struct xv6_dirent *)(ptr + dip->addrs[k] * BSIZE);
                       if(strcmp(entry[e].name,"..") != 0) {  //checks if match
		       //not a match, continue
		       }
		       else {  //match
		           dotdotf++;  //increment to show .. entry found
		       }
		   }
	       }
	    }
	    if(dip->addrs[NDIRECT] != 0) {
	        indirect = (uint*)(ptr + dip->addrs[NDIRECT] * BSIZE);
	        for(int l = 0; l < NINDIRECT; l++) {
		    for(int u = 0; u < BSIZE/sizeof(struct xv6_dirent); u++) {
		        entry = (struct xv6_dirent *)(ptr + indirect[l] * BSIZE);
			if(strcmp(entry[u].name,"..") != 0) {
			
			}
			else {
			    dotdotf++;
			}
		    }
		}
	    }
	    if(dotf != 1 || dotdotf != 1) {  //check to make sure directory contains . and .. entries
	        fprintf(stderr,"ERROR: directory not properly formatted.\n");
		exit(1);
	    }
	    dotf = 0;
	    dotdotf = 0;
	}
	dip++;
    } 
}

/* Checks that for in-use inodes, each address in use is also marked in use in the bitmap
 * Prints an error if not.
 */
void inUseCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    uchar* bitmap;
    //int ownbitmap[block->nblocks];
    uint* indirect;
    uchar bit;
    uchar mask;
    for(int i = 0; i < block->ninodes; i++) {
        if(dip->type != 0) {  //checks if inode is in use
	    for(int j = 0; j < NDIRECT; j++) {  //direct block
	        if(dip->addrs[j] != 0) {  //addr in use
	            bitmap = (uchar*)(ptr + BSIZE * BBLOCK(dip->addrs[j],block->ninodes));
	            bit = bitmap[dip->addrs[j]/8];
	            mask = 1 << (dip->addrs[j] % 8);
	            if((bit & mask) != 0) {
	                //in use
			//ownbitmap[dip->addrs[j]] = (bit & mask);
	            }
	            else {  //not in use, error
	                fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
			exit(1);
		    }
		    //ownbitmap[dip->addrs[j]] = 1;
		}
	    }
	    if(dip->addrs[NDIRECT] != 0) {  //indirect block
	        indirect = (uint*)(ptr + dip->addrs[NDIRECT]*BSIZE); 
	        for(int k = 0; k < NINDIRECT; k++) {
		    if(indirect[k] != 0) {
			bitmap = (uchar*)(ptr + BSIZE * BBLOCK(indirect[k],block->ninodes));
			bit = bitmap[indirect[k]/8];
			mask = 1 << (indirect[k] % 8);
			if((bit & mask) != 0) {
			    //in use
			    //ownbitmap[indirect[k]] = (bit & mask);
			}
			else {  //not in use but marked in use in inode
			    fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
			    exit(1);
			}
		        //ownbitmap[indirect[k]] = 1;
		    }
		}
	    }
	}
	dip++;
    }
    //use bit arithmetic ie buf[i/8] to access bitmap
}

/* Check 6
 *
 */
void bitmapCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    int ownbitmap[block->nblocks];
    //printf("%d\n",block->nblocks);
    uint* indirect;
    uchar byte;
    uchar mask;
    for(int t = 0; t < block->nblocks; t++) {
        ownbitmap[t] = 0;
    }
    uchar* bitmap = (uchar*)(ptr + BSIZE * BBLOCK(0, block->ninodes));
    for(int i = 0; i < block->ninodes; i++) {
        if(dip->type != 0) {  //checks if inode is in use
            for(int j = 0; j < NDIRECT+1; j++) {  //direct block
                if(dip->addrs[j] != 0) {  //addr in use
                    ownbitmap[dip->addrs[j]] = 1;
                }
            }
            if(dip->addrs[NDIRECT] != 0) {  //indirect block
                indirect = (uint*)(ptr + dip->addrs[NDIRECT]*BSIZE);
                for(int k = 0; k < NINDIRECT; k++) {
                    if(indirect[k] != 0) {
                        ownbitmap[indirect[k]] = 1;
                    }
                }
            }
        }
        dip++;
    }
    for(int n = (block->size - block->nblocks); n < block->nblocks; n++) {
        bitmap = (uchar*)(ptr + BSIZE * BBLOCK(n,block->ninodes));
	byte = bitmap[n/8];
	mask = 1 << (n % 8);
	if(ownbitmap[n] == 1 && (byte & mask) != 0) {
	    //good, in use and marked as such
	}
	else if(ownbitmap[n] == 0 && (byte & mask) == 0) {
	    //unused, marked as such. good
	}
	else if(ownbitmap[n] == 0 && (byte & mask) != 0) {  //checks if bitmap marks block as in use but block is not in use
            //printf("own %d bit %d mask %d\n",n,byte,mask);
	    fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
	    exit(1);
	}
	else {
	    //fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
	    //exit(1);
	}
    }
}

/* Checks that for in-use inodes, each direct address is in use only once
 * Prints an error if not
 */
void directUseCheck(struct superblock* block, struct dinode* dip) {
    int addrUsed[block->ninodes * NDIRECT];
    for(int i = 0; i < block->ninodes; i++) {
        if(dip->type != 0) {  //in use
	    for(int j = 0; j < NDIRECT; j++) {
	        if(dip->addrs[j] != 0) {  //check if in use
                    for(int k = 0; k < ((i*NDIRECT)+j); k++) {  //loops through all addrs so far
		       if(dip->addrs[j] == addrUsed[k]) {  //addr already exists in array, so error
		           fprintf(stderr,"ERROR: direct address used more than once.\n");
			   exit(1);
		       }
		    }
		    //addr not found in array
	            addrUsed[(i*NDIRECT)+j] = dip->addrs[j];  //add to array
	        }
	    } 
	}
	dip++;
    }
}

/*  Checks that the file size in the inode is within the size of the blocks used for storage
 *  Prints an error if the file size is wrong
 */
void fileSizeCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    int blocks = 0;  //counting variable for blocks used
    uint* indirect;
    for(int i = 0; i < block->ninodes; i++) {
	//blocks = 0;
        if(dip->type != 0 && dip->size != 0) {  //in use
            for(int j = 0; j < NDIRECT; j++) {
	        if(dip->addrs[j] != 0) {
	            blocks++;
	        } 
	    }
            if(dip->addrs[NDIRECT] != 0) {  //indirect block in use
		indirect = (uint*)(ptr + dip->addrs[NDIRECT]*BSIZE);
	        for(int k = 0; k < NINDIRECT; k++) {
		    if(indirect[k] != 0) {
		        blocks++;
                    }
	        }
            }
	    //got number of blocks used now
	    //printf("size in inode: %d, calculated block size %d.\n",dip->size,(blocks * BSIZE));
	    if(dip->size > (blocks *BSIZE)) {  //size must be <= blocks * bsize
		//printf("size larger: size in inode: %d, calculated block size %d.\n",dip->size,(blocks * BSIZE));
	        fprintf(stderr,"ERROR: incorrect file size in inode.\n");
		exit(1);
	    }
	    else if(dip->size <= ((blocks-1)*BSIZE)) {  //size must be > b-1 * s
	        //printf("size smaller: size in inode: %d, calculated block size %d.\n",dip->size,(blocks * BSIZE));
	        fprintf(stderr,"ERROR: incorrect file size in inode.\n");
		exit(1);
	    }
	    else {  //good
	    
	    }
	    /*if(dip->size <= (blocks * BSIZE)) {
	    }
	    else if(dip->size > ((blocks-1)*BSIZE)) {
	    }
	    else {  //bad
	        fprintf(stderr,"ERROR: incorrect file size in inode.\n");
                exit(1);
	    }*/
        }
        dip++;
	blocks = 0;
    }
}

/*  Checks if directory marks and inodes in use are consistent.
 *  Throws an error if not
 */
void inodeReferredCheck(struct superblock* block, struct dinode* dip, void* ptr) {
    int* inodesUsed = malloc(block->ninodes*((NDIRECT+NINDIRECT)*(BSIZE/sizeof(struct xv6_dirent))));
    //printf("Made a big array.\n");
    struct xv6_dirent *entry;
    struct dinode* copy = dip;
    uint* indirect;
    int usedSize = 0;
    int found = 0;
    for(int i = 0; i < block->ninodes; i++) {
        if(copy->type == T_DIR) {  //check if inode is directory
	    //printf("Direct block.\n");
	    for(int j = 0; j < NDIRECT; j++) {  //loops through and checks direct block
	        if(copy->addrs[j] != 0) {
		    entry = (struct xv6_dirent *)(ptr + copy->addrs[j] * BSIZE);
		    for(int k = 0; k < BSIZE/sizeof(struct xv6_dirent); k++) {  //goes through entries at addr
                        if(entry[k].inum != 0) {
			    inodesUsed[usedSize] = entry[k].inum;
			    usedSize++;
			}
		    } 
		}
	    }
	    //printf("Indirect block.\n");
	    if(copy->addrs[NDIRECT] != 0) {  //indirect block in use
	        indirect = (uint*)(ptr + copy->addrs[NDIRECT]*BSIZE);
	        for(int m = 0; m < NINDIRECT; m++) {
	            if(indirect[m] != 0) {  //checks if in use
			entry = (struct xv6_dirent *)(ptr + indirect[m]*BSIZE);
		        for(int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {  //goes through entries at addr in indirect block
			    if(entry[n].inum != 0) {
			        inodesUsed[usedSize] = entry[n].inum;
				usedSize++;
			    }
			}
		    }
	        } 
	    }
	}
	copy++;
    }
    //printf("built.\n");
    //directory inums built
    for(int q = 0; q < block->ninodes; q++) {
        if(dip->type != 0) {  //inode in use
            for(int r = 0; r < usedSize; r++) {
	        if(inodesUsed[r] == q) {  //found in directory
		    found = 1;
		}
	    }
	    if(found != 1) {  //if used inode not found in directory
	        fprintf(stderr,"ERROR: inode marked used but not found in a directory.\n");
		exit(1);
	    }
        }
        else {  //not in use
            for(int s = 0; s < usedSize; s++) {
	        if(inodesUsed[s] == q) {  //found inode in directory
		    found = 1;
		}
	    }
	    if(found != 0) {  //if unused inode is found in directory
	        fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
	        exit(1);
	    }
        }
       	found = 0;
	dip++;
    }
    free(inodesUsed);
}

/* Checks that number of links for regular files match the number of times the file is referred to in directories.
 * If not, prints an error.
 */
void referenceCheck(struct superblock* block, struct dinode* dip,void* ptr) {
    /*int* inodesUsed;
    if((inodesUsed = malloc(block->ninodes*((NDIRECT+NINDIRECT)*(BSIZE/sizeof(struct xv6_dirent))))) == NULL) {
        fprintf(stderr,"Malloc Failed in check eleven.\n");
	exit(1);
    }*/
    int refcount[block->ninodes];
    struct xv6_dirent *entry;
    struct dinode* copy = dip;
    uint* indirect;
    //int usedSize = 0;
    //int found = 0;
    /*for(int t = 0; t < (block->ninodes*((NDIRECT+NINDIRECT)*(BSIZE/sizeof(struct xv6_dirent))))/4; t++) {
        inodesUsed[t] = 0;
    }*/
    for(int z = 0; z < block->ninodes; z++) {
        refcount[z] = 0;
    }
    for(int i = 0; i < block->ninodes; i++) {
        if(copy->type == T_DIR) {  //check if inode is directory
            //printf("Direct block.\n");
            for(int j = 0; j < NDIRECT; j++) {  //loops through and checks direct block
                if(copy->addrs[j] != 0) {
                    entry = (struct xv6_dirent *)(ptr + copy->addrs[j] * BSIZE);
                    for(int k = 0; k < BSIZE/sizeof(struct xv6_dirent); k++) {  //goes through entries at addr
                        if((entry[k].inum != 0) && (strcmp(entry[k].name,".") != 0) && (strcmp(entry[k].name,".."))) {
                            //inodesUsed[usedSize] = entry[k].inum;
                            //usedSize++;
			    refcount[entry[k].inum]++;
                        }
                    }
                }
            }
            //printf("Indirect block.\n");
            if(copy->addrs[NDIRECT] != 0) {  //indirect block in use
                indirect = (uint*)(ptr + copy->addrs[NDIRECT]*BSIZE);
                for(int m = 0; m < NINDIRECT; m++) {
                    if(indirect[m] != 0) {  //checks if in use
                        entry = (struct xv6_dirent *)(ptr + indirect[m]*BSIZE);
                        for(int n = 0; n < BSIZE/sizeof(struct xv6_dirent); n++) {  //goes through entries at addr in indirect block
                            if((entry[n].inum != 0) && (strcmp(entry[n].name,".") != 0) && (strcmp(entry[n].name, "..") != 0)) {
                                //inodesUsed[usedSize] = entry[n].inum;
                                //usedSize++;
				refcount[entry[n].inum]++;
                            }
                        }
                    }
                }
            }
        }
        copy++;
    }
    //all dirents in array
    dip++;
    dip++;
    for(int i = 2; i < block->ninodes; i++) {
        //printf("inode %d refcnt %d, checker refcnt %d.\n",i,dip->nlink,refcount[i]);
	if(dip->type == T_FILE) {
	    if(dip->nlink != refcount[i]) {
	        fprintf(stderr,"ERROR: bad reference count for file.\n");
		exit(1);
	    }
	}
	else if(dip->type == T_DIR) {
	    if(refcount[i] != 1) {
	        fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
		exit(1);
	    }
	}
	else {
	
	}
	dip++;
    }
    //free(inodesUsed);
}

/*  Main function
 */
int main(int argc, char *argv[]) {
    int fp;
    if(argc == 2) {  //checks for correct arguments and that file is valid
        //printf("Attempting to open\n");  //debug
        fp = open(argv[1],O_RDONLY);
    }
    else {
        fprintf(stderr,"Usage: xfsck <file_system_image>\n");
	exit(1);
    }
    //printf("opened!\n");  //debug
    if(fp < 0) {
        fprintf(stderr,"image not found.\n");
	exit(1);
    }
    //if file opens correctly
    //
    struct stat sbuf;
    fstat(fp, &sbuf);
    

    //block 0 unused
    //block 1 is superblock
    //block 2 is start of inodes
    
    //gets our pointer to the file contents
    void *img_ptr = mmap(NULL,sbuf.st_size,PROT_READ,MAP_PRIVATE,fp,0);
    
    struct superblock *sb = (struct superblock *)(img_ptr+BSIZE);  //start of superblock

    struct dinode *dinp = (struct dinode *) (img_ptr + 2 * BSIZE);  //start of inodes
    
    //struct xv6_dirent *entryp = (struct xv6_dirent *)(img_ptr + 29 *BSIZE);  //pointer to directory
    //inode 0 is unused again?
    //inode 1 belongs to the root directory
    //inode has 12 direct data blocks and 1 indirect data block
    
    //call checks
    sbSizeCheck(sb);
    typeCheck(sb,dinp);
    addrCheck(sb,dinp,img_ptr);
    directoryCheck(sb,dinp,img_ptr);
    inUseCheck(sb,dinp,img_ptr);
    bitmapCheck(sb,dinp,img_ptr);
    directUseCheck(sb,dinp);
    fileSizeCheck(sb,dinp,img_ptr);
    inodeReferredCheck(sb,dinp,img_ptr);
    referenceCheck(sb,dinp,img_ptr);
}
