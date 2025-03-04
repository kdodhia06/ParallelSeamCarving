#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <cstring>
#include <omp.h>
#include <cfloat>
#include <limits>
#include "mic.h"

#define GET_INDEX(x,y,w) (x * w + y)*3
#define GET_INDEX_1(x,y,w) (x * w + y)

using namespace std;

static int _argc;
static const char **_argv;

using namespace std::chrono;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::duration<double> dsec;

extern void  png_to_text(const char *filename);
void calculate_energy(uint8_t *image, double *energy, int rows, int cols);
void reduce_image(uint8_t *image, int seam_count, int rows, int cols);
void calculate_ACM(double *energy, int rows, int cols);
void find_seam(double *energy, int *seam, int rows, int cols);
void remove_seam(uint8_t *image, int *seam, int rows, int cols);

/* Starter code function, don't touch */
const char *get_option_string(const char *option_name,
                  const char *default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return _argv[i + 1];
  return default_value;
}

/* Starter code function, do not touch */
int get_option_int(const char *option_name, int default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return atoi(_argv[i + 1]);
  return default_value;
}

/* Starter code function, do not touch */
float get_option_float(const char *option_name, float default_value)
{
  for (int i = _argc - 2; i >= 0; i -= 2)
    if (strcmp(_argv[i], option_name) == 0)
      return (float)atof(_argv[i + 1]);
  return default_value;
}

static void show_help(const char *program_path)
{
    printf("Usage: %s OPTIONS\n", program_path);
    printf("\n");
    printf("OPTIONS:\n");
    printf("\t-f <input_filename> (required)\n");
    printf("\t-w <width> (required)\n");
    printf("\t-h <height> (required)\n");
    printf("\t-s <num_of_seams> (required)\n");
    printf("\t-n <num_threads>\n");
}


void calculate_energy(uint8_t *image, double *energy, int rows, int cols){

    double inf = 10000;
    
    for(int col = 0; col < cols; col++){
        for(int row = 0; row < rows; row++){
            double energy_val;
            
            if (row == rows - 1|| col == cols - 1 || col == 0 || row == 0){
                energy_val = 1;
            } else {

                int rUp = image[GET_INDEX(row - 1, col, cols)];
                int rDown = image[GET_INDEX(row + 1, col, cols)];
                int rLeft = image[GET_INDEX(row, col - 1, cols)];
                int rRight = image[GET_INDEX(row, col + 1, cols)];

                int gUp = image[GET_INDEX(row - 1, col, cols) + 1];
                int gDown = image[GET_INDEX(row + 1, col, cols) + 1];
                int gLeft = image[GET_INDEX(row, col - 1, cols) + 1];
                int gRight = image[GET_INDEX(row, col + 1, cols) + 1];

                int bUp = image[GET_INDEX(row - 1, col, cols) + 2];
                int bDown = image[GET_INDEX(row + 1, col, cols) + 2];
                int bLeft = image[GET_INDEX(row, col - 1, cols) + 2];
                int bRight = image[GET_INDEX(row, col + 1, cols) + 2];

                int rdx = abs(rRight - rLeft);
                int gdx = abs(gRight - gLeft);
                int bdx = abs(bRight - bLeft);
                int rdy = abs(rUp - rDown);
                int gdy = abs(gUp - gDown);
                int bdy = abs(bUp - bDown);

                int delta = rdx + gdx + bdx + rdy + gdy + bdy;
                energy_val = (((double)delta) / ((double)1530));
            }
            energy[row * cols + col] = energy_val;
        }
    }

}

