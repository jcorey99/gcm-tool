/*
 * GCM v0.5beta (c)2007-2008 by dsbomb under the GPLv2 licence.
 *
 * Dec 16, 2007 - v0.1 - first public release.
 * Dec 16, 2007 - v0.2 - add -fs and -sh options
 * Dec 18, 2007 -fs now displays what the shrunken size will be
 * Dec 20, 2007 -sh improved speed, display percentage
 * found some differences between GCMUtility and this with Ikaruga
 * Dec 26, 2007 -crc added to calculate CRC32 of the file, thanks to pycrc
 * Jan  1, 2008 - v0.3 - next release
 * Jan 10, 2008 - v0.4 - fix bootfile offset on shrunken images
 * Jan 15, 2008 -ex added
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include "crc.h"

// is there a max name size?
#define MAXNAMELEN 1024

typedef struct _gcm_fileentry {
	int isdir;
	unsigned long entry;
	unsigned long stringoffset;
	unsigned long offset;
	unsigned long length;
	char path[MAXNAMELEN];
	char name[MAXNAMELEN];
} gcm_fileentry;
// TODO: for now, 10k entries - fix to malloc as needed
gcm_fileentry *GCM_FileList[1024*10];
unsigned int filecount = 0;

void parsedir(unsigned char *buff, unsigned long start, unsigned long end, char *dir);

int main(int argc, char *argv[]) {
	FILE *f;
	unsigned char *buff;
	char gcmfile[1024], small_gcm[1024];
	int do_fs = 0, do_shrink = 0, do_crc = 0, do_ex = 0;

	printf("GCM v0.5beta (c)2007 by dsbomb\n");
	printf("==============================\n");
	if (argc < 2) {
		fprintf(stderr, "Usage: gcm [-fs|-sh|-crc] <gcm filename> [shrink|extract filename]\n");
		fprintf(stderr, "  -fs : Show the file system\n");
		fprintf(stderr, "  -sh : Shrink GCM image\n");
		fprintf(stderr, "  -crc: Calculate CRC32 of GCM image\n");
		fprintf(stderr, "  -ex : Extract file(s) from GCM image\n");
		exit(-1);
	}

	small_gcm[0] = 0;
	if (strcmp(argv[1], "-fs") == 0) {
		do_fs = 1;
		strncpy(gcmfile, argv[2], 1024);
	} else if (strcmp(argv[1], "-sh") == 0) {
		do_shrink = 1;
		strncpy(gcmfile, argv[2], 1024);
		if (argc == 4) {
			strncpy(small_gcm, argv[3], 1024);
		}
	} else if (strncmp(argv[1], "-crc", 1024) == 0) {
		do_crc = 1;
		strncpy(gcmfile, argv[2], 1024);
	} else if (strncmp(argv[1], "-ex", 1024) == 0) {
		do_ex = 1;
		strncpy(gcmfile, argv[2], 1024);
		if (argc == 4) {
			strncpy(small_gcm, argv[3], 1024); // use small_gcm for file/dir to extract
		}
	} else {
		strncpy(gcmfile, argv[1], 1024);
	}

	f = fopen(gcmfile, "rb");
	if (f == NULL) {
		fprintf(stderr, "Error opening %s\n", gcmfile);
		exit(-1);
	}
	buff = malloc(2048);
	fread(buff, 1, 2048, f);

	printf("Filename: %s\n", gcmfile);
	// 0-3: System, Game, Region codes
	printf("Game Code: %c%c%c%c, Region: ", buff[0], buff[1], buff[2], buff[3]);
	if (buff[3] == 'E')
		printf("USA");
	else if (buff[3] == 'P')
		printf("PAL");
	else if (buff[3] == 'J')
		printf("JPN");
	else
		printf("Unknown");
	// 4-5: Game maker code
	printf("\nMaker: %c%c, ", buff[4], buff[5]);
	// 6: Disk ID, 7: Version
	printf("Disk ID: %02X, Version: %02X\n", buff[6], buff[7]);
	// 8: Audio streaming flag, 9: Stream buffer size
	printf("Audio streaming: %s", (buff[8] == 1) ? "Yes" : "No");
	if (buff[8] == 1)
		printf(", Buffer size: %d", buff[9]);
	// 20-3F?: Description
	char name[993];
	memcpy(name, buff+0x20, 992);
	name[993] = 0;
	printf("\nGame Name: %s\n\n", name);

	if (do_crc) {
		crc_t crc;
		crc = crc_init();
		printf("%s CRC: 000.", gcmfile);
		fseek(f, 0, SEEK_END);
		int i = 0, foo = ftell(f);
		fseek(f, 0, SEEK_SET);
		while (!feof(f)) {
			i = fread(buff, 1, 1024*2, f);
			crc = crc_update(crc, buff, i);
			float p = 100.0 * ftell(f) / foo;
			printf("\b\b\b\b%3d%%", (int)p);
		}
		crc = crc_finalize(crc);
		printf("\b\b\b\b%08lX\n", (long)crc);
	} else if (do_fs || do_shrink || do_ex) {
		// 400-403: debug monitor (dh.bin)
	    // 404-407: addr to load debug monitor
		// 408-41F: unused
		// 420-423: offset to main executable DOL (bootfile)
		unsigned long bootfile = ((buff[0x420] * 256 + buff[0x421]) * 256 + buff[0x422]) * 256 + buff[0x423];
		// 424-427: offset to the FST (fst.bin)
		unsigned long fst = ((buff[0x424] * 256 + buff[0x425]) * 256 + buff[0x426]) * 256 + buff[0x427];
		// 428-42B: size of FST
		unsigned long fstsize = ((buff[0x428] * 256 + buff[0x429]) * 256 + buff[0x42A]) * 256 + buff[0x42B];
		// 42C-42F: maximum size of FST
		unsigned long fstmaxsize = ((buff[0x42C] * 256 + buff[0x42D]) * 256 + buff[0x42E]) * 256 + buff[0x42F];
		if (do_fs) {
			printf("Bootfile offset: 0x%08lX\n", bootfile);
			printf("FST offset: 0x%08lX\n", fst);
			printf("FST size: %ld\n", fstsize);
			printf("FST max size: %ld\n\n", fstmaxsize);
		}
		free(buff);  // free header buffer
		buff = malloc(fstsize+2);
		memset(buff, 11, fstsize+2); // set to dummy value

		// fast forward to the FST
		fseek(f, fst, SEEK_SET);
		unsigned long z = fread(buff, 1, fstsize+1, f);  // buff now contains FST
		if ( (z != fstsize+1) || (ferror(f)) || (feof(f)) ) {
			fprintf(stderr, "ERROR reading FST!\n");
			exit(-1);
		}

		unsigned long numentries = ((buff[8] * 256 + buff[9]) * 256 + buff[10]) * 256 + buff[11];
		parsedir(buff, 1, numentries, "");
		free(buff);

		unsigned long totalsize = 0;
		unsigned long shrunksize = fst + fstsize;

		// Extracting files
		int extract_all = 0;
		char ex_path[1024], ex_name[1024], *slash;

		if (do_ex) {
			if (small_gcm[0] == 0) {
				extract_all = 1;
			} else {
				// drop leading '.' for use within mingw bash shell
				if (small_gcm[0] == '.') {
					char tmp[1024];
					strcpy(tmp, small_gcm+1);
					strcpy(small_gcm, tmp);
				}
				slash = strrchr(small_gcm, '/');
				if (slash) {
					strncpy(ex_path, small_gcm, slash-small_gcm);
					ex_path[slash-small_gcm] = 0;
					strncpy(ex_name, slash+1, 1024);
				}
			}
		}

		for(z = 0; z<filecount; z++) {
			if (do_fs) {
				if (GCM_FileList[z]->isdir) {
					printf("%5ld: %s/%s, Parent: %ld, Next: %ld\n", GCM_FileList[z]->entry, GCM_FileList[z]->path,
							GCM_FileList[z]->name, GCM_FileList[z]->offset, GCM_FileList[z]->length);
				} else {
					printf("%5ld: %s/%s, size: %ld, offset: %08lX-%08lX\n", GCM_FileList[z]->entry, GCM_FileList[z]->path,
							GCM_FileList[z]->name, GCM_FileList[z]->length, GCM_FileList[z]->offset,
							GCM_FileList[z]->offset+GCM_FileList[z]->length-1);
				}
			}
			if (GCM_FileList[z]->isdir == 0) {
				totalsize += GCM_FileList[z]->length;
				shrunksize += GCM_FileList[z]->length;
				if (shrunksize % 4) shrunksize += 4 - (shrunksize % 4);
			}
			if (do_ex) {
				// TODO: extracting whole dirs and extracting everything
				if ( /*(extract_all) ||*/ ( (strcmp(GCM_FileList[z]->path, ex_path) == 0) &&
					(strcmp(GCM_FileList[z]->name, ex_name) == 0) ) ) {
					if (!GCM_FileList[z]->isdir) {
						printf("Extracting: %s/%s\n", ex_path, ex_name);
						// TODO: make necessary dirs in ex_path
						if (ex_path[0]) {  // if not root dir
							//int a = mkdir(ex_path);
							//printf("Making dir: %s -> %d\n", ex_path, a);
						}
						FILE *ex_file = fopen(ex_name, "wb");
						if (!feof(ex_file)) {
							fseek(f, GCM_FileList[z]->offset, SEEK_SET);
							unsigned long written = 0, readsize = 1024*8;
							buff = malloc(readsize);
							while (written < GCM_FileList[z]->length) {
								if ( (GCM_FileList[z]->length - written) < readsize ) {
									fread(buff, 1, GCM_FileList[z]->length - written, f);
									written += fwrite(buff, 1, GCM_FileList[z]->length - written, ex_file);
								} else {
									fread(buff, 1, readsize, f);
									written += fwrite(buff, 1, readsize, ex_file);
								}
							} // while
							fclose(ex_file);
							free(buff);
						} // if !feof
					} // if file
				} // if extractable
			} // if do_ex
		} // for each file

		if (do_fs) {
			printf("Total size: %ld, Shrunken size: %ld\n", totalsize, shrunksize);
		}

		// 2440 - Apploader
		buff = malloc(32);  // apploader header
		memset(buff, 11, 32); // set to dummy value

		// fast forward to the Apploader header
		fseek(f, 0x2440, SEEK_SET);
		z = fread(buff, 1, 32, f);  // buff now contains FST
		if ( (z != 32) || (ferror(f)) || (feof(f)) ) {
			fprintf(stderr, "ERROR reading Apploader header!\n");
			exit(-1);
		}
		if (buff[10] != 0) {
			buff[10] = 0;
		}
		unsigned long apploader = ((buff[0x10] * 256 + buff[0x11]) * 256 + buff[0x12]) * 256 + buff[0x13];
		unsigned long appsize = ((buff[0x14] * 256 + buff[0x15]) * 256 + buff[0x16]) * 256 + buff[0x17];
		unsigned long trailsize = ((buff[0x18] * 256 + buff[0x19]) * 256 + buff[0x1A]) * 256 + buff[0x1B];
		if (do_fs) {
			printf("\nApploader: %s, Entry Point: 0x%08lX\n", buff, apploader);
			printf("Apploader size: %ld, ", appsize);
			printf("Trailer size: %ld\n", trailsize);
		}
		free(buff);

		if (do_shrink) {
			FILE *sm;
			char *backslash, *slash;

			if (small_gcm[0] == 0) {
				backslash = strrchr(gcmfile, '\\');
				slash = strrchr(gcmfile, '/');
				if ( (backslash) || (slash) ) {
					char str1[1024], str2[1024];
					if (backslash) {  // Windows style filenames C:\Foo\Whatever.gcm
						strncpy(str1, gcmfile, backslash-gcmfile);
						str1[backslash-gcmfile] = 0;
						strncpy(str2, backslash+1, 1024);
					} else { // Unix style filenames /foo/whatever.gcm
						strncpy(str1, gcmfile, slash-gcmfile+1);
						str1[slash-gcmfile+1] = 0;
						strncpy(str2, slash+1, 1024);
					}
					sprintf(small_gcm, "%ssmall-%s", str1, str2);
				} else {
					sprintf(small_gcm, "small-%s", gcmfile);
				}
			}
			sm = fopen(small_gcm, "wb");
			if (ferror(sm)) {
				fprintf(stderr, "ERROR creating %s", small_gcm);
				exit(-1);
			}
			printf("Writing to %s: ....", small_gcm);

			// copy everything before the FST
			buff = malloc(fst);
			fseek(f, 0, SEEK_SET);
			z = fread(buff, 1, fst, f);
			if ( (z != fst) || (ferror(f)) || (feof(f)) ) {
				fprintf(stderr, "ERROR reading first block!\n");
				exit(-1);
			}

			fwrite(buff, 1, fst, sm);
			char c = 0;
			while (z % 4) {
				fwrite(&c, 1, 1, sm);  // pad to 4 bytes
				z++;
			}
			// z now is the offset to put FST
			unsigned long offset = z + fstsize;  // first available offset for files
			if (offset % 4) offset += 4 - (offset % 4);

			int i;
			unsigned char *entry = malloc(12);
			// write root entry, same as original
			fread(entry, 1, 12, f);
			fwrite(entry, 1, 12, sm);
			z += 12;
			unsigned long newbootfile = 0;
			for(i=0; i<filecount; i++) {
				entry[0] = GCM_FileList[i]->isdir;
				entry[1] = GCM_FileList[i]->stringoffset >> 16;
				entry[2] = (GCM_FileList[i]->stringoffset >> 8) & 0xFF;
				entry[3] = GCM_FileList[i]->stringoffset & 0xFF;
				if (GCM_FileList[i]->isdir) {
					// Just copy data for directories
					entry[4] = GCM_FileList[i]->offset >> 24;
					entry[5] = (GCM_FileList[i]->offset >> 16) & 0xFF;
					entry[6] = (GCM_FileList[i]->offset >> 8) & 0xFF;
					entry[7] = GCM_FileList[i]->offset & 0xFF;
					//memcpy(entry+4, &GCM_FileList[i]->offset, 4);
					entry[8] = GCM_FileList[i]->length >> 24;
					entry[9] = (GCM_FileList[i]->length >> 16) & 0xFF;
					entry[10] = (GCM_FileList[i]->length >> 8) & 0xFF;
					entry[11] = GCM_FileList[i]->length & 0xFF;
					//memcpy(entry+8, &GCM_FileList[i]->length, 4);
				} else {
					// Adjust to new offset for files
					//printf("%5d: %s, Old Offset: 0x%08lX, New Offset: 0x%08lX\n", i, GCM_FileList[i]->name, GCM_FileList[i]->offset, offset);
					// check if this is the bootfile
					if (GCM_FileList[i]->offset == bootfile) {
						newbootfile = offset;
					}
					entry[4] = offset >> 24;
					entry[5] = (offset >> 16) & 0xFF;
					entry[6] = (offset >> 8) & 0xFF;
					entry[7] = offset & 0xFF;
					//memcpy(entry+4, &offset, 4);
					entry[8] = GCM_FileList[i]->length >> 24;
					entry[9] = (GCM_FileList[i]->length >> 16) & 0xFF;
					entry[10] = (GCM_FileList[i]->length >> 8) & 0xFF;
					entry[11] = GCM_FileList[i]->length & 0xFF;
					//memcpy(entry+8, &GCM_FileList[i]->length, 4);
					offset += GCM_FileList[i]->length;
					if (offset % 4) offset += 4 - (offset % 4);
				}
				fwrite(entry, 1, 12, sm);
				z += 12;
			}
			free(entry);

			// write string table
			int foo = fstsize - (numentries * 12);
			fseek(f, fst + (numentries * 12), SEEK_SET);
			entry = malloc(foo);
			fread(entry, 1, foo, f);
			fwrite(entry, 1, foo, sm);
			z += foo;
			free(entry);

			c = 0;
			while (z % 4) {
				fwrite(&c, 1, 1, sm);
				z++;
			}
			offset = z;
			free(buff);

			unsigned long readsize = 1024 * 32;
			buff = malloc(readsize);
			//printf("%s: 000.", small_gcm);
			for(i=0; i<filecount; i++) {
				if (GCM_FileList[i]->isdir == 0) {
					//printf("File: %s, Old Offset: %08lX, New Offset: %08lX\n", GCM_FileList[i]->name, GCM_FileList[i]->offset, offset);
					fseek(f, GCM_FileList[i]->offset, SEEK_SET);
					//int t;
					unsigned long written = 0;
					while (written < GCM_FileList[i]->length) {
						if ( (GCM_FileList[i]->length - written) < readsize ) {
							fread(buff, 1, GCM_FileList[i]->length - written, f);
							written += fwrite(buff, 1, GCM_FileList[i]->length - written, sm);
						} else {
							fread(buff, 1, readsize, f);
							written += fwrite(buff, 1, readsize, sm);
						}
						float p = 100.0 * (z+written) / shrunksize;
						printf("\b\b\b\b%3d%%", (int)p);
					}
					z += GCM_FileList[i]->length;
					c = 0;
					while (z % 4) {
						fwrite(&c, 1, 1, sm);  // pad to 4 bytes
						z++;
					}
					offset = z;
				} // if a file
			} // for each file

			// fix bootfile offset
			if ( (bootfile != newbootfile) && (newbootfile != 0) ) {
				buff[0] = newbootfile >> 24;
				buff[1] = (newbootfile >> 16) & 0xFF;
				buff[2] = (newbootfile >> 8) & 0xFF;
				buff[3] = newbootfile & 0xFF;
				fseek(sm, 0x420, SEEK_SET);
				fwrite(buff, 1, 4, sm);
			}
			fclose(sm);
			//free(buff);
			printf("\n");
		} // if do_shrink
	} // if do_fs or do_shrink or do_ex

	free(buff);
	return 0;
}

