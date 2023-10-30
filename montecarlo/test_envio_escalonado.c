#include <stdio.h>

int ilog2(int n, int *sobran) {
    int n_original = n;

    int j;
    for (j = 0; n > 0; ++j, n>>=1) {}
    if (sobran != NULL)
        *sobran = n_original - (1 << (j-1));
    return j;
}

int main() {

    //                          8 9
	//                4 5 6 7 |------------------ sobrantes
	//          2 3 |---------| nivel 3
	//      1 |-----| nivel 2
	//  0 |---| nivel 1
	// ---| nivel 0
	//

    int n = 7;

    int sobran;
    int niveles = ilog2(n, &sobran);

    printf("%d procesos en %d niveles (%d sobrantes)\n", n, niveles, sobran);

    for (int i = 0; i < n; ++i) {
        printf("Process %d\n", i);

        int local_i;
        int nivel = ilog2(i, &local_i);

        printf("\tnivel %d\n", nivel);

        int receives;         
        if (nivel < (niveles-1)) {
            // Si no estamos en el último nivel, hacemos siempre
            // 2 receives, excepto el ROOT, que hace solo 1 receive.
            if (i == 0) {
                receives = 1;
            } else {
                receives = 2;
            }
        } else if (nivel == (niveles-1)) {
            // En el último nivel, el número de receives depende de
            // cuantos procesos queden sueltos.
            printf("\tLocal i %d\n", local_i);

            // Calcular cuantos procesos faltan por tener receptor.
            int faltan = sobran - (local_i*2);
            receives = faltan > 0 ? (faltan <= 2 ? faltan : 2) : 0;
        } else {
            // Los procesos sueltos no tienen que recibir nada.
            receives = 0;
        }
        printf("\tReceives %d\n", receives);

        if (i != 0) {
            int send_to = i >> 1;
            printf("\tSned to %d\n", send_to);
        }

    }

}