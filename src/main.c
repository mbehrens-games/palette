/*******************************************************************************
** PALETTE (palette file generation) - Michael Behrens 2018-2022
*******************************************************************************/

/*******************************************************************************
** main.c
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI      3.14159265358979323846f
#define TWO_PI  6.28318530717958647693f

typedef struct color
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} color;

enum
{
  SOURCE_APPROX_NES = 0,
  SOURCE_APPROX_NES_ROTATED,
  SOURCE_COMPOSITE_06_0p75X,
  SOURCE_COMPOSITE_06_3X,
  SOURCE_COMPOSITE_12_1p50X,
  SOURCE_COMPOSITE_12_6X,
  SOURCE_COMPOSITE_18_1X,
  SOURCE_COMPOSITE_24_0p75X,
  SOURCE_COMPOSITE_24_3X,
  SOURCE_COMPOSITE_36_2X,
  SOURCE_COMPOSITE_48_1p50X
};

enum
{
  HUE_MODIFIER_FULL = 0,
  HUE_MODIFIER_LOWER_HALF,
  HUE_MODIFIER_UPPER_HALF
};

/* the table step is 1 / (n + 2), where */
/* n is the number of colors per hue    */
#define COMPOSITE_06_TABLE_STEP 0.125f              /* 1/8  */
#define COMPOSITE_12_TABLE_STEP 0.071428571428571f  /* 1/14 */
#define COMPOSITE_18_TABLE_STEP 0.05f               /* 1/20 */
#define COMPOSITE_24_TABLE_STEP 0.038461538461538f  /* 1/26 */
#define COMPOSITE_36_TABLE_STEP 0.026315789473684f  /* 1/38 */
#define COMPOSITE_48_TABLE_STEP 0.02f               /* 1/50 */

/* the luma is the average of the low and high voltages */
/* for the 1st half of each table, the low value is 0   */
/* for the 2nd half of each table, the high value is 1  */
/* the saturation is half of the peak-to-peak voltage   */

/* for the nes tables, the numbers were obtained    */
/* from information on the nesdev wiki              */
/* (see the "NTSC video" and "PPU palettes" pages)  */
float S_nes_p_p[4] = {0.399f,   0.684f, 0.692f, 0.285f};
float S_nes_lum[4] = {0.1995f,  0.342f, 0.654f, 0.8575f};
float S_nes_sat[4] = {0.1995f,  0.342f, 0.346f, 0.1425f};

float S_approx_nes_p_p[4] = {0.4f, 0.7f,  0.7f,   0.3f};
float S_approx_nes_lum[4] = {0.2f, 0.35f, 0.65f,  0.85f};
float S_approx_nes_sat[4] = {0.2f, 0.35f, 0.35f,  0.15f};

float S_composite_06_lum[6];
float S_composite_06_sat[6];

float S_composite_12_lum[12];
float S_composite_12_sat[12];

float S_composite_18_lum[18];
float S_composite_18_sat[18];

float S_composite_24_lum[24];
float S_composite_24_sat[24];

float S_composite_36_lum[36];
float S_composite_36_sat[36];

float S_composite_48_lum[48];
float S_composite_48_sat[48];

color*  G_colors_array;
int     G_num_colors;
int     G_max_colors;

int     G_source;

float*  S_luma_table;
float*  S_saturation_table;
int     S_table_length;

