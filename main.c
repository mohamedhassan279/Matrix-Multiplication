#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_LEN 256
FILE *file;
char *output; // name of output file

// struct for each matrix
struct matrix {
    int rows;
    int cols;
    int **arr;
};

// struct to pass as argument
struct multiplication {
    int row;
    int col;
    struct matrix *A;
    struct matrix *B;
    struct matrix *C;
};

// read the matrix from the text file
int read_matrix(const char name[], struct matrix *m) {
    // build the name of the file
    char *filename = calloc(5 + strlen(name), sizeof(char));
    strcpy(filename, name);
    strcat(filename, ".txt");

    file = fopen(filename, "r");

    // error in reading the file or file does not exist
    if (file == NULL) {
        printf("Error reading the file: %s\n", filename);
        return -1;
    }

    int rows, cols;
    fscanf(file, "row=%d col=%d", &rows, &cols);

    m->rows = rows;
    m->cols = cols;
    int **arr = (int **) malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        arr[i] = (int *) malloc(cols * sizeof(int));
        for (int j = 0; j < cols; j++) {
            if (fscanf(file, "%d", &arr[i][j]) != 1) {
                printf("Error reading the file: %s\n", filename);
                return -1;
            }
        }
    }
    m->arr = arr;
    free(filename);
    fclose(file);
    return 0;
}

// write the output matrix in file
void write_matrix(int type, struct matrix *C) {
    char filename[MAX_LEN];
    strcpy(filename, output);
    switch (type) {
        case 0:
            strcat(filename, "_per_matrix");
            break;
        case 1:
            strcat(filename, "_per_row");
            break;
        case 2:
            strcat(filename, "_per_element");
            break;
    }
    strcat(filename, ".txt");
    FILE *out = fopen(filename, "w");
    switch (type) {
        case 0:
            fprintf(out, "Method: A thread per matrix\n");
            break;
        case 1:
            fprintf(out, "Method: A thread per row\n");
            break;
        case 2:
            fprintf(out, "Method: A thread per element\n");
            break;
    }
    fprintf(out, "row=%d col=%d\n", C->rows, C->cols);
    for (int i = 0; i < C->rows; i++) {
        for (int j = 0; j < C->cols; j++) {
            fprintf(out, "%d ", C->arr[i][j]);
        }
        fprintf(out, "\n");
    }
    fclose(out);
}


void *thread_per_matrix(void *args) {
    struct multiplication *per_matrix;
    per_matrix = (struct multiplication *) args;
    for (int i = 0; i < per_matrix->A->rows; i++) {
        for (int j = 0; j < per_matrix->B->cols; j++) {
            int sum = 0;
            for (int k = 0; k < per_matrix->B->rows; k++) {
                sum += per_matrix->A->arr[i][k] * per_matrix->B->arr[k][j];
            }
            per_matrix->C->arr[i][j] = sum;
        }
    }
    free(per_matrix);
    pthread_exit(NULL);
}

void thread_per_matrix_main(struct matrix *A, struct matrix *B) {
    struct multiplication *per_matrix = malloc(sizeof(struct multiplication));
    int **res = (int **) calloc(A->rows, sizeof(int *));
    for (int i = 0; i < A->rows; i++) {
        res[i] = (int *) calloc(B->cols, sizeof(int));
    }
    struct matrix *C = malloc(sizeof(struct matrix));
    C->rows = A->rows;
    C->cols = B->cols;
    C->arr = res;
    per_matrix->row = 0;
    per_matrix->col = 0;
    per_matrix->A = A;
    per_matrix->B = B;
    per_matrix->C = C;
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_per_matrix, (void *) per_matrix)) {
        printf("Can not create a thread\n");
        exit(1);
    }
    pthread_join(thread, NULL);
    write_matrix(0, C);
    for (int i = 0; i < C->rows; i++)
        free(C->arr[i]);
    free(C->arr);
    free(C);
    free(per_matrix);
}

void *thread_per_row(void *args) {
    struct multiplication *per_row;
    per_row = (struct multiplication *) args;
    for (int j = 0; j < per_row->B->cols; j++) {
        int sum = 0;
        for (int k = 0; k < per_row->B->rows; k++) {
            sum += per_row->A->arr[per_row->row][k] * per_row->B->arr[k][j];
        }
        per_row->C->arr[per_row->row][j] = sum;
    }
    free(per_row);
    pthread_exit(NULL);
}

