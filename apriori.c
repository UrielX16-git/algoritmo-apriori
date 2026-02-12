#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINEA       1024
#define MAX_ITEMS_TRANS 128
#define CAP_INICIAL     64

typedef struct {
    int *elementos;   // Array dinamico de IDs de items (ordenado)
    int  cantidad;    // Numero de items en el conjunto (k)
    int  soporte;     // Frecuencia de aparicion
} ConjuntoItems;

typedef struct Nodo {
    ConjuntoItems *conjunto;
    struct Nodo   *siguiente;
} Nodo;
typedef struct {
    Nodo *cabeza;
    int   tamanio;
} ListaConjuntos;

typedef struct {
    int **transacciones;    // Array de arrays de IDs
    int  *tamanios;         // Tamanio de cada transaccion
    int   num_transacciones;
    int   capacidad;
} BaseDatos;

typedef struct {
    char **nombres;
    int    capacidad;
    int    cantidad;
} TablaSimbolos;

static TablaSimbolos tabla_global;

void inicializar_tabla(void) {
    tabla_global.capacidad = CAP_INICIAL;
    tabla_global.cantidad  = 0;
    tabla_global.nombres   = (char **)malloc(sizeof(char *) * tabla_global.capacidad);
}

int obtener_id_item(const char *nombre) {
    int i;
    for (i = 0; i < tabla_global.cantidad; i++) {
        if (strcmp(tabla_global.nombres[i], nombre) == 0)
            return i;
    }
    // No encontrado, agregar
    if (tabla_global.cantidad >= tabla_global.capacidad) {
        tabla_global.capacidad *= 2;
        tabla_global.nombres = (char **)realloc(tabla_global.nombres,
                                                 sizeof(char *) * tabla_global.capacidad);
    }
    tabla_global.nombres[tabla_global.cantidad] = (char *)malloc(strlen(nombre) + 1);
    strcpy(tabla_global.nombres[tabla_global.cantidad], nombre);
    return tabla_global.cantidad++;
}

// Libera la memoria de la tabla de simbolos
void liberar_tabla(void) {
    int i;
    for (i = 0; i < tabla_global.cantidad; i++)
        free(tabla_global.nombres[i]);
    free(tabla_global.nombres);
}

// Crea e inicializa una lista de conjuntos vacia
ListaConjuntos *crear_lista(void) {
    ListaConjuntos *lista = (ListaConjuntos *)malloc(sizeof(ListaConjuntos));
    lista->cabeza  = NULL;
    lista->tamanio = 0;
    return lista;
}

// Crea un ConjuntoItems con los elementos dados
ConjuntoItems *crear_conjunto(int *elementos, int cantidad) {
    ConjuntoItems *c = (ConjuntoItems *)malloc(sizeof(ConjuntoItems));
    c->elementos = (int *)malloc(sizeof(int) * cantidad);
    memcpy(c->elementos, elementos, sizeof(int) * cantidad);
    c->cantidad = cantidad;
    c->soporte  = 0;
    return c;
}

// Agrega un ConjuntoItems al inicio de la lista
void agregar_a_lista(ListaConjuntos *lista, ConjuntoItems *conjunto) {
    Nodo *nuevo = (Nodo *)malloc(sizeof(Nodo));
    nuevo->conjunto  = conjunto;
    nuevo->siguiente = lista->cabeza;
    lista->cabeza    = nuevo;
    lista->tamanio++;
}

// Libera memoria de una ListaConjuntos
void liberar_lista(ListaConjuntos *lista) {
    Nodo *actual = lista->cabeza;
    while (actual) {
        Nodo *temp = actual;
        actual = actual->siguiente;
        free(temp->conjunto->elementos);
        free(temp->conjunto);
        free(temp);
    }
    free(lista);
}

