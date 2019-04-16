/*
 * headstripper
 * File: main.c
 * (c) 2019 Jonas Gunz himself(at)jonasgunz.de
 *
 * License: MIT
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


//SWAPPED
#define _MAGIC_JPG 0xD8FF //JPG Magic Value
#define _MAGIC_BMP 0x4D42 //BMP Magic Value

//JPG
#define _JPG_EXIF 0xE1
#define _JPG_COM  0xFE
#define _JPG_COPY 0xEE

#define _JPG_SOS  0xDA
#define _JPG_EOI  0xD9

void strip_jpg(char *_filename);

int main(int argc, char* argv[])
{
	FILE *image;
	uint16_t magic;

	if(argc != 2)
	{
		printf("Wrong number of arguments!\nUsage: %s <Filename>\n", argv[0]);
		exit(1);
	}
	
	{
		image = fopen(argv[1], "r");

		if(!image)
		{
			printf("Unable to open %s\n", argv[1]);
			exit(2);
		}

		fread(&magic, 2, 1, image);
		fclose(image);
	}

	printf("Magic Value: %X\n", magic);
	switch(magic)
	{
		case _MAGIC_JPG:
			printf("Detected JPG\n");
			strip_jpg(argv[1]);
			break;
		default:
			printf("Unknown filetype.\n");
			exit(1);
			break;
	};

	return 0;
}

void strip_jpg(char *_filename)
{
	FILE *in;
	FILE *out;

	int f__outfile_len = strlen(_filename);
	char *f__outfile = malloc(f__outfile_len + 8);

	strcpy( f__outfile, _filename);
	strcpy(&f__outfile[f__outfile_len], ".strip\0");

	in = fopen(_filename, "r");
	out = fopen(f__outfile, "w");

	//Feelin' lazy. Might add debug later, idk
	if(!in)
		exit(1);
	if(!out)
		exit(1);
	
	while(1)
	{
		unsigned char c = fgetc(in);
		if(c == 0xFF)
		{
			unsigned char c2 = fgetc(in);
			if( c2 == _JPG_EXIF || c2 == _JPG_COM || c2 == _JPG_COPY ) //Add tags here
			{	
				unsigned char c3 = fgetc(in);
				unsigned char c4 = fgetc(in);
				uint16_t seg_len = (( c3 << 8 ) & 0xff00) + c4;
				printf("SEG %X SEGLEN %uB\n", c2, seg_len);

				seg_len -= 2; //2B are already read!

				for (uint16_t i = 0; i < seg_len; i++)
				{
					fgetc(in);
				}	
			}	
			else if ( c2 == _JPG_EOI ) 
			{
				printf("ENDSEG\n");

				fputc(c, out);
				fputc(c2, out);
				break;
			}
			else
			{
				fputc(c, out);
				fputc(c2, out);
			}
		}
		else
			fputc(c, out);
		if(feof(in))
			break;
	}
		
	fclose(in);
	fclose(out);
	free(f__outfile);	
}
