#include "fake_error.h"

// se for 1, é erro, if zero, no erro
int data_error(int probability)
{
    time_t t1;
    int aux;

    srand((unsigned)time(&t1));

    aux = (rand() % 100);
    printf("aux->%d\n", aux);

    if (aux < probability)
    {
        return 0;
    }

    return 1;
}