/*******************************************************************************
** generate_voltage_tables()
*******************************************************************************/
short int generate_voltage_tables()
{
  int k;

  /* composite 06 tables */
  for (k = 0; k < 3; k++)
  {
    S_composite_06_lum[k] = (k + 1) * COMPOSITE_06_TABLE_STEP;
    S_composite_06_lum[5 - k] = 1.0f - S_composite_06_lum[k];

    S_composite_06_sat[k] = S_composite_06_lum[k];
    S_composite_06_sat[5 - k] = S_composite_06_sat[k];
  }

  /* composite 12 tables */
  for (k = 0; k < 6; k++)
  {
    S_composite_12_lum[k] = (k + 1) * COMPOSITE_12_TABLE_STEP;
    S_composite_12_lum[11 - k] = 1.0f - S_composite_12_lum[k];

    S_composite_12_sat[k] = S_composite_12_lum[k];
    S_composite_12_sat[11 - k] = S_composite_12_sat[k];
  }

  /* composite 18 tables */
  for (k = 0; k < 9; k++)
  {
    S_composite_18_lum[k] = (k + 1) * COMPOSITE_18_TABLE_STEP;
    S_composite_18_lum[17 - k] = 1.0f - S_composite_18_lum[k];

    S_composite_18_sat[k] = S_composite_18_lum[k];
    S_composite_18_sat[17 - k] = S_composite_18_sat[k];
  }

  /* composite 24 tables */
  for (k = 0; k < 12; k++)
  {
    S_composite_24_lum[k] = (k + 1) * COMPOSITE_24_TABLE_STEP;
    S_composite_24_lum[23 - k] = 1.0f - S_composite_24_lum[k];

    S_composite_24_sat[k] = S_composite_24_lum[k];
    S_composite_24_sat[23 - k] = S_composite_24_sat[k];
  }

  /* composite 36 tables */
  for (k = 0; k < 18; k++)
  {
    S_composite_36_lum[k] = (k + 1) * COMPOSITE_36_TABLE_STEP;
    S_composite_36_lum[35 - k] = 1.0f - S_composite_36_lum[k];

    S_composite_36_sat[k] = S_composite_36_lum[k];
    S_composite_36_sat[35 - k] = S_composite_36_sat[k];
  }

  /* composite 48 tables */
  for (k = 0; k < 24; k++)
  {
    S_composite_48_lum[k] = (k + 1) * COMPOSITE_48_TABLE_STEP;
    S_composite_48_lum[47 - k] = 1.0f - S_composite_48_lum[k];

    S_composite_48_sat[k] = S_composite_48_lum[k];
    S_composite_48_sat[47 - k] = S_composite_48_sat[k];
  }

  return 0;
}

/*******************************************************************************
** set_voltage_table_pointers()
*******************************************************************************/
short int set_voltage_table_pointers()
{
  if (G_source == SOURCE_APPROX_NES)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if ( (G_source == SOURCE_COMPOSITE_06_0p75X) || 
            (G_source == SOURCE_COMPOSITE_06_3X))
  {
    S_luma_table = S_composite_06_lum;
    S_saturation_table = S_composite_06_sat;
    S_table_length = 6;
  }
  else if ( (G_source == SOURCE_COMPOSITE_12_1p50X) || 
            (G_source == SOURCE_COMPOSITE_12_6X))
  {
    S_luma_table = S_composite_12_lum;
    S_saturation_table = S_composite_12_sat;
    S_table_length = 12;
  }
  else if (G_source == SOURCE_COMPOSITE_18_1X)
  {
    S_luma_table = S_composite_18_lum;
    S_saturation_table = S_composite_18_sat;
    S_table_length = 18;
  }
  else if ( (G_source == SOURCE_COMPOSITE_24_0p75X) || 
            (G_source == SOURCE_COMPOSITE_24_3X))
  {
    S_luma_table = S_composite_24_lum;
    S_saturation_table = S_composite_24_sat;
    S_table_length = 24;
  }
  else if (G_source == SOURCE_COMPOSITE_36_2X)
  {
    S_luma_table = S_composite_36_lum;
    S_saturation_table = S_composite_36_sat;
    S_table_length = 36;
  }
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
  {
    S_luma_table = S_composite_48_lum;
    S_saturation_table = S_composite_48_sat;
    S_table_length = 48;
  }
  else
  {
    printf("Cannot set voltage table pointers; invalid source specified.\n");
    return 1;
  }

  return 0;
}

/*******************************************************************************
** add_color()
*******************************************************************************/
short int add_color(unsigned char r, unsigned char g, unsigned char b)
{
  /* make sure array index is valid */
  if ((G_num_colors < 0) || (G_num_colors >= G_max_colors))
  {
    printf("Unable to add color: Colors array is filled.\n");
    return 1;
  }

  /* add color */
  G_colors_array[G_num_colors].r = r;
  G_colors_array[G_num_colors].g = g;
  G_colors_array[G_num_colors].b = b;

  G_num_colors += 1;

  return 0;
}

