#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <png.h>

#define PNG_SETJMP_NOT_SUPPORTED

#define MAX_CHARS 50000

struct png_image {
	int width;
	int height;
	int channels;
	char bit_depth;
};

png_bytep *row_pointers;  // Needs to be global


void init_png_image(
	struct png_image* img, int width,
	int height, int channels, char bit_depth) {
	img->width = width;
	img->height = height;
	img->channels = channels;
	img->bit_depth = bit_depth;
}

void allocate_row_bytes(
	struct png_image* img,
	png_structp* png_ptr,
	png_infop* info_ptr) {
	
	row_pointers =
		(png_bytep*) malloc(sizeof(png_bytep) * img->height);
	int i;
	for(i=0; i < img->height; i++)
		row_pointers[i] = NULL; // security precaution
	for(i=0; i < img->height; i++)
		row_pointers[i] = (png_byte*) malloc(
			png_get_rowbytes(*png_ptr, *info_ptr));
}

void free_row_bytes(int height) {
	int i;
	for (i=0; i < height; i++)
		free(row_pointers[i]);
	free(row_pointers);
}

void write_new_img(char* out_name, png_infop *info_ptr) {
	FILE *fwp = fopen(out_name, "wb");
	
	png_structp png_ptr = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL,
		NULL,
		NULL);
	if(!png_ptr)
		exit(1);

	png_init_io(png_ptr, fwp);
	// (, , , png_transforms, )
	png_write_png(png_ptr, *info_ptr, 0, NULL);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, *info_ptr);

	png_destroy_write_struct(&png_ptr, NULL);
	fclose(fwp);

}

void decode(char** bytes, struct png_image* img) {
	int i;
	int j;
	int zeroes_in_row = 0;
	int z = 0;
	int k = 7;
	char lsb;
	char current_char = 0;
	for (i=0; i < img->height; i++) {
		for (j=0; j < img->channels * img->width; j++) {
			lsb = (row_pointers[i][j]) & 1;
			if (!lsb)
				zeroes_in_row++;
			else
				zeroes_in_row = 0;
			
			if (zeroes_in_row == 8) { 
				// rewinding array of characters
				for (z; z > 0; z--)
					*(*bytes)--;
				return;
			}

			current_char ^= (-lsb ^ current_char) & (1 << k--);
			if (k < 0) {
				k = 7;
				*(*bytes)++ = current_char;
				z++;
				current_char = 0;
			}
		}
	}
}

void write_null_byte(int i, int j, int height, int actual_width) {
	int k = 0;
	for (i; i < height; i++) {
		for (j; j < actual_width; j++) {
			row_pointers[i][j] = row_pointers[i][j] & 0xFE | 0;
			if (++k >= 7)
				return;
		}
	}
}

void write_sequence_of_bytes_to_image(
	struct png_image* img,
	char** bytes,
	int seq_size) {

	int i;
	int j;
	int k = 7;
	int z = 0;
	char current_byte;
	char x;
	for (i=0; i < img->height; i++) {
		for (j=0; j < img->channels * img->width; j++) {
			if (z == seq_size) {
				write_null_byte(
					i, j, img->height, img->channels *img->width);
				return;
			}
			current_byte = **bytes;
			x = ((current_byte >> k--) & 0x01);
			row_pointers[i][j] = row_pointers[i][j] & 0xFE | x;
			if (k < 0) {
				k = 7;
				z++;
				*(*bytes)++;
			}
		}
	}
}

void clean(
	FILE **p_img,
	png_structp* png_ptr,
	png_infop* info_ptr, png_infop* end_info,
	int height) {
	
	free_row_bytes(height);
	png_destroy_read_struct(png_ptr, info_ptr, end_info);
	fclose(*p_img);
}

void read_png_image(
	struct png_image* img,
	png_structp* png_ptr,
	png_infop* info_ptr,
	png_infop *end_info) {

	int width = png_get_image_width(*png_ptr, *info_ptr);
	int height = png_get_image_height(*png_ptr, *info_ptr);
	int channels = png_get_channels(*png_ptr, *info_ptr);
	int bit_depth = png_get_bit_depth(*png_ptr, *info_ptr);
	
	init_png_image(img, width, height, channels, bit_depth);
	
	allocate_row_bytes(img, png_ptr, info_ptr);
	
	png_read_image(*png_ptr, row_pointers);

	png_set_rows(*png_ptr, *info_ptr, row_pointers);
	png_read_end(*png_ptr, *end_info);
}

void set_up(
	FILE **p_img,
	char* name,
	png_structp *png_ptr,
	png_infop *info_ptr,
	png_infop *end_info) {

	*p_img = fopen(name, "rb");
	char header[8];    // 8 is the max size that can be checked
	fread(header, 1, 8, *p_img);
	if(png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "%s\n", "Its not PNG");
		exit(1);
	}
	
	*png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL,
		NULL,
		NULL);

	if (!(*png_ptr))
		exit(1);
	
	*info_ptr = png_create_info_struct(*png_ptr);
	if (!(*info_ptr)) {
		png_destroy_read_struct(
			png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		exit(1);
	}
	
	*end_info = png_create_info_struct(*png_ptr);
	if (!(end_info)) {
		png_destroy_read_struct(
			png_ptr, info_ptr,(png_infopp)NULL);
		exit(1);
	}

	png_init_io(*png_ptr, *p_img);
	png_set_sig_bytes(*png_ptr, 8);

	png_read_info(*png_ptr, *info_ptr);
}

int main(int argc, char** argv) {
	
	FILE *p_img;
	png_structp png_ptr;
	png_infop info_ptr; 
	png_infop end_info;

	struct png_image img;
	
	
	if (strcmp(argv[1], "u") == 0) {  // unhide text
		
		set_up(&p_img, argv[2], &png_ptr, &info_ptr, &end_info);
		read_png_image(&img, &png_ptr, &info_ptr, &end_info);
		
		char* decoded = malloc(sizeof(char) * MAX_CHARS);
		decode(&decoded, &img);
		printf("%s\n", decoded);
		free(decoded);
	} else if (strcmp(argv[1],"h") == 0) {  // hide text
		
		set_up(&p_img, argv[2], &png_ptr, &info_ptr, &end_info);
		read_png_image(&img, &png_ptr, &info_ptr, &end_info);
		
		write_sequence_of_bytes_to_image(
			&img, &argv[4], strlen(argv[4]));
		write_new_img(argv[3], &info_ptr);
	} else {
		fprintf(stderr, "%s", "Invalid input\n");
		exit(1);
	}
	clean(&p_img, &png_ptr, &info_ptr, &end_info, img.height);
	

// compile without error handling for now:
// PNG_SETJMP_NOT_SUPPORTED
}