void thread_per_row_main(struct matrix *A, struct matrix *B) {
    int n = A->rows;
    pthread_t threads[n];
    int **res = (int **) calloc(A->rows, sizeof(int *));
    for (int i = 0; i < A->rows; i++) {
        res[i] = (int *) calloc(B->cols, sizeof(int));
    }
    struct matrix *C = malloc(sizeof(struct matrix));
    C->rows = A->rows;
    C->cols = B->cols;
    C->arr = res;
    for (int i = 0; i < n; i++) {
        struct multiplication *per_row = malloc(sizeof(struct multiplication));
        per_row->row = i;
        per_row->col = 0;
        per_row->A = A;
        per_row->B = B;
        per_row->C = C;
        if (pthread_create(&threads[i], NULL, thread_per_row, (void *) per_row)) {
            printf("Can not create a thread\n");
            exit(1);
        }
    }
    for (int i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }
    write_matrix(1, C);
    for (int i = 0; i < C->rows; i++)
        free(C->arr[i]);
    free(C->arr);
    free(C);
}

void *thread_per_element(void *args) {
    struct multiplication *per_element;
    per_element = (struct multiplication *) args;
    int sum = 0;
    for (int k = 0; k < per_element->B->rows; k++) {
        sum += per_element->A->arr[per_element->row][k] * per_element->B->arr[k][per_element->col];
    }
    per_element->C->arr[per_element->row][per_element->col] = sum;
    free(per_element);
    pthread_exit(NULL);
}

void thread_per_element_main(struct matrix *A, struct matrix *B) {
    int rows = A->rows, cols = B->cols;
    pthread_t threads[rows][cols];
    int **res = (int **) calloc(A->rows, sizeof(int *));
    for (int i = 0; i < A->rows; i++) {
        res[i] = (int *) calloc(B->cols, sizeof(int));
    }
    struct matrix *C = malloc(sizeof(struct matrix));
    C->rows = A->rows;
    C->cols = B->cols;
    C->arr = res;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            struct multiplication *per_element = malloc(sizeof(struct multiplication));
            per_element->row = i;
            per_element->col = j;
            per_element->A = A;
            per_element->B = B;
            per_element->C = C;
            if (pthread_create(&threads[i][j], NULL, thread_per_row, (void *) per_element)) {
                printf("Can not create a thread\n");
                exit(1);
            }
        }
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            pthread_join(threads[i][j], NULL);
        }
    }
    write_matrix(2, C);
    for (int i = 0; i < C->rows; i++)
        free(C->arr[i]);
    free(C->arr);
    free(C);
}

int main(int argc, char *argv[]) {
    // if arguments is wrong
    if (argc != 1 && argc != 4) {
        printf("Error in arguments\n");
        return 0;
    }
    struct matrix *A = malloc(sizeof(struct matrix));
    struct matrix *B = malloc(sizeof(struct matrix));

    // no arguments
    if (argv[1] == NULL) {
        if (read_matrix("a", A) == -1)
            return 0;
        if (read_matrix("b", B) == -1)
            return 0;
        output = "c";
    } else {        // custom arguments
        if (read_matrix(argv[1], A) == -1)
            return 0;
        if (read_matrix(argv[2], B) == -1)
            return 0;
        output = argv[3];
    }
    if (A->cols != B->rows) {
        printf("Columns of first matrix must be equal to rows of second matrix\n");
        for (int i = 0; i < A->rows; i++)
            free(A->arr[i]);
        free(A->arr);
        free(A);
        for (int i = 0; i < B->rows; i++)
            free(B->arr[i]);
        free(B->arr);
        free(B);
        return 0;
    }
    struct timeval stop, start;

    // first method: a thread per matrix
    gettimeofday(&start, NULL); //start checking time
    thread_per_matrix_main(A, B);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);

    // second method: a thread per row
    gettimeofday(&start, NULL); //start checking time
    thread_per_row_main(A, B);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);

    // third method: a thread per element
    gettimeofday(&start, NULL); //start checking time
    thread_per_element_main(A, B);
    gettimeofday(&stop, NULL); //end checking time
    printf("Seconds taken %lu\n", stop.tv_sec - start.tv_sec);
    printf("Microseconds taken: %lu\n", stop.tv_usec - start.tv_usec);

    // free the heap
    for (int i = 0; i < A->rows; i++)
        free(A->arr[i]);
    free(A->arr);
    free(A);
    for (int i = 0; i < B->rows; i++)
        free(B->arr[i]);
    free(B->arr);
    free(B);
    return 0;
}