/*******************************************************************************
** generate_palette_greys()
*******************************************************************************/
short int generate_palette_greys()
{
  int     k;

  int     r;
  int     g;
  int     b;

  /* generate greys */
  for (k = 0; k < S_table_length; k++)
  {
    r = (int) ((S_luma_table[k] * 255) + 0.5f);
    g = (int) ((S_luma_table[k] * 255) + 0.5f);
    b = (int) ((S_luma_table[k] * 255) + 0.5f);

    add_color(r, g, b);
  }

  return 0;
}

/*******************************************************************************
** generate_palette_hue()
*******************************************************************************/
short int generate_palette_hue(int hue, int modifier)
{
  int     k;

  float   y;
  float   i;
  float   q;

  int     r;
  int     g;
  int     b;

  int     start_index;
  int     end_index;

  /* make sure hue is valid */
  if ((hue < 0) || (hue >= 360))
  {
    printf("Cannot create palette hue; invalid hue specified.\n");
    return 1;
  }

  /* apply modifer */
  if (modifier == HUE_MODIFIER_FULL)
  {
    start_index = 0;
    end_index = S_table_length;
  }
  else if (modifier == HUE_MODIFIER_LOWER_HALF)
  {
    start_index = 0;
    end_index = S_table_length / 2;
  }
  else if (modifier == HUE_MODIFIER_UPPER_HALF)
  {
    start_index = S_table_length / 2;
    end_index = S_table_length;
  }
  else
  {
    printf("Cannot create palette hue; invalid modifier specified.\n");
    return 1;
  }

  /* generate hue */
  for (k = start_index; k < end_index; k++)
  {
    y = S_luma_table[k];
    i = S_saturation_table[k] * cos(TWO_PI * hue / 360.0f);
    q = S_saturation_table[k] * sin(TWO_PI * hue / 360.0f);

    r = (int) (((y + (i * 0.956f) + (q * 0.619f)) * 255) + 0.5f);
    g = (int) (((y - (i * 0.272f) - (q * 0.647f)) * 255) + 0.5f);
    b = (int) (((y - (i * 1.106f) + (q * 1.703f)) * 255) + 0.5f);

    /* bound rgb values */
    if (r < 0)
      r = 0;
    else if (r > 255)
      r = 255;

    if (g < 0)
      g = 0;
    else if (g > 255)
      g = 255;

    if (b < 0)
      b = 0;
    else if (b > 255)
      b = 255;

    /* add color to the palette */
    add_color(r, g, b);
  }

  return 0;
}

/*******************************************************************************
** generate_palette_from_source()
*******************************************************************************/
short int generate_palette_from_source()
{
  int i;

  int hue;
  int step;

  /* approximate nes */
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    /* set hue step & hue start */
    step = 30;

    if (G_source == SOURCE_APPROX_NES_ROTATED)
      hue = 15;
    else
      hue = 0;

    /* add pure black */
    add_color(0, 0, 0);

    /* add greys */
    generate_palette_greys();

    /* add pure white */
    add_color(255, 255, 255);

    /* add hues */
    for (i = 0; i < 360 / step; i++)
    {
      generate_palette_hue(hue, HUE_MODIFIER_FULL);

      /* increment hue */
      hue += step;
      hue = hue % 360;
    }
  }
  /* composite source */
  else if ( (G_source == SOURCE_COMPOSITE_06_0p75X) || 
            (G_source == SOURCE_COMPOSITE_06_3X)    || 
            (G_source == SOURCE_COMPOSITE_12_1p50X) || 
            (G_source == SOURCE_COMPOSITE_12_6X)    || 
            (G_source == SOURCE_COMPOSITE_18_1X)    || 
            (G_source == SOURCE_COMPOSITE_24_0p75X) || 
            (G_source == SOURCE_COMPOSITE_24_3X)    || 
            (G_source == SOURCE_COMPOSITE_36_2X)    || 
            (G_source == SOURCE_COMPOSITE_48_1p50X))
  {
    /* determine hue step & hue start */
    if ((G_source == SOURCE_COMPOSITE_06_0p75X) || 
        (G_source == SOURCE_COMPOSITE_24_0p75X))
    {
      step = 40;
    }
    else if (G_source == SOURCE_COMPOSITE_18_1X)
    {
      step = 30;
    }
    else if ( (G_source == SOURCE_COMPOSITE_12_1p50X) || 
              (G_source == SOURCE_COMPOSITE_48_1p50X))
    {
      step = 20;
    }
    else if (G_source == SOURCE_COMPOSITE_36_2X)
    {
      step = 15;
    }
    else if ( (G_source == SOURCE_COMPOSITE_06_3X) || 
              (G_source == SOURCE_COMPOSITE_24_3X))
    {
      step = 10;
    }
    else if (G_source == SOURCE_COMPOSITE_12_6X)
    {
      step = 5;
    }
    else
    {
      step = 30;
    }

    hue = 0;

    /* add greys */
    generate_palette_greys();

    /* add hues */
    for (i = 0; i < 360 / step; i++)
    {
      generate_palette_hue(hue, HUE_MODIFIER_FULL);

      /* increment hue */
      hue += step;
      hue = hue % 360;
    }
  }

  return 0;
}

