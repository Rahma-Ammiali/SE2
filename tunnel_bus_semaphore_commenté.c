// etudiante ammiali rahma g6 


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10

// Sémaphores pour gérer la synchronisation
sem_t mutex;        // Sémaphore binaire pour protéger les variables partagées
sem_t sens_XY;      // Sémaphore pour bloquer les bus de X vers Y
sem_t sens_YX;      // Sémaphore pour bloquer les bus de Y vers X

// Variables partagées
int nb_XY = 0;       // Nombre de bus actuellement dans le tunnel dans le sens X -> Y
int nb_YX = 0;       // Nombre de bus actuellement dans le sens Y -> X
int attente_XY = 0;  // Nombre de bus de X -> Y en attente
int attente_YX = 0;  // Nombre de bus de Y -> X en attente
int sens_courant = 0; // 0 = libre, 1 = X->Y, 2 = Y->X

// Structure pour passer les arguments au thread
typedef struct {
    int id;
    char ville;
} Bus;

void trajet(char depart, char arrivee, int id, int i) {
    printf("Bus %d de %c : %c -> %c (Trajet %d)\n", id, depart, depart, arrivee, i);
    usleep((rand() % 500 + 1000) * 1000);  // sleep aléatoire entre 1 et 1.5 secondes
}

void *fonction_bus(void *arg) {
    Bus *bus = (Bus *)arg;
    char depart, arrivee;
    int *nb, *attente;
    sem_t *sem_sens, *sem_opposée;

    for (int i = 1; i <= NB_TRAJETS; i++) {
        // ==== Déterminer le sens du trajet aller ====
        if (bus->ville == 'X') {
            depart = 'X'; arrivee = 'Y';
            nb = &nb_XY; attente = &attente_XY;
            sem_sens = &sens_XY; sem_opposée = &sens_YX;
        } else {
            depart = 'Y'; arrivee = 'X';
            nb = &nb_YX; attente = &attente_YX;
            sem_sens = &sens_YX; sem_opposée = &sens_XY;
        }

        // ==== Entrée dans le tunnel ====
        sem_wait(&mutex);  // Entrée section critique

        if ((sens_courant != 0 && sens_courant != (depart == 'X' ? 1 : 2)) || (*nb == 0 && *(depart == 'X' ? &nb_YX : &nb_XY) > 0)) {
            // Le tunnel est occupé dans l'autre sens
            (*attente)++;
            sem_post(&mutex);  // Libération du mutex avant de bloquer
            sem_wait(sem_sens); // Attente du feu vert
            sem_wait(&mutex);  // Reprise du mutex après réveil
            (*attente)--;
        }

        // Mise à jour : entrée autorisée
        sens_courant = (depart == 'X' ? 1 : 2);
        (*nb)++;

        sem_post(&mutex);  // Fin section critique

        trajet(depart, arrivee, bus->id, i);  // Simule le trajet

        // ==== Sortie du tunnel ====
        sem_wait(&mutex);  // Section critique

        (*nb)--;

        // Si plus aucun bus dans ce sens et des bus en attente de l'autre sens
        if (*nb == 0 && (*(depart == 'X' ? &attente_YX : &attente_XY) > 0)) {
            sens_courant = (depart == 'X' ? 1 : 2);
            for (int j = 0; j < *(depart == 'X' ? &attente_YX : &attente_XY); j++)
                sem_post(sem_opposée); // Lâche tous les bus de l'autre sens
        } else if (*nb == 0 && *(depart == 'X' ? &attente_YX : &attente_XY) == 0) {
            // Plus personne, on libère le tunnel
            sens_courant = 0;
        }

        sem_post(&mutex);  // Fin section critique

        // ==== Retour ====
        // Inverser les variables pour le trajet retour
        if (depart == 'X') {
            depart = 'Y'; arrivee = 'X';
            nb = &nb_YX; attente = &attente_YX;
            sem_sens = &sens_YX; sem_opposée = &sens_XY;
        } else {
            depart = 'X'; arrivee = 'Y';
            nb = &nb_XY; attente = &attente_XY;
            sem_sens = &sens_XY; sem_opposée = &sens_YX;
        }

        // ==== Entrée dans le tunnel pour le retour ====
        sem_wait(&mutex);

        if ((sens_courant != 0 && sens_courant != (depart == 'X' ? 1 : 2)) || (*nb == 0 && *(depart == 'X' ? &nb_YX : &nb_XY) > 0)) {
            (*attente)++;
            sem_post(&mutex);
            sem_wait(sem_sens);
            sem_wait(&mutex);
            (*attente)--;
        }

        sens_courant = (depart == 'X' ? 1 : 2);
        (*nb)++;

        sem_post(&mutex);

        trajet(depart, arrivee, bus->id, i);  // Simule le trajet retour

        // ==== Sortie du tunnel ====
        sem_wait(&mutex);

        (*nb)--;

        if (*nb == 0 && (*(depart == 'X' ? &attente_YX : &attente_XY) > 0)) {
            sens_courant = (depart == 'X' ? 1 : 2);
            for (int j = 0; j < *(depart == 'X' ? &attente_YX : &attente_XY); j++)
                sem_post(sem_opposée);
        } else if (*nb == 0 && *(depart == 'X' ? &attente_YX : &attente_XY) == 0) {
            sens_courant = 0;
        }

        sem_post(&mutex);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NB_BUS_X + NB_BUS_Y];
    Bus bus[NB_BUS_X + NB_BUS_Y];

    // Initialisation des sémaphores
    sem_init(&mutex, 0, 1);
    sem_init(&sens_XY, 0, 0);
    sem_init(&sens_YX, 0, 0);

    srand(time(NULL));  // Initialisation du générateur de nombres aléatoires

    // Création des threads pour les bus de X
    for (int i = 0; i < NB_BUS_X; i++) {
        bus[i].id = i + 1;
        bus[i].ville = 'X';
        pthread_create(&threads[i], NULL, fonction_bus, (void *)&bus[i]);
    }

    // Création des threads pour les bus de Y
    for (int i = 0; i < NB_BUS_Y; i++) {
        bus[NB_BUS_X + i].id = i + 1;
        bus[NB_BUS_X + i].ville = 'Y';
        pthread_create(&threads[NB_BUS_X + i], NULL, fonction_bus, (void *)&bus[NB_BUS_X + i]);
    }

    // Attente de la fin de tous les threads
    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destruction des sémaphores
    sem_destroy(&mutex);
    sem_destroy(&sens_XY);
    sem_destroy(&sens_YX);

    return 0;
}