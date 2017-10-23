#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <png.h>

#define PNG_SETJMP_NOT_SUPPORTED

#define NAME "img/k.png"
#define OUT_NAME "img/out.png"

struct png_image {
	int width;
	int height;
	int channels;
};


png_bytep *row_pointers;  // Needs to be global

void init_png_image(
	struct png_image* img, int width, int height, int channels) {
	img->width = width;
	img->height = height;
	img->channels = channels;
}

void open_check_img(FILE *p_img) {
	p_img = fopen(NAME, "rb");
	assert(p_img != NULL);
}

void write_new_img(png_infop *info_ptr) {
	FILE *fwp = fopen(OUT_NAME, "wb");
	
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

void write_sequence_of_bytes_to_image(
	struct png_image* img,
	char** bytes,
	int seq_size) {

	int i;
	int j;
	for (i=0; i < img->height; i++) {
		for (j=0; j < img->channels * img->width; j++) {
			row_pointers[i][j] ^= (1 << 6); 
		}
	}
}

int main(void) {
	
	FILE *p_img;
	p_img = fopen(NAME, "rb");

	char header[8];    // 8 is the max size that can be checked
	fread(header, 1, 8, p_img);
	if(png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "%s\n", "Its not PNG");
		return(1);
	}
	
	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL,
		NULL,
		NULL);

	if (!png_ptr)
		exit(1);

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(
			&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		exit(1);
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info) {
		png_destroy_read_struct(
			&png_ptr, &info_ptr,(png_infopp)NULL);
		exit(1);
	}

	png_init_io(png_ptr, p_img);
	png_set_sig_bytes(png_ptr, 8);

	// (, , , png_transforms, )
	png_read_info(png_ptr, info_ptr);

	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	int channels = png_get_channels(png_ptr, info_ptr);
	// int number_of_passes = png_set_interlace_handling(png_ptr);
	// png_read_update_info(png_ptr, info_ptr);
	
	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	int i;
	for(i=0; i < height; i++)
		row_pointers[i] = NULL; // security precaution
	for(i=0; i < height; i++)
		row_pointers[i] = (png_byte*) malloc(
			png_get_rowbytes(png_ptr, info_ptr));
	
	png_read_image(png_ptr, row_pointers);

	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_read_end(png_ptr, end_info);

	struct png_image img;
	init_png_image(&img, width, height, channels);
	
	char* bytes;

	write_sequence_of_bytes_to_image(&img, &bytes, 0);
	
	write_new_img(&info_ptr);

	// Clean
	for (i=0; i < height; i++)
		free(row_pointers[i]);
	free(row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(p_img);

// compile without error handling for now:
// PNG_SETJMP_NOT_SUPPORTED
}
