// Renzo Lucioni (HUID: 90760092)
// Matt Sheets (HUID: 50834689)

#include "matrix_multiply.h"

// used for averaging results when flag is 1
#define NUMTRIALS 15

// Determined experimentally, as described in the writeup
#define CROSSOVER 64

// we leave out dim and dim_orig to avoid repitition (they're the same for all matrices in a
//   function - struct
typedef struct {
    int32_t *matrix;
    size_t row_off;
    size_t col_off;
    size_t dim_real;
} matrix;

// accesses the element in "matrix" of dimension "dim" at "row" and "column"
#define MELT(m, row, col) (m).matrix[((row) + (m).row_off) * (m).dim_real + ((col) + (m).col_off)]

//int32_t crossover;

void print_matrix(const matrix m, size_t dim) {
    for (size_t i = 0; i < dim; i++) {
        printf("| ");
        for (size_t j = 0; j < dim; j++)
            printf("%3d | ", MELT(m, i, j));
        printf("\n");
    }
    printf("\n");
}

void square_matrix_multiply(matrix c, const matrix a, const matrix b, size_t dim) {
    for (size_t i = 0; i < dim; i++)
        for (size_t j = 0; j < dim; j++)
            MELT(c, i, j) = 0;
    for (size_t i = 0; i < dim; i++)
        for (size_t k = 0; k < dim; k++) {
            int32_t a_ik = MELT(a, i, k);
            for (size_t j = 0; j < dim; j++)
                MELT(c, i, j) += a_ik * MELT(b, k, j);
        }
}

// madd(c, a, b, dim)
//   Addition of square matrices (a + b = c)
void madd(matrix c, const matrix a, const matrix b, size_t dim) {
    for (size_t i = 0; i < dim; i++)
        for (size_t j = 0; j < dim; j++)
            MELT(c, i, j) = MELT(a, i, j) + MELT(b, i, j);
}

// msub(c, a, b, dim)
//   Subtraction of square matrices (a - b = c)
void msub(matrix c, const matrix a, const matrix b, size_t dim) {
    for (size_t i = 0; i < dim; i++)
        for (size_t j = 0; j < dim; j++)
            MELT(c, i, j) = MELT(a, i, j) - MELT(b, i, j);
}

// sadd(e, a, b, c, d, dim)
//   Composition of several matrix operations for use by Strassen to avoid
//   redundant loops (used for AE + BG in the Strassen Algorithm)
// TODO: Inline to optimize away function call (since only used once)
void sadd(matrix e, const matrix a, const matrix b, const matrix c, const matrix d, 
      size_t dim) {
    for (size_t i = 0; i < dim; i++)
        for (size_t j = 0; j < dim; j++)
            MELT(e, i, j) = MELT(a, i, j) + MELT(b, i, j)
            - MELT(c, i, j) + MELT(d, i, j);
}

// ssub(e, a, b, c, d, dim)
//   Composition of several matrix operations for use by Strassen to avoid
//   redundant loops (used for CF + DH in the Strassen Algorithm)
// TODO: Inline to optimize away function call (since only used once)
void ssub(matrix e, const matrix a, const matrix b, const matrix c, const matrix d, 
      size_t dim) {
    for (size_t i = 0; i < dim; i++)
        for (size_t j = 0; j < dim; j++)
            MELT(e, i, j) = MELT(a, i, j) + MELT(b, i, j)
            - MELT(c, i, j) - MELT(d, i, j);
}

matrix init_matrix(matrix o, size_t row_off, size_t col_off) {
    matrix n = {.matrix = o.matrix,
                .row_off = row_off + o.row_off,
                .col_off = col_off + o.col_off,
                .dim_real = o.dim_real};
    return n;
}