BaseDatos *cargar_transacciones(const char *archivo) {
    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", archivo);
        return NULL;
    }

    BaseDatos *bd = (BaseDatos *)malloc(sizeof(BaseDatos));
    bd->capacidad         = CAP_INICIAL;
    bd->num_transacciones = 0;
    bd->transacciones     = (int **)malloc(sizeof(int *) * bd->capacidad);
    bd->tamanios          = (int *)malloc(sizeof(int) * bd->capacidad);

    char linea[MAX_LINEA];
    while (fgets(linea, MAX_LINEA, fp)) {
        // Eliminar salto de linea
        linea[strcspn(linea, "\r\n")] = '\0';
        if (strlen(linea) == 0)
            continue;

        int items_temp[MAX_ITEMS_TRANS];
        int num_items = 0;

        char *token = strtok(linea, ",");
        while (token) {
            // Eliminar espacios al inicio
            while (*token == ' ') token++;
            // Eliminar espacios al final
            char *fin = token + strlen(token) - 1;
            while (fin > token && *fin == ' ') {
                *fin = '\0';
                fin--;
            }

            if (strlen(token) > 0) {
                items_temp[num_items++] = obtener_id_item(token);
            }
            token = strtok(NULL, ",");
        }

        if (num_items > 0) {
            // Ordenar items de la transaccion (burbuja simple)
            int i, j;
            for (i = 0; i < num_items - 1; i++) {
                for (j = 0; j < num_items - i - 1; j++) {
                    if (items_temp[j] > items_temp[j + 1]) {
                        int tmp        = items_temp[j];
                        items_temp[j]     = items_temp[j + 1];
                        items_temp[j + 1] = tmp;
                    }
                }
            }

            // Expandir si es necesario
            if (bd->num_transacciones >= bd->capacidad) {
                bd->capacidad *= 2;
                bd->transacciones = (int **)realloc(bd->transacciones,
                                                     sizeof(int *) * bd->capacidad);
                bd->tamanios = (int *)realloc(bd->tamanios,
                                               sizeof(int) * bd->capacidad);
            }

            int idx = bd->num_transacciones;
            bd->transacciones[idx] = (int *)malloc(sizeof(int) * num_items);
            memcpy(bd->transacciones[idx], items_temp, sizeof(int) * num_items);
            bd->tamanios[idx] = num_items;
            bd->num_transacciones++;
        }
    }

    fclose(fp);
    return bd;
}

// Libera memoria de la BaseDatos
void liberar_base_datos(BaseDatos *bd) {
    int i;
    for (i = 0; i < bd->num_transacciones; i++)
        free(bd->transacciones[i]);
    free(bd->transacciones);
    free(bd->tamanios);
    free(bd);
}

int es_subconjunto(int *a, int tam_a, int *b, int tam_b) {
    int i = 0, j = 0;
    while (i < tam_a && j < tam_b) {
        if (a[i] == b[j]) {
            i++;
            j++;
        } else if (b[j] < a[i]) {
            j++;
        } else {
            return 0; /* a[i] no esta en b */
        }
    }
    return (i == tam_a);
}

// Compara dos conjuntos elemento a elemento. Retorna 1 si son iguales.
int conjuntos_iguales(int *a, int *b, int tam) {
    int i;
    for (i = 0; i < tam; i++) {
        if (a[i] != b[i])
            return 0;
    }
    return 1;
}

// Busca si un conjunto de tamanio tam existe en la lista
int buscar_en_lista(ListaConjuntos *lista, int *elementos, int tam) {
    Nodo *actual = lista->cabeza;
    while (actual) {
        if (actual->conjunto->cantidad == tam &&
            conjuntos_iguales(actual->conjunto->elementos, elementos, tam))
            return 1;
        actual = actual->siguiente;
    }
    return 0;
}


// Muestra el conjunto en formato {Item1, Item2, ...}
void imprimir_conjunto(ConjuntoItems *conjunto) {
    int i;
    printf("{");
    for (i = 0; i < conjunto->cantidad; i++) {
        printf("%s", tabla_global.nombres[conjunto->elementos[i]]);
        if (i < conjunto->cantidad - 1)
            printf(", ");
    }
    printf("}");
}

/* Obtiene el soporte de un conjunto buscandolo en todas las listas frecuentes */
int obtener_soporte(ConjuntoItems *conjunto, ListaConjuntos **todos_frecuentes, int max_k) {
    int k = conjunto->cantidad - 1; /* indice en el array (0-based) */
    if (k < 0 || k >= max_k)
        return 0;
    Nodo *actual = todos_frecuentes[k]->cabeza;
    while (actual) {
        if (actual->conjunto->cantidad == conjunto->cantidad &&
            conjuntos_iguales(actual->conjunto->elementos, conjunto->elementos, conjunto->cantidad))
            return actual->conjunto->soporte;
        actual = actual->siguiente;
    }
    return 0;
}

