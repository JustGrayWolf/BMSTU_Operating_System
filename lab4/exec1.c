#include <stdio.h>
#define N 10
#define M 10

void output(int **a2, int n, int m);

int input(int **a, int *n, int *m);

int sum(int *a, int m);

void makematrix(int **a, int n, int m);

void creat_matrix(int **a, int *buf);

void swap_int(int *el1, int *el2);

void calculate_sums(int **a, int *mas, int n, int m);

void swap_pointer_to_int(int **p1, int **p2);

int main()
{
    int buf[N][M];
    int *a[N];
    creat_matrix(a, *buf);
    int n, m;
    int code = input(a, &n, &m);
    if (code == 0) 
    {
        makematrix(a, n, m);
        output(a, n, m);
    }
    return code;
}

int input(int **a, int *n, int *m)
{
    int code = 0;
    int correct_count = 0;
    FILE *f = fopen("test1.txt", "r");
    if (!f)
    	return 1;
    if ((fscanf(f, "%d%d", n, m) == 2) && (*n <= N) && (*n > 0) && (*m <= M) && (*m > 0))
    {
        for (int i = 0; i < *n; i++)
            for (int j = 0; j < *m; j++)
                correct_count += fscanf(f, "%d", *(a + i) + j);
        if (correct_count != *n * *m) 
            code = 1;
    }
    else
        code = 1;
    return code;
}

void output(int **a, int n, int m)
{
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            printf("%d ", a[i][j]);
        }
        printf("\n");
    }
}

void swap_int(int *el1, int *el2)
{
    int b = *el1;
    *el1 = *el2;
    *el2 = b;
}

void swap_pointer_to_int(int **p1, int **p2)
{
    int *p = *p1;
    *p1 = *p2;
    *p2 = p;
}

void calculate_sums(int **a, int *mas, int n, int m)
{
    for (int i = 0; i < n; i++)
    {
        mas[i] = sum(a[i], m);
    }
}

void makematrix(int **a, int n, int m)
{
    int mas[N];
    calculate_sums(a, mas, n, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n - 1; j++)
        {
            if (mas[j] > mas[j + 1])
            {
                swap_int(mas + j, mas + j + 1);
                swap_pointer_to_int(a + j, a + j + 1);
            }
        }
}

int sum(int *a, int m)
{
    int s = 0;
    for (int j = 0; j < m; j++)
    {
        s += a[j];        
    }
    return s;
}


void creat_matrix(int **a, int *buf)
{
    for (int i = 0; i < N; i++)
        a[i] = buf + i * M;
}
