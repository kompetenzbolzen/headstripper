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
#include <errno.h>


//Magic values
#define _MAGIC_JPG 0xD8FF //JPG Magic Value
#define _MAGIC_PNG 0x5089
#define _MAGIC_PNG_FULL 0x0A1A0A0D474E5089 //89 50 4E 47 0D 0A 1A 0A
#define _MAGIC_TFF 0x2A00 //TIFF Magic Value

//JPG
#define _JPG_EXIF 0xE1 //EXIF
#define _JPG_COM  0xFE //Comments
#define _JPG_COPY 0xEE //Copyright notice

#define _JPG_SOS  0xDA //Start of image data
#define _JPG_EOI  0xD9 //End of image

//PNG
#define _PNG_IHDR 0x52444849 
#define _PNG_IDAT 0x54414449
#define _PNG_IEND 0x444E4549
#define _PNG_PLTE 0x45544C50
#define _PNG_tRNS 0x534e5274
#define _PNG_gAMA 0x414d4167
#define _PNG_cHRM 0x4d524863
#define _PNG_sRGB 0x42475273
#define _PNG_iCCP 0x50434369
#define _PNG_sBIT 0x54494273

int strip_jpg(char *_filename);

int strip_png(char *_filename);

int strip_tiff(char *_filename);

int main(int argc, char* argv[])
{
	FILE *image;
	uint16_t magic;

	if(argc != 2)
	{
		printf("Wrong number of arguments!\nUsage: %s <Filename>\n", argv[0]);
		exit(1);
	}
	
	//Read magic val
	image = fopen(argv[1], "r");
	if(!image)
	{
		printf("Unable to open \"%s\": %i\n", argv[1], errno);
		exit(2);
	}

	fread(&magic, 2, 1, image);
	fclose(image);

	switch(magic)
	{
		case _MAGIC_JPG:
			printf("Detected JPG\n");
			strip_jpg(argv[1]);
			break;
		case _MAGIC_PNG:
			printf("Detected PNG\n");
			strip_png(argv[1]);
			break;
		case _MAGIC_TFF:
			printf("Detected TIFF\n");
			strip_tiff(argv[1]);
			break;
		default:
			printf("Bad/Unknown Filetype\n");
			exit(1);
			break;
	};

	return 0;
}

//This function assumes a valid JFIF container file! No file validation is performed
int strip_jpg(char *_filename)
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
		return 1;
	if(!out) {
		printf("Unable to open for writing %s: %i\n", f__outfile, errno);
		return 1;
	}
	
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

				for (uint16_t i = 0; i < seg_len; i++) {
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

	return 0;
}

int strip_png(char *_filename)
{

	FILE *in;
	FILE *out;

	int f__outfile_len = strlen(_filename);
	char *f__outfile = malloc(f__outfile_len + 8);

	strcpy( f__outfile, _filename);
	strcpy(&f__outfile[f__outfile_len], ".strip\0");

	in = fopen(_filename, "r");
	out = fopen(f__outfile, "w");

	if(!in)
		return 1;
	if(!out) {
		printf("Unable to open for writing %s: %i\n", f__outfile, errno);
		return 1;
	}
	
	uint64_t magic_num = _MAGIC_PNG_FULL;
	fwrite(&magic_num, 1, 8, out);

	uint32_t read_buff = 0;
	uint32_t chunk_len = 0;

	while(1)
	{
		//Heavy shifting
		//Chunk header is 4B
		chunk_len = ((chunk_len << 8) & 0xFFFFFF00) + ( read_buff & 0x000000FF );

		unsigned char c = fgetc(in);
		read_buff = ((read_buff >> 8) & 0x00FFFFFF) + (((uint32_t)c << 24) & 0xFF000000);
	
		//Whitelist only important PNG chunks
		if(	read_buff == _PNG_IHDR || 
			read_buff == _PNG_IDAT ||
			read_buff == _PNG_PLTE ||
		//	read_buff == _PNG_sBIT ||
		//	read_buff == _PNG_tRNS ||
		//	read_buff == _PNG_gAMA ||
		//	read_buff == _PNG_cHRM ||
		//	read_buff == _PNG_sRGB ||
		//	read_buff == _PNG_iCCP ||
		//	read_buff == _PNG_sRGB ||
			read_buff == _PNG_IEND	)
		{
			char *f__buffer = malloc( chunk_len );
			uint32_t crc;

			//*Ghetto music plays*
			uint64_t chunk_str = 0;
			chunk_str += read_buff;	
			printf("CHUNK %s SIZE %uB\n", &chunk_str, chunk_len);

			fread(f__buffer, 1, chunk_len, in);
			fread(&crc, 1, 4, in);

			//Flip chunk_len...
			uint32_t lencpy = chunk_len;
			for (int i = 0; i < 4; i++)
			{
				fputc(lencpy >> 24, out);
				lencpy <<= 8;
			}
			//fwrite(&chunk_len, 1, sizeof(chunk_len), out); //IS flipped!

			fwrite(&read_buff, 1, sizeof(read_buff), out);
			fwrite(f__buffer, 1, chunk_len, out);
			fwrite(&crc, 1, 4, out);

			free(f__buffer);

			if(read_buff == _PNG_IEND)
				break;
		}

		if(feof(in))
			break;	
	}


	fclose(in);
	fclose(out);
	free(f__outfile);

	return 1;
}

int strip_tiff(char *_filename)
{

	FILE *in;
	FILE *out;

	int f__outfile_len = strlen(_filename);
	char *f__outfile = malloc(f__outfile_len + 8);

	strcpy( f__outfile, _filename);
	strcpy(&f__outfile[f__outfile_len], ".strip\0");

	in = fopen(_filename, "r");
	out = fopen(f__outfile, "w");

	if(!in)
		return 1;
	if(!out) {
		printf("Unable to open for writing %s: %i\n", f__outfile, errno);
		return 1;
	}

	printf("Unimplemented\n");

	fclose(in);
	fclose(out);
	free(f__outfile);

	return 1;
}