// Recorre todas las transacciones, cuenta cada item y filtra por soporte minimo.
ListaConjuntos *obtener_frecuentes_1(BaseDatos *bd, int soporte_minimo) {
    // Conteo de frecuencia para cada item
    int *conteos = (int *)calloc(tabla_global.cantidad, sizeof(int));
    int i, j;

    for (i = 0; i < bd->num_transacciones; i++) {
        for (j = 0; j < bd->tamanios[i]; j++) {
            conteos[bd->transacciones[i][j]]++;
        }
    }

    ListaConjuntos *L1 = crear_lista();
    for (i = 0; i < tabla_global.cantidad; i++) {
        if (conteos[i] >= soporte_minimo) {
            int elem = i;
            ConjuntoItems *c = crear_conjunto(&elem, 1);
            c->soporte = conteos[i];
            agregar_a_lista(L1, c);
        }
    }

    free(conteos);
    return L1;
}

// Genera C_k (Join + Poda)
// Toma L_{k-1} y genera candidatos de tamanio k.
// Join: Une dos conjuntos que comparten los primeros k-2 elementos.
// Poda: Verifica que todos los subconjuntos de tamanio k-1 sean frecuentes.
ListaConjuntos *generar_candidatos(ListaConjuntos *L_anterior) {
    ListaConjuntos *candidatos = crear_lista();

    if (!L_anterior || L_anterior->tamanio == 0)
        return candidatos;

    int k_menos_1 = L_anterior->cabeza->conjunto->cantidad;
    int k         = k_menos_1 + 1;

    // Convertir lista a array para acceso indexado
    ConjuntoItems **arr = (ConjuntoItems **)malloc(sizeof(ConjuntoItems *) * L_anterior->tamanio);
    Nodo *n = L_anterior->cabeza;
    int idx = 0;
    while (n) {
        arr[idx++] = n->conjunto;
        n = n->siguiente;
    }
    int total = L_anterior->tamanio;

    int i, j, m;
    for (i = 0; i < total; i++) {
        for (j = i + 1; j < total; j++) {
            // Verificar que los primeros k-2 elementos sean iguales
            int prefijo_igual = 1;
            for (m = 0; m < k_menos_1 - 1; m++) {
                if (arr[i]->elementos[m] != arr[j]->elementos[m]) {
                    prefijo_igual = 0;
                    break;
                }
            }

            if (prefijo_igual && arr[i]->elementos[k_menos_1 - 1] < arr[j]->elementos[k_menos_1 - 1]) {
                // Crear nuevo candidato
                int *nuevo_elem = (int *)malloc(sizeof(int) * k);
                for (m = 0; m < k_menos_1; m++)
                    nuevo_elem[m] = arr[i]->elementos[m];
                nuevo_elem[k_menos_1] = arr[j]->elementos[k_menos_1 - 1];

                // Poda: verificar que todos los subconjuntos de tamanio k-1 estan en L_anterior
                int pasar_poda = 1;
                int sub[k_menos_1]; /* subconjunto temporal */
                int s, t;
                for (s = 0; s < k; s++) {
                    // Generar subconjunto excluyendo el elemento s
                    t = 0;
                    for (m = 0; m < k; m++) {
                        if (m != s)
                            sub[t++] = nuevo_elem[m];
                    }
                    if (!buscar_en_lista(L_anterior, sub, k_menos_1)) {
                        pasar_poda = 0;
                        break;
                    }
                }

                if (pasar_poda) {
                    ConjuntoItems *c = crear_conjunto(nuevo_elem, k);
                    agregar_a_lista(candidatos, c);
                }
                free(nuevo_elem);
            }
        }
    }

    free(arr);
    return candidatos;
}

