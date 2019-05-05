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

#define _VERSION 0
#define _SUBVERSION 3

#ifdef _DEBUG
#warning "Compiling in DEBUG mode"
#endif

#ifdef _DEBUG
#define DEBUG_PRINTF( ... ) { printf ( __VA_ARGS__ ); }
#else
#define DEBUG_PRINTF( ... ) { }
#endif

//Magic values
#define _MAGIC_JPG 0xD8FF //JPG Magic Value
#define _MAGIC_PNG 0x5089
#define _MAGIC_PNG_FULL 0x0A1A0A0D474E5089 //89 50 4E 47 0D 0A 1A 0A
#define _MAGIC_TFF 0x2A00 //TIFF Magic Value

//JPG
#define _JPG_SOI  0xFFD8
#define _JPG_APP0 0xFFE0
#define _JPG_CX	  0xFFC0
#define _JPG_DQT  0xFFDB
#define _JPG_DRI  0xFFDD
#define _JPG_SOS  0xFFDA //Start of image data
#define _JPG_EOI  0xFFD9 //End of image

//Stuff to ignore
#define _JPG_EXIF 0xFFE1 //EXIF
#define _JPG_COM  0xFFFE //Comments
#define _JPG_COPY 0xFFEE //Copyright notice


//PNG
//Essential chunks
#define _PNG_IHDR 0x52444849 
#define _PNG_IDAT 0x54414449
#define _PNG_IEND 0x444E4549
#define _PNG_PLTE 0x45544C50
//Aux chunks
#define _PNG_tRNS 0x534e5274
#define _PNG_gAMA 0x414d4167
#define _PNG_cHRM 0x4d524863
#define _PNG_sRGB 0x42475273
#define _PNG_iCCP 0x50434369
#define _PNG_sBIT 0x54494273

int strip_jpg(char *_filename);

int strip_png(char *_filename);

int strip_tiff(char *_filename);

//String combine
char* strcmb(char *_str1, char *_str2);

int main(int argc, char* argv[])
{
	FILE *image;
	uint16_t magic;

	if(argc < 2)
	{
		printf("Wrong number of arguments!\nUsage: %s <Filename(s)>\n", argv[0]);
		exit(1);
	}
	
	printf("headstripper v%i.%i\n", _VERSION, _SUBVERSION);
	DEBUG_PRINTF("==DEBUG BUILD==\n");

	for(unsigned int cntr = 1; cntr < argc; cntr++)
	{
		char *filename;

		if (argv[cntr][0] == '"')
		{
			unsigned int len = strlen(&argv[cntr][1]);
			filename = malloc(len);
			strcpy(filename, &argv[cntr][1]);
			filename[len - 2] = '\0';
		}
		else
			filename = argv[cntr];

		//Read magic val
		image = fopen(filename, "rb");
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
				printf("%s: JPG\n", filename);
				strip_jpg(filename);
				break;
			case _MAGIC_PNG:
				printf("%s: PNG\n", filename);
				strip_png(filename);
				break;
			case _MAGIC_TFF:
				printf("%s: TIFF\n", filename);
				strip_tiff(filename);
				break;
			default:
				printf("%s: Bad/Unknown Filetype\n", filename);
				exit(1);
				break;
		};
	}

	return 0;
}