// strassen(c, a, b, dim)
//   Multiplication of square matrices using the Strassen algorithm (a x b = c)
//   Comments based off of the following matrix multiplication:
//    -    -    -   -       -                - 
//   | A  B |  | E F |  =  | AE + BG  AF + BH | 
//   | C  D |  | G H |     | CE + DG  CF + DH |
//    -    -    -   -       -                -
void strassen(matrix c, const matrix a, const matrix b, size_t dim) {
    //if (dim <= crossover)
    if (dim <= CROSSOVER)
        square_matrix_multiply(c, a, b, dim);
    else {
        // cutting into submatrices
        size_t dim_half = dim >> 1;

        matrix a1 = init_matrix(a, 0, 0);
        matrix a2 = init_matrix(a, 0, dim_half);
        matrix a3 = init_matrix(a, dim_half, 0);
        matrix a4 = init_matrix(a, dim_half, dim_half);

        matrix b1 = init_matrix(b, 0, 0);
        matrix b2 = init_matrix(b, 0, dim_half);
        matrix b3 = init_matrix(b, dim_half, 0);
        matrix b4 = init_matrix(b, dim_half, dim_half);

        matrix c1 = init_matrix(c, 0, 0);
        matrix c2 = init_matrix(c, 0, dim_half);
        matrix c3 = init_matrix(c, dim_half, 0);
        matrix c4 = init_matrix(c, dim_half, dim_half);

        // work matrix (for P1 through P7 - extra for work)
        size_t subsize = dim_half * dim_half;

        int32_t *p = (int32_t *) malloc(sizeof(int32_t) * subsize * 8);

        matrix p1 = {.matrix = (&p[0]), .row_off = 0, .col_off = 0, .dim_real = dim_half};
        matrix p2 = {.matrix = (&p[subsize]), .row_off = 0, .col_off = 0, .dim_real = dim_half};
        matrix p3 = {.matrix = (&p[2 * subsize]), .row_off = 0, .col_off = 0, 
                     .dim_real = dim_half};
        matrix p4 = {.matrix = (&p[3 * subsize]), .row_off = 0, .col_off = 0,
                     .dim_real = dim_half};
        matrix p5 = {.matrix = (&p[4 * subsize]), .row_off = 0, .col_off = 0,
                     .dim_real = dim_half};
        matrix p6 = {.matrix = (&p[5 * subsize]), .row_off = 0, .col_off = 0,
                     .dim_real = dim_half};
        matrix p7 = {.matrix = (&p[6 * subsize]), .row_off = 0, .col_off = 0,
                     .dim_real = dim_half};
        matrix p8 = {.matrix = (&p[7 * subsize]), .row_off = 0, .col_off = 0,
                     .dim_real = dim_half};

        // First calculate P7, P6, & P5 due to double operations (double space)
        // P7
        msub(p1, a1, a3, dim_half); // A - C
        madd(p2, b1, b2, dim_half); // E + F
        strassen(p7, p1, p2, dim_half); // (A - C)(E + F)
        
        // P6
        msub(p1, a2, a4, dim_half); // B - D
        madd(p2, b3, b4, dim_half); // G + H
        strassen(p6, p1, p2, dim_half); // (B - D)(G + H)

        // P5
        madd(p1, a1, a4, dim_half); // A + D
        madd(p2, b1, b4, dim_half); // E + H
        strassen(p5, p1, p2, dim_half); // (A + D)(E + H)

        // P4
        msub(p1, b3, b1, dim_half); // G - E
        strassen(p4, a4, p1, dim_half); // D(G - E)

        // P3
        madd(p1, a3, a4, dim_half); // C + D
        strassen(p3, p1, b1, dim_half); // (C + D)E

        // P2
        madd(p1, a1, a2, dim_half); // A + B
        strassen(p2, p1, b4, dim_half); // (A + B)H

        // P1
        msub(p8, b2, b4, dim_half); // F - H
        strassen(p1, a1, p8, dim_half); // A(F - H)

        // Calculate new matrix
        sadd(c1, p5, p4, p2, p6, dim_half); // AE + BG  (P5 + P4 - P2 + P6)
        madd(c2, p1, p2, dim_half); // AF + BH  (P1 + P2)
        madd(c3, p3, p4, dim_half); // CE + DG  (P3 + P4)
        ssub(c4, p5, p1, p3, p7, dim_half); // CF + DH  (P5 + P1 - P3 + P7)

        // deallocate work matrix
        free(p);
    }
}