/*******************************************************************************
** write_gpl_file()
*******************************************************************************/
short int write_gpl_file(char* filename)
{
  FILE* fp_out;

  int   color_index;

  unsigned char r;
  unsigned char g;
  unsigned char b;

  fp_out = NULL;

  /* check that output gpl file was given */
  if (filename == NULL)
  {
    printf("No output GPL file specified. Exiting...\n");
    return 1;
  }

  /* open output file */
  fp_out = fopen(filename, "w");

  /* if file did not open, return */
  if (fp_out == NULL)
  {
    printf("Unable to open output GPL file. Exiting...\n");
    return 1;
  }

  /* write out header info */
  fprintf(fp_out, "GIMP Palette\n");

  if (G_source == SOURCE_APPROX_NES)
    fprintf(fp_out, "Name: Approximate NES\n\n");
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    fprintf(fp_out, "Name: Approximate NES Rotated\n\n");
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    fprintf(fp_out, "Name: Composite 06 0.75X\n\n");
  else if (G_source == SOURCE_COMPOSITE_06_3X)
    fprintf(fp_out, "Name: Composite 06 3X\n\n");
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    fprintf(fp_out, "Name: Composite 12 1.5X\n\n");
  else if (G_source == SOURCE_COMPOSITE_12_6X)
    fprintf(fp_out, "Name: Composite 12 6X\n\n");
  else if (G_source == SOURCE_COMPOSITE_18_1X)
    fprintf(fp_out, "Name: Composite 18 1X\n\n");
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    fprintf(fp_out, "Name: Composite 24 0.75X\n\n");
  else if (G_source == SOURCE_COMPOSITE_24_3X)
    fprintf(fp_out, "Name: Composite 24 3X\n\n");
  else if (G_source == SOURCE_COMPOSITE_36_2X)
    fprintf(fp_out, "Name: Composite 36 2X\n\n");
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    fprintf(fp_out, "Name: Composite 48 1.5X\n\n");

  /* write out palette colors */
  for (color_index = 0; color_index < G_num_colors; color_index++)
  {
    r = G_colors_array[color_index].r;
    g = G_colors_array[color_index].g;
    b = G_colors_array[color_index].b;

    if (r < 10)
      fprintf(fp_out, "  %d ", r);
    else if (r < 100)
      fprintf(fp_out, " %d ", r);
    else
      fprintf(fp_out, "%d ", r);

    if (g < 10)
      fprintf(fp_out, "  %d ", g);
    else if (g < 100)
      fprintf(fp_out, " %d ", g);
    else
      fprintf(fp_out, "%d ", g);

    if (b < 10)
      fprintf(fp_out, "  %d", b);
    else if (b < 100)
      fprintf(fp_out, " %d", b);
    else
      fprintf(fp_out, "%d", b);

    fprintf(fp_out, "\t(%d, %d, %d)\n", r, g, b);
  }

  /* close file */
  fclose(fp_out);

  return 0;
}