void find_seam(double *energy, int *seam, int rows, int cols)
{
    
    double  min_val = -1;
    int min_col = -1;
    double inf = 100000;

    for (int col = 1; col < cols-1; col++) {
        double cur = energy[(rows-1) * cols + col];
        if (cur < min_val || min_col == -1 ) {
            min_val = cur;
            min_col = col;
        }
    }

    int cur_col = min_col;
    int row;

    for (row = rows-1; row > 0; row--) {
        seam[row] = cur_col;
        
        double  upleft, upright;
        if (cur_col > 1) {
            upleft = energy[(row-1) * cols + (cur_col-1)];
        } else {
            upleft = inf;
        }

        if (cur_col < cols-2) {
            upright = energy[(row-1) * cols + (cur_col+1)];
        } else {
            upright = inf;
        }

        double up = energy[(row-1)*cols + cur_col];

        min_val = min(upleft, min(up, upright));

        if (min_val == upright) {
            cur_col++;
        } else if (min_val == upleft) {
            cur_col--;
        }
    }
    seam[row] = cur_col;

}


/*
 * Remove seam from the image.
 */

void remove_seam(uint8_t *outImage, int *seam, int rows, int cols)
{
    for (int row = 0; row < rows; row++) {
        int col_to_remove = seam[row];
        for (int col = 0; col < cols; col++) {

            int prev_index = row * cols + col;
            int prev_index3 = prev_index * 3;

            int new_index = row * (cols-1) + col;
            int new_index3 = new_index * 3;

            if (col > col_to_remove) {

                int out_index = new_index - 1;
                int out_index3 = out_index * 3;

                outImage[out_index3] = outImage[prev_index3];
                outImage[out_index3 + 1] = outImage[prev_index3 + 1];
                outImage[out_index3 + 2] = outImage[prev_index3 + 2];

            } else if (col < col_to_remove) {

                outImage[new_index3] = outImage[prev_index3];
                outImage[new_index3 + 1] = outImage[prev_index3 + 1];
                outImage[new_index3 + 2] = outImage[prev_index3 + 2];

            }
        }
    }

}



/*
 * Draw seam on the image.
 */
/*
void draw_seam(uint8_t *outImage, int *seam, int rows, int cols)
{
    for (int row = 0; row < rows; row++) {
        int col_to_remove = seam[row];
        for (int col = 0; col < cols; col++) {
            int index = row * cols + col;
            int index3 = index * 3;
            if (col > col_to_remove) {
                outImage[index3] = 255;
                outImage[index3 + 1] = 0;
                outImage[index3 + 2] = 0;
            }
        }
    }
}
*/

void calculate_ACM(double *energy, int rows, int cols) {

    for (int row = 2; row < rows; row++) {

        for (int col = 1; col < cols; col++) {

            double up = energy[(row-1) * cols + col];

            if (col == 1) {

                double upright = energy[(row-1) * cols + (col+1)];
                energy[row * cols + col] = energy[row * cols + col] + min(up, upright);

            } else if (col == cols - 2){

                double upleft = energy[(row-1) * cols + col-1];
                energy[row * cols + col] = energy[row * cols + col] + min(up, upleft);

            } else {

                double upright = energy[(row-1) * cols + (col+1)];
                double upleft = energy[(row-1) * cols + (col-1)];
                energy[row * cols + col] = energy[row * cols + col] + min(up, min(upleft, upright));

            }
        }
    }

}


void reduce_image(uint8_t *reducedImg, double *energy, int *seam, int v, int rows, int cols) {

   double energy_compute_time = 0;
   double ACM_compute_time = 0;
   double seam_finding_time = 0;
   double seam_removal_time = 0;
   
   for(int i = 0; i < v; i++) {
        /*
         * Remove one vertical seam from img. The algorithm:
        1) get energy matrix.
        2) generate accumulated energy matrix
        3) find seam
        4) remove this seam from the image
         */

        auto now = Clock::now();    
        calculate_energy(reducedImg, energy, rows, cols);
        energy_compute_time += duration_cast<dsec>(Clock::now() - now).count();

        now = Clock::now();
        calculate_ACM(energy, rows, cols);
        ACM_compute_time += duration_cast<dsec>(Clock::now() - now).count();

        now = Clock::now();
        find_seam(energy, seam, rows, cols);
        seam_finding_time += duration_cast<dsec>(Clock::now() - now).count();

        now = Clock::now();
        remove_seam(reducedImg, seam, rows, cols);
        seam_removal_time += duration_cast<dsec>(Clock::now() - now).count();

        cols--;
    }

    printf("Total energy calculation Time: %lf.\n", energy_compute_time);
    printf("Total ACM generation Time: %lf.\n", ACM_compute_time);
    printf("Total seam finding Time: %lf.\n", seam_finding_time);
    printf("Total seam removal Time: %lf.\n", seam_removal_time);
}