//This function assumes a valid JFIF container file! No file validation is performed
int strip_jpg(char *_filename)
{
	FILE *in;
	FILE *out;

	char *f__outfile = strcmb("strip.", _filename);

	in = fopen(_filename, "rb");
	out = fopen(f__outfile, "wb");

	//Feelin' lazy. Might add debug later, idk
	if(!in)
		return 1;
	if(!out) {
		printf("Unable to open for writing %s: %i\n", f__outfile, errno);
		return 1;
	}

	uint16_t magic_num = 0xD8FF;
	fwrite(&magic_num, 1, 2, out);

	uint16_t segment = 0xFFD8;

	while(1)
	{
		unsigned char c = fgetc(in);
		segment = (segment << 8) + (c & 0x00FF);
		if(segment == _JPG_APP0 ||
		   segment == _JPG_DQT  ||
		   segment == _JPG_DRI  ||
		  (segment & 0xFFF0) == (_JPG_CX & 0xFFF0))
		{
			unsigned char l1 = fgetc(in);
			unsigned char l2 = fgetc(in);
			uint16_t seg_len = (( l1 << 8 ) & 0xff00) + l2;

			DEBUG_PRINTF("Segment %X, size %uB @%liB\n", segment, seg_len, ftell(in));

			char *f__buffer = malloc(seg_len - 2);

			fread(f__buffer, 1, seg_len - 2, in);
			fputc(segment >> 8, out);
			fputc(segment, out);
			fputc(seg_len >> 8, out);
			fputc(seg_len, out);
			fwrite(f__buffer, 1, seg_len - 2, out);

			free(f__buffer);
		}
		else if(segment == _JPG_EXIF ||
			segment == _JPG_COM  ||
			segment == _JPG_COPY ||
			(((segment & 0xFFF0) == (_JPG_APP0 & 0xFFF0)) && ((segment & 0x000F) > 1)))
		{
			unsigned char l1 = fgetc(in);
			unsigned char l2 = fgetc(in);
			uint16_t seg_len = (( l1 << 8 ) & 0xff00) + l2;

			DEBUG_PRINTF("Ignoring segment %X, size %uB @%liB\n", segment, seg_len, ftell(in));

			char *f__buffer = malloc(seg_len - 2);

			fread(f__buffer, 1, seg_len - 2, in);
			free(f__buffer);

		}
		else if(segment == _JPG_SOS)
		{
			DEBUG_PRINTF("SOS @%li\n", ftell(in));
			uint16_t endseg = _JPG_SOS;
			fputc(segment >> 8, out);
			fputc(segment, out);
			while(1)
			{
				unsigned char cc = fgetc(in);
				endseg = (endseg << 8) + (cc & 0x00FF);
				if(endseg == _JPG_EOI)
					break;
				else
					fputc(cc, out);
			}
			DEBUG_PRINTF("ENDSEG @%li\n", ftell(in));
			fwrite(&endseg, 1, 2, out);
			break;
		}
		if(feof(in))
		{
			DEBUG_PRINTF("Reached EOF before ENDSEG @%li\n", ftell(in));
			break;
		}
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

	char *f__outfile = strcmb("strip.", _filename);

	in = fopen(_filename, "rb");
	out = fopen(f__outfile, "wb");

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

#ifdef _DEBUG
			//*Ghetto music plays*
			uint64_t chunk_str = 0;
			chunk_str += read_buff;	
			printf("CHUNK %s SIZE %uB @%li\n", (char*)(&chunk_str), chunk_len, ftell(in) );
#endif
			fread(f__buffer, 1, chunk_len, in);
			fread(&crc, 1, 4, in);

			//Flip chunk_len...
			uint32_t lencpy = chunk_len;
			for (int i = 0; i < 4; i++)
			{
				fputc(lencpy >> 24, out);
				lencpy <<= 8;
			}

			fwrite(&read_buff, 1, sizeof(read_buff), out);
			fwrite(f__buffer, 1, chunk_len, out);
			fwrite(&crc, 1, 4, out);

			free(f__buffer);

			if(read_buff == _PNG_IEND)
				break;
		}

		if(feof(in))
		{
			DEBUG_PRINTF("Reached EOF before IEND @%li\n", ftell(in) );
			break;
		}
	}

	fclose(in);
	fclose(out);
	free(f__outfile);

	return 1;
}

int strip_tiff(char *_filename)
{

	printf("TIFF Unimplemented.\n");
	return 1;
}

char* strcmb(char *_str1, char *_str2)
{
	char *f__return = malloc(strlen(_str1) + strlen(_str2) + 1);

	strcpy( f__return, _str1);
	strcpy(&f__return[ strlen(_str1) ], _str2);

	return f__return;
}