// Recorre BD y cuenta frecuencia de cada candidato
// Para cada candidato, verifica en cuantas transacciones aparece como subconjunto.
void contar_soporte(ListaConjuntos *candidatos, BaseDatos *bd) {
    Nodo *actual = candidatos->cabeza;
    while (actual) {
        actual->conjunto->soporte = 0;
        int i;
        for (i = 0; i < bd->num_transacciones; i++) {
            if (es_subconjunto(actual->conjunto->elementos, actual->conjunto->cantidad,
                               bd->transacciones[i], bd->tamanios[i])) {
                actual->conjunto->soporte++;
            }
        }
        actual = actual->siguiente;
    }
}

// Retorna L_k eliminando los que no cumplen
// Crea una nueva lista solo con los candidatos cuyo soporte es >= soporte_minimo.
ListaConjuntos *filtrar_candidatos(ListaConjuntos *candidatos, int soporte_minimo) {
    ListaConjuntos *frecuentes = crear_lista();
    Nodo *actual = candidatos->cabeza;

    while (actual) {
        if (actual->conjunto->soporte >= soporte_minimo) {
            ConjuntoItems *copia = crear_conjunto(actual->conjunto->elementos,
                                                   actual->conjunto->cantidad);
            copia->soporte = actual->conjunto->soporte;
            agregar_a_lista(frecuentes, copia);
        }
        actual = actual->siguiente;
    }

    return frecuentes;
}

// Genera y filtra reglas de asociacion
void generar_reglas(ListaConjuntos **todos_frecuentes, int max_k, double confianza_minima) {
    int k;
    printf("\n========================================\n");
    printf("  REGLAS DE ASOCIACION\n");
    printf("  (Confianza minima: %.0f%%)\n", confianza_minima * 100);
    printf("========================================\n\n");

    int reglas_encontradas = 0;

    for (k = 1; k < max_k; k++) { /* k >= 2 (indice 1 en array 0-based) */
        Nodo *actual = todos_frecuentes[k]->cabeza;
        while (actual) {
            ConjuntoItems *S = actual->conjunto;
            int tam = S->cantidad;
            int soporte_S = S->soporte;

            // Generar todos los subconjuntos no vacios propios de S
            // Usar mascara de bits: desde 1 hasta (2^tam - 2)
            int total_subconj = (1 << tam) - 1;
            int mascara;
            for (mascara = 1; mascara < total_subconj; mascara++) {
                // Construir subconjunto X (bits encendidos) y complemento Z (bits apagados)
                int x_elem[tam], z_elem[tam];
                int x_tam = 0, z_tam = 0;
                int bit;
                for (bit = 0; bit < tam; bit++) {
                    if (mascara & (1 << bit))
                        x_elem[x_tam++] = S->elementos[bit];
                    else
                        z_elem[z_tam++] = S->elementos[bit];
                }

                // Obtener soporte de X
                ConjuntoItems x_temp;
                x_temp.elementos = x_elem;
                x_temp.cantidad  = x_tam;
                int soporte_X = obtener_soporte(&x_temp, todos_frecuentes, max_k);

                if (soporte_X > 0) {
                    double confianza = (double)soporte_S / soporte_X;
                    if (confianza >= confianza_minima) {
                        // Imprimir regla
                        int m;
                        printf("  {");
                        for (m = 0; m < x_tam; m++) {
                            printf("%s", tabla_global.nombres[x_elem[m]]);
                            if (m < x_tam - 1) printf(", ");
                        }
                        printf("} => {");
                        for (m = 0; m < z_tam; m++) {
                            printf("%s", tabla_global.nombres[z_elem[m]]);
                            if (m < z_tam - 1) printf(", ");
                        }
                        printf("}  [Confianza: %.2f%%  |  Soporte: %d]\n",
                               confianza * 100, soporte_S);
                        reglas_encontradas++;
                    }
                }
            }

            actual = actual->siguiente;
        }
    }

    if (reglas_encontradas == 0)
        printf("  No se encontraron reglas que cumplan los criterios.\n");
    else
        printf("\n  Total de reglas encontradas: %d\n", reglas_encontradas);
}