int main(int argc, const char *argv[])
{
  auto init_start = Clock::now();
  double init_time = 0;
 
  _argc = argc - 1;
  _argv = argv + 1;

  /* You'll want to use these parameters in your algorithm */
  const char *input_filename = get_option_string("-f", NULL);
  int seam_count = get_option_int("-s", -1);
  int num_of_threads = get_option_int("-n", 1);
  int error = 0;

  if (input_filename == NULL) {
    printf("Error: You need to specify -f.\n");
    error = 1;
  }

  if (seam_count == -1) {
    printf("Error: You need to specify -s.\n");
    error = 1;
  }

  if (error) {
    show_help(argv[0]);
    return 1;
  }

  printf("Number of threads: %d\n", num_of_threads);

  FILE *input = fopen(input_filename, "r");

  if (!input) {
    printf("Unable to open file: %s.\n", input_filename);
    return -1;
  }

  int r, g, b;
  int index = 0;
  int rows, cols;
  fscanf(input, "%d %d\n", &cols, &rows);

  int img_size = rows * cols * 3;

  uint8_t *image = (uint8_t *)calloc(img_size, sizeof(uint8_t));
  double *energy = (double *)calloc(cols * rows, sizeof(double));
  int *seam = (int *)calloc(rows, sizeof(int));

  while (fscanf(input, "%d %d %d\n", &r, &g, &b) != EOF) {
    /* PARSE THE INPUT FILE HERE */
    image[index] = (uint8_t)r; index++;
    image[index] = (uint8_t)g; index++;
    image[index] = (uint8_t)b; index++;
  }

  init_time += duration_cast<dsec>(Clock::now() - init_start).count();
  printf("Initialization Time: %lf.\n", init_time);
  auto compute_start = Clock::now();
  double compute_time = 0;

#ifdef RUN_MIC /* Use RUN_MIC to distinguish between the target of compilation */

  /* This pragma means we want the code in the following block be executed in 
   * Xeon Phi.
   */
#pragma offload target(mic) \
  inout(image: length(img_size) INOUT) \
  inout(energy: length(rows * cols) INOUT) \
  inout(seam: length(rows) INOUT) 
#endif
  {
    /* Implement the wire routing algorithm here
     * Feel free to structure the algorithm into different functions
     * Don't use global variables.
     * Use OpenMP to parallelize the algorithm. 
     * You should really implement as much of this (if not all of it) in
     * helper functions. */
     //initialize_costs(costs, wires, num_of_wires, dim_x, dim_y);
    reduce_image(image, energy, seam, seam_count, rows, cols);
  }

  compute_time += duration_cast<dsec>(Clock::now() - compute_start).count();
  printf("Computation Time: %lf.\n", compute_time);
  printf("Total time: %1f.\n", compute_time + init_time);
  
  /* OUTPUT YOUR RESULTS TO FILES HERE 
   * When you're ready to output your data to files, uncommment this chunk of
   * code and fill in the specified blanks indicated by comments. More about
   * this in the README. */  

  FILE *outFile = fopen("outputImg.txt", "w");
  if (!outFile) {
    printf("Error: couldn't output image file");
    return -1;
  }
  int new_width = cols - seam_count;
  // output information here
  fprintf(outFile,"%d %d\n", new_width, rows);
  
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < new_width; col++) {
      int index = row * new_width + col;
      int index3 = index * 3;
      fprintf(outFile,"%d %d %d\n", (int)image[index3], (int)image[index3+1], (int)image[index3+2]);
    }
  }

  free(image);
  fclose(outFile);

  return 0;
}