/*
 * parsedir: parse the dir structure, filling GCM_FileList[]
 * buff: loaded with FST
 * TODO: check for proper start/end boundaries
 */
void parsedir(unsigned char *buff, unsigned long start, unsigned long end, char *dir) {
	unsigned int entry, i;
	char name[MAXNAMELEN];
	name[0] = 0;
	unsigned long length = 0, offset = 0;
	unsigned long numentries = ((buff[8] * 256 + buff[9]) * 256 + buff[10]) * 256 + buff[11];
	char *stringtable = buff + numentries * 12;
	char curdir[1024];

	strncpy(curdir, dir, 1024);
	i = start * 12;
	for (entry = start; entry < end; entry++) {
		offset = ((buff[i+4] * 256 + buff[i+5]) * 256 + buff[i+6]) * 256 + buff[i+7];
		length = ((buff[i+8] * 256 + buff[i+9]) * 256 + buff[i+10]) * 256 + buff[i+11];
		unsigned long stringoffset = (buff[i+1] * 256 + buff[i+2]) * 256 + buff[i+3];
		// Check if this entry is already in the GCM_FileList
		int zz, res=0;
		for(zz=0; zz<filecount; zz++) {
			if (GCM_FileList[zz]->entry == entry) {
				res++;
				break;
			}
		}
		if (res == 0) {
			gcm_fileentry *f = malloc(sizeof(gcm_fileentry));
			f->isdir = 0;
			f->entry = entry;
			f->stringoffset = stringoffset;
			f->offset = offset;
			f->length = length;
			strncpy(f->name, (char*) (stringtable+stringoffset), MAXNAMELEN);
			strncpy(f->path, dir, MAXNAMELEN);
			GCM_FileList[filecount++] = f;
			if (buff[i] == 1) {
				f->isdir = 1;
				sprintf(curdir, "%s/%s", f->path, f->name);
				parsedir(buff, entry+1, length, curdir);
			}
		}
		i += 12;
	} // for
}