/*******************************************************************************
** write_tga_file()
*******************************************************************************/
short int write_tga_file(char* filename)
{
  FILE* fp_out;

  unsigned char image_id_field_length;
  unsigned char color_map_type;
  unsigned char image_type;
  unsigned char image_descriptor;

  unsigned char color_map_specification[5];

  short int     x_origin;
  short int     y_origin;

  short int     image_w;
  short int     image_h;

  unsigned char pixel_bpp;
  short int     pixel_num_bytes;

  unsigned char output_buffer[3];

  int           color_index;

  /* make sure number of colors is < 1024 */
  if (G_num_colors >= 1024)
  {
    printf("Write TGA file failed: Number of colors >= 1024.\n");
    return 0;
  }

  /* make sure filename is valid */
  if (filename == NULL)
  {
    printf("Write TGA file failed: No filename specified.\n");
    return 1;
  }

  /* open file */
  fp_out = fopen(filename, "wb");

  /* if file did not open, return error */
  if (fp_out == NULL)
  {
    printf("Write TGA file failed: Unable to open output file.\n");
    return 1;
  }

  /* initialize parameters */
  image_id_field_length = 0;
  color_map_type = 0;
  image_type = 2;
  image_descriptor = 0x20;

  color_map_specification[0] = 0;
  color_map_specification[1] = 0;
  color_map_specification[2] = 0;
  color_map_specification[3] = 0;
  color_map_specification[4] = 0;

  x_origin = 0;
  y_origin = 0;

  if (G_num_colors <= 64)
    image_w = 64;
  else if (G_num_colors <= 256)
    image_w = 256;
  else if (G_num_colors <= 1024)
    image_w = 1024;
  else
    image_w = 1024;

  image_h = 1;

  pixel_bpp = 24;
  pixel_num_bytes = 3;

  output_buffer[0] = 0;
  output_buffer[1] = 0;
  output_buffer[2] = 0;

  /* write image id field length */
  if (fwrite(&image_id_field_length, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write colormap type */
  if (fwrite(&color_map_type, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image type */
  if (fwrite(&image_type, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write colormap specification */
  if (fwrite(color_map_specification, 1, 5, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write x origin */
  if (fwrite(&x_origin, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write y origin */
  if (fwrite(&y_origin, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image width */
  if (fwrite(&image_w, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image height */
  if (fwrite(&image_h, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write pixel bpp */
  if (fwrite(&pixel_bpp, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image descriptor */
  if (fwrite(&image_descriptor, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write palette colors */
  for (color_index = 0; color_index < G_num_colors; color_index++)
  {
    output_buffer[2] = G_colors_array[color_index].r;
    output_buffer[1] = G_colors_array[color_index].g;
    output_buffer[0] = G_colors_array[color_index].b;

    if (fwrite(output_buffer, 1, 3, fp_out) < 1)
    {
      fclose(fp_out);
      return 1;
    }
  }

  /* fill remaining spaces with zeroes */
  for (color_index = G_num_colors; color_index < image_w; color_index++)
  {
    output_buffer[2] = 0;
    output_buffer[1] = 0;
    output_buffer[0] = 0;

    if (fwrite(output_buffer, 1, 3, fp_out) < 1)
    {
      fclose(fp_out);
      return 1;
    }
  }

  /* close file */
  fclose(fp_out);

  return 0;
}

/*******************************************************************************
** main()
*******************************************************************************/
int main(int argc, char *argv[])
{
  int   i;

  char  output_base_filename[256];
  char  output_gpl_filename[256];
  char  output_tga_filename[256];

  /* initialization */
  G_colors_array = NULL;
  G_num_colors = 0;
  G_max_colors = 0;

  output_base_filename[0] = '\0';
  output_gpl_filename[0] = '\0';
  output_tga_filename[0] = '\0';

  /* initialize variables */
  G_source = SOURCE_APPROX_NES;

  S_luma_table = S_approx_nes_lum;
  S_saturation_table = S_approx_nes_sat;
  S_table_length = 4;

  /* generate voltage tables */
  generate_voltage_tables();

  /* read command line arguments */
  i = 1;

  while (i < argc)
  {
    /* source */
    if (!strcmp(argv[i], "-s"))
    {
      i++;

      if (i >= argc)
      {
        printf("Insufficient number of arguments. ");
        printf("Expected source name. Exiting...\n");
        return 0;
      }

      if (!strcmp("approx_nes", argv[i]))
        G_source = SOURCE_APPROX_NES;
      else if (!strcmp("approx_nes_rotated", argv[i]))
        G_source = SOURCE_APPROX_NES_ROTATED;
      else if (!strcmp("composite_06_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_06_0p75X;
      else if (!strcmp("composite_06_3x", argv[i]))
        G_source = SOURCE_COMPOSITE_06_3X;
      else if (!strcmp("composite_12_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_12_1p50X;
      else if (!strcmp("composite_12_6x", argv[i]))
        G_source = SOURCE_COMPOSITE_12_6X;
      else if (!strcmp("composite_18_1x", argv[i]))
        G_source = SOURCE_COMPOSITE_18_1X;
      else if (!strcmp("composite_24_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_24_0p75X;
      else if (!strcmp("composite_24_3x", argv[i]))
        G_source = SOURCE_COMPOSITE_24_3X;
      else if (!strcmp("composite_36_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_36_2X;
      else if (!strcmp("composite_48_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_48_1p50X;
      else
      {
        printf("Unknown source %s. Exiting...\n", argv[i]);
        return 0;
      }

      i++;
    }
    else
    {
      printf("Unknown command line argument %s. Exiting...\n", argv[i]);
      return 0;
    }
  }

  /* generate output filenames */
  if (G_source == SOURCE_APPROX_NES)
    strncpy(output_base_filename, "approx_nes", 16);
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    strncpy(output_base_filename, "approx_nes_rotated", 24);
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    strncpy(output_base_filename, "composite_06_0p75x", 24);
  else if (G_source == SOURCE_COMPOSITE_06_3X)
    strncpy(output_base_filename, "composite_06_3x", 24);
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    strncpy(output_base_filename, "composite_12_1p50x", 24);
  else if (G_source == SOURCE_COMPOSITE_12_6X)
    strncpy(output_base_filename, "composite_12_6x", 24);
  else if (G_source == SOURCE_COMPOSITE_18_1X)
    strncpy(output_base_filename, "composite_18_1x", 24);
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    strncpy(output_base_filename, "composite_24_0p75x", 24);
  else if (G_source == SOURCE_COMPOSITE_24_3X)
    strncpy(output_base_filename, "composite_24_3x", 24);
  else if (G_source == SOURCE_COMPOSITE_36_2X)
    strncpy(output_base_filename, "composite_36_2x", 24);
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    strncpy(output_base_filename, "composite_48_1p50x", 24);

  strncpy(output_gpl_filename, output_base_filename, 256);
  strncpy(output_tga_filename, output_base_filename, 256);

  strncat(output_gpl_filename, ".gpl", 4);
  strncat(output_tga_filename, ".tga", 4);

  /* allocate palette array */
  if ((G_source == SOURCE_APPROX_NES)         || 
      (G_source == SOURCE_APPROX_NES_ROTATED) || 
      (G_source == SOURCE_COMPOSITE_06_0p75X))
  {
    G_max_colors = 64;
  }
  else if ( (G_source == SOURCE_COMPOSITE_06_3X)    || 
            (G_source == SOURCE_COMPOSITE_12_1p50X) || 
            (G_source == SOURCE_COMPOSITE_18_1X)    || 
            (G_source == SOURCE_COMPOSITE_24_0p75X))
  {
    G_max_colors = 256;
  }
  else if ( (G_source == SOURCE_COMPOSITE_12_6X)  || 
            (G_source == SOURCE_COMPOSITE_24_3X)  || 
            (G_source == SOURCE_COMPOSITE_36_2X)  || 
            (G_source == SOURCE_COMPOSITE_48_1p50X))
  {
    G_max_colors = 1024;
  }
  else
  {
    printf("Unable to determine max palette colors. Exiting...\n");
    return 0;
  }

  G_colors_array = malloc(sizeof(color) * G_max_colors);

  if (G_colors_array == NULL)
  {
    printf("Error allocating palette array. Exiting...\n");
    return 0;
  }

  /* set voltage table pointers */
  if (set_voltage_table_pointers())
  {
    printf("Error setting voltage table pointers. Exiting...\n");
    return 0;
  }

  /* generate palette */
  if (generate_palette_from_source())
  {
    printf("Error generating palette. Exiting...\n");
    return 0;
  }

  /* print color count */
  printf("Palette generated. Number of Colors: %d\n", G_num_colors);

  /* write output gpl file */
  write_gpl_file(output_gpl_filename);

  /* write output tga file */
  write_tga_file(output_tga_filename);

  /* free palette array */
  if (G_colors_array != NULL)
  {
    free(G_colors_array);
    G_colors_array = NULL;
  }

  return 0;
}
