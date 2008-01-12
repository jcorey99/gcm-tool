/*
 * GCM v0.1 (c)2007 by dsbomb under the GPLv2 licence.
 * 
 * Dec 16, 2007 - v0.1 - first public release.
 *
 */

#include <stdio.h>

int main(int argc, char *argv[]) {
   FILE *f;
   char buff[1024];

   printf("GCM v0.1 (c)2007 by dsbomb\n");
   printf("==========================\n");
   if (argc != 2) {
      printf("Usage: gcm <filename>\n");
      exit(-1);
   }

   f = fopen(argv[1], "r");
   fgets(buff, 1024, f);
   // 0-3: System, Game, Region codes
   printf("Game Code: %c%c%c%c, Region: ", buff[0], buff[1], buff[2], buff[3]);
   if (buff[3] == 'E') printf("USA");
   else if (buff[3] == 'P') printf("PAL");
   else if (buff[3] == 'J') printf("JPN");
   else printf("Unknown");
   // 4-5: Game maker code
   printf("\nMaker: %c%c, ", buff[4], buff[5]);
   // 6: Disk ID, 7: Version
   printf("Disk ID: %02X, Version: %02X\n", buff[6], buff[7]);
   // 8: Audio streaming flag, 9: Stream buffer size
   printf("Audio streaming: %s", (buff[8] == 1) ? "Yes" : "No");
   if (buff[8] == 1) printf(", Buffer size: %d", buff[9]);
   // 20-3F?: Description
   char name[993];
   memcpy(name, buff+0x20, 992);
   name[993] = 0;
   printf("\nGame Name: %s\n", name);
}