int main(int argc, char **argv) {
    // process command line arguments
    if (argc != 4) {
        fprintf(stderr, "usage: %s flag dimension inputfile\n", argv[0]);
        exit(1);
    }

    /*if (argc != 5) {
        fprintf(stderr, "usage: %s flag dimension inputfile crossover\n", argv[0]);
        exit(1);
    }*/

    int flag = atoi(argv[1]);
    size_t dim = atoi(argv[2]);
    char* inputfile = argv[3];
    //crossover = atoi(argv[4]);
    
    assert(dim > 0);

    FILE *fp;
    fp = fopen(inputfile, "r");
    assert(fp != NULL);

    int32_t elt;
    size_t i;
    size_t j;

    // if not a power of 2, pad appropriately
    size_t dim_pad = dim;
    if ((dim & (dim - 1)) != 0) {
        // Bit Twiddling Hack for finding next highest power of 2
        //   http://graphics.stanford.edu/~seander/bithacks.html
        dim_pad--;
        dim_pad |= dim_pad >> 1;
        dim_pad |= dim_pad >> 2;
        dim_pad |= dim_pad >> 4;
        dim_pad |= dim_pad >> 8;
        dim_pad |= dim_pad >> 16;
        // handle 64 bit
        //if (sizeof(size_t) == 8)
        //    dim_pad |= dim_pad >> 32;
        dim_pad++;
    }

    matrix ma = {.matrix = (int32_t *) malloc(sizeof(int32_t) * dim_pad * dim_pad),
                 .row_off = 0, .col_off = 0, .dim_real = dim_pad};
    for (i = 0; i < dim_pad; i++)
        for (j = 0; j < dim_pad; j++) {
            // As per Piazza question #217, we assume the files being passed 
            // have the correct number of values (i.e., we don't check for EOF)
            if (i < dim && j < dim) {
                (void) fscanf(fp, "%d", &elt);
                MELT(ma, i, j) = elt;
            }
            else {
                MELT(ma, i, j) = 0;
            }
        }

    // for large matrices
    //rewind(fp);

    matrix mb = {.matrix = (int32_t *) malloc(sizeof(int32_t) * dim_pad * dim_pad),
                 .row_off = 0, .col_off = 0, .dim_real = dim_pad};
    for (i = 0; i < dim_pad; i++)
        for (j = 0; j < dim_pad; j++) {
            if (i < dim && j < dim) {
                (void) fscanf(fp, "%d", &elt); 
                MELT(mb, i, j) = elt;
            }
            else {
                MELT(mb,i, j) = 0;
            }
        }

    fclose(fp);

    matrix mc = {.matrix = (int32_t *) malloc(sizeof(int32_t) * dim_pad * dim_pad),
                 .row_off = 0, .col_off = 0, .dim_real = dim_pad};

    int numtrials = 1;
    if (flag == 1)
        numtrials = NUMTRIALS;

    // track CPU time for the modified Strassen's
    int time_total = 0;
    for (int i = 0; i < numtrials; ++i) {
        clock_t start = clock();    
        strassen(mc, ma, mb, dim_pad);
        time_total += clock() - start;
    }

    // use flag 1 to get time in milliseconds
    if (flag == 1) {
        int msec = (time_total * 1000) / CLOCKS_PER_SEC;
        printf("%.2f\n", (float)msec / numtrials);
    }
    // use flag 0 (or other flags) to get diagonal elements
    else {
        // print diagonal elements - does not read padding
        for (i = 0; i < dim; i++)
            printf("%d\n", MELT(mc, i, i));
    }

    free(ma.matrix);
    free(mb.matrix);
    free(mc.matrix);
    return 0;
}