int main(int argc, char *argv[]) {
    // Validar argumentos
    if (argc < 4) {
        printf("Uso: %s <archivo_transacciones> <soporte_minimo> <confianza_minima>\n", argv[0]);
        printf("  soporte_minimo:   conteo absoluto (ej. 2)\n");
        printf("  confianza_minima: valor entre 0 y 1 (ej. 0.5)\n");
        return 1;
    }

    const char *archivo       = argv[1];
    int    soporte_minimo     = atoi(argv[2]);
    double confianza_minima   = atof(argv[3]);

    printf("========================================\n");
    printf("  ALGORITMO APRIORI\n");
    printf("========================================\n");
    printf("  Archivo:           %s\n", archivo);
    printf("  Soporte minimo:    %d\n", soporte_minimo);
    printf("  Confianza minima:  %.0f%%\n", confianza_minima * 100);
    printf("========================================\n\n");

    // Inicializar tabla de simbolos
    inicializar_tabla();

    // Cargar transacciones
    BaseDatos *bd = cargar_transacciones(archivo);
    if (!bd) {
        liberar_tabla();
        return 1;
    }

    printf("Transacciones cargadas: %d\n", bd->num_transacciones);
    printf("Items unicos: %d\n\n", tabla_global.cantidad);

    // Mostrar transacciones cargadas
    int i;
    printf("--- Base de Datos ---\n");
    for (i = 0; i < bd->num_transacciones; i++) {
        int j;
        printf("  T%d: {", i + 1);
        for (j = 0; j < bd->tamanios[i]; j++) {
            printf("%s", tabla_global.nombres[bd->transacciones[i][j]]);
            if (j < bd->tamanios[i] - 1) printf(", ");
        }
        printf("}\n");
    }
    printf("\n");

    // Almacenar todos los niveles de frecuentes
    int capacidad_niveles = 16;
    ListaConjuntos **todos_frecuentes = (ListaConjuntos **)malloc(
        sizeof(ListaConjuntos *) * capacidad_niveles);
    int max_k = 0;

    // Paso 1: L1
    ListaConjuntos *L_actual = obtener_frecuentes_1(bd, soporte_minimo);
    todos_frecuentes[max_k++] = L_actual;

    printf("--- L1 (Frecuentes de tamanio 1) ---\n");
    Nodo *n = L_actual->cabeza;
    while (n) {
        printf("  ");
        imprimir_conjunto(n->conjunto);
        printf("  soporte: %d\n", n->conjunto->soporte);
        n = n->siguiente;
    }
    printf("  Total: %d conjuntos frecuentes\n\n", L_actual->tamanio);

    // Bucle principal
    int k = 1;
    while (L_actual->tamanio > 0) {
        // Generar candidatos
        ListaConjuntos *candidatos = generar_candidatos(L_actual);

        if (candidatos->tamanio == 0) {
            liberar_lista(candidatos);
            break;
        }

        // Contar soporte
        contar_soporte(candidatos, bd);

        // Filtrar
        ListaConjuntos *L_nuevo = filtrar_candidatos(candidatos, soporte_minimo);
        liberar_lista(candidatos);

        if (L_nuevo->tamanio == 0) {
            liberar_lista(L_nuevo);
            break;
        }

        // Almacenar nivel
        if (max_k >= capacidad_niveles) {
            capacidad_niveles *= 2;
            todos_frecuentes = (ListaConjuntos **)realloc(todos_frecuentes,
                sizeof(ListaConjuntos *) * capacidad_niveles);
        }
        todos_frecuentes[max_k++] = L_nuevo;

        printf("--- L%d (Frecuentes de tamanio %d) ---\n", k + 1, k + 1);
        n = L_nuevo->cabeza;
        while (n) {
            printf("  ");
            imprimir_conjunto(n->conjunto);
            printf("  soporte: %d\n", n->conjunto->soporte);
            n = n->siguiente;
        }
        printf("  Total: %d conjuntos frecuentes\n\n", L_nuevo->tamanio);

        L_actual = L_nuevo;
        k++;
    }

    generar_reglas(todos_frecuentes, max_k, confianza_minima);

    // Limpieza de memoria
    printf("\n========================================\n");
    printf("  Algoritmo finalizado correctamente.\n");
    printf("========================================\n");

    for (i = 0; i < max_k; i++)
        liberar_lista(todos_frecuentes[i]);
    free(todos_frecuentes);
    liberar_base_datos(bd);
    liberar_tabla();

    return 0;
}
