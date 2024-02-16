#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdbool.h>
#include <math.h>

#define SHM_KEY 12363

typedef struct
{
    int i;
    int t;
    int dem1;
    int dem2;
    int dem3;
} Requette;

typedef struct
{
    int t;
    int nb1;
    int nb2;
    int nb3;
} Instruction;

typedef struct
{
    int alloc1;
    int alloc2;
    int alloc3;
} Alloc;

typedef struct
{
    int etat;
    // int nbr_req;
    double time_wait;
} Information;

typedef struct
{
    int t;
    int dem1;
    int dem2;
    int dem3;
} Demande;

typedef struct
{
    long int mtype;
    Requette req;
    int ok;
} Reponse;

typedef struct
{
    int q;
    int cpt;
    int p;
    Requette TR[3];
} Tampon; // shared memo

sem_t *mutex, *mutex1, *n_vide;
int TReponse;
int FLibre[5];
int TDispo[3] = {10, 10, 10};
Tampon *TRequette;
Demande TDemande[5];
Alloc TAlloc[5];
Information TInfo[5];
int CLE = 14543454;

void Init_stc()
{
    // Initialize TDemande array
    for (int i = 0; i < 5; i++)
    {
        TDemande[i].t = 0; // Set other values as needed
        TDemande[i].dem1 = 0;
        TDemande[i].dem2 = 0;
        TDemande[i].dem3 = 0;
    }

    // Initialize TAlloc array
    for (int i = 0; i < 5; i++)
    {
        TAlloc[i].alloc1 = 0; // Set other values as needed
        TAlloc[i].alloc2 = 0;
        TAlloc[i].alloc3 = 0;
    }

    // Initialize TInfo array
    for (int i = 0; i < 5; i++)
    {
        TInfo[i].etat = 0; // Set other values as needed
        TInfo[i].time_wait = 0.0;
    }
}
void Create_Init_sem()
{
    // init semaphores
    mutex = sem_open("mutex", O_CREAT | O_EXCL, 0644, 1);
    n_vide = sem_open("n_vide", O_CREAT | O_EXCL, 0644, 3);
    mutex1 = sem_open("mutex1", O_CREAT | O_EXCL, 0644, 1);
    sem_unlink("mutex");
    sem_unlink("n_vide");
    sem_unlink("mutex1");

    printf(" ** Init semaphores **\n");
}
void Delete_sem()
{
    sem_close(mutex);
    sem_close(n_vide);
    sem_close(mutex1);

    sem_unlink("mutex");
    sem_unlink("n_vide");
    sem_unlink("mutex1");
    printf(" ** Delete semaphores **\n");
}
void Create_queues()
{
    char *path = "./project.c"; // Use the same path for both queues

    // Create or get the first message queue
    if ((TReponse = msgget(ftok(path, (key_t)CLE), IPC_CREAT | IPC_EXCL | 0666)) == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    CLE++;
    printf(" ** Message queue created with ID: %d **\n", TReponse);
    for (int i = 0; i < 5; ++i) {
        if ((FLibre[i] = msgget(ftok(path, (key_t)CLE), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
            perror("msgget");
            exit(EXIT_FAILURE);
        }

    CLE++;
    printf(" ** Message queue created with ID: %d **\n", FLibre[i]);
    }
}
void Cleanup(int queue)
{
    // Remove the message queues when the program exits
    if (msgctl(queue, IPC_RMID, NULL) == -1)
    {
        perror("msgctl in Cleanup for TReponse");
        exit(EXIT_FAILURE);
    }
    printf(" ** Message queue with ID %d destroyed **\n", queue);
}
void Cleanup_queues()
{
    Cleanup(TReponse);
    Cleanup(FLibre[0]);
    Cleanup(FLibre[1]);
    Cleanup(FLibre[2]);
    Cleanup(FLibre[3]);
    Cleanup(FLibre[4]);
}
void Create_tmp()
{
    int shmid;

    // Create a shared memory segment
    if ((shmid = shmget(SHM_KEY, sizeof(Tampon), IPC_CREAT | 0666)) == -1)
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    printf(" ** Tampon created with ID: %d **\n", shmid);
    // Attach the shared memory segment
    if ((TRequette = (Tampon *)shmat(shmid, NULL, 0)) == (Tampon *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    printf(" ** Tampon attached **\n");
    // Initialize the shared memory segment (optional)
    TRequette->q = 0;
    TRequette->p = 0;
    TRequette->cpt = 0;
    for (int i = 0; i < 3; ++i)
    {
        TRequette->TR[i].i = 0;
        // printf("Tampon i : %d\n",TRequette->TR[i].i);
        TRequette->TR[i].t = 0;
        TRequette->TR[i].dem1 = 0;
        // printf("Tampon dem1 : %d\n",TRequette->TR[i].dem1);
        TRequette->TR[i].dem2 = 0;
        // printf("Tampon dem2 : %d\n",TRequette->TR[i].dem2);
        TRequette->TR[i].dem3 = 0;
        // printf("Tampon dem3 : %d\n",TRequette->TR[i].dem3);
    }
    printf(" ** Init Tampon **\n");
}
void Destroy_shm()
{
    // Detach shared memory
    shmdt(TRequette);
    printf(" ** Tampon dettached **\n");
    // Destroy shared memory
    int shmid;
    shmid = shmget(SHM_KEY, sizeof(Tampon), IPC_CREAT | 0666);
    shmctl(shmid, IPC_RMID, NULL);
    printf(" ** Tampon with ID %d destroyed with success **\n", shmid);

}
void dispose_req(Requette req)
{
    (*TRequette).TR[(*TRequette).q % 3].i = req.i;
    (*TRequette).TR[(*TRequette).q % 3].t = req.t;
    (*TRequette).TR[(*TRequette).q % 3].dem1 = req.dem1;
    (*TRequette).TR[(*TRequette).q % 3].dem2 = req.dem2;
    (*TRequette).TR[(*TRequette).q % 3].dem3 = req.dem3;

    printf(" ** Req (Calcul : %d) added to Tampon with success **",req.i);
    //printf("Tampon dispose : %d\n", TRequette->TR[0].i);
    //printf("Tampon dispose : %d\n", TRequette->TR[1].i);
    //printf("Tampon dispose : %d\n", TRequette->TR[2].i);
    (*TRequette).q++;
}
Requette retrieve_req()
{
    Requette req;
    req.i = (*TRequette).TR[((*TRequette).p) % 3].i;
    req.t = (*TRequette).TR[((*TRequette).p) % 3].t;
    req.dem1 = (*TRequette).TR[((*TRequette).p) % 3].dem1;
    req.dem2 = (*TRequette).TR[((*TRequette).p) % 3].dem2;
    req.dem3 = (*TRequette).TR[((*TRequette).p) % 3].dem3;

    (*TRequette).TR[((*TRequette).p) % 3].i = 0;
    (*TRequette).TR[((*TRequette).p) % 3].t = 0;
    (*TRequette).TR[((*TRequette).p) % 3].dem1 = 0;
    (*TRequette).TR[((*TRequette).p) % 3].dem2 = 0;
    (*TRequette).TR[((*TRequette).p) % 3].dem3 = 0;

    //printf("Tampon retrieve : %d\n", TRequette->TR[0].i);
    //printf("Tampon retrieve : %d\n", TRequette->TR[1].i);
    //printf("Tampon retrieve : %d\n", TRequette->TR[2].i);

    printf(" ** Req (Calcul : %d) retrieved from Tampon with success **",req.i);
    (*TRequette).p++;
    return req;
}

Reponse receive_message(int queue, int target_i)
{
    Reponse message;
    // Receive a message from the TReponse queue with a specific "i" value
    if (msgrcv(queue, &message, sizeof(Reponse), target_i, 0) == -1)
    {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }

    // Process the received message
    printf(" ** Received Message from TRequette : i=%ld, ok=%d **\n", message.mtype, message.ok);
    return message;
}
void send_message(int msgid, Requette req, int ok)
{
    Reponse message;

    // Prepare the message
    message.mtype = req.i; // Set the message type
    message.req = req;
    message.ok = ok;

    // Send the message to the queue
    if (msgsnd(msgid, &message, sizeof(Reponse), 0) == -1)
    {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    printf(" ** Message sent to the queue for Calcul %d **\n", req.i);
}
Instruction Read(FILE *fp)
{
    Instruction inst;
    fscanf(fp, "%d", &inst.t);
    fscanf(fp, "%d", &inst.nb1);
    fscanf(fp, "%d", &inst.nb2);
    fscanf(fp, "%d", &inst.nb3);
    return inst;
}

int compareIndexedTime(int a, int b, Information info[], int order)
{
    // Check if etat is 1 for both indices
    double diff;
    if (order == 1)
    {
        // Sorting in ascending order (from the lowest to the highest)
        diff = info[a].time_wait - info[b].time_wait;
    }
    else
    {
        // Sorting in descending order (from the highest to the lowest)
        diff = info[b].time_wait - info[a].time_wait;
    }

    return (diff > 0) - (diff < 0);
}

void satisfy_other()
{
    Requette req;
    bool trouver = 0;
    int sortedIndexes[5] = {-1, -1, -1, -1, -1};
    int t = 0;
    printf(" ** Satisfy other **\n");
    for (int i = 0; i < 5; i++)
    {
        if (TInfo[i].etat == 1)
        {
            sortedIndexes[t] = i;
            t++;
        }
    }

    if (t != 0)
    {
        const size_t size = 5; // Assuming a fixed size of 5

        int order = 0; // 1 for ascending order, 0 for descending order

        // Bubble sort
        for (int i = 0; i < t - 1; ++i)
        {
            for (int j = 0; j < t - i - 1; ++j)
            {
                int index1 = sortedIndexes[j];
                int index2 = sortedIndexes[j + 1];

                int comparisonResult = compareIndexedTime(index1, index2, TInfo, order);

                if ((order == 1 && comparisonResult > 0) || (order == 0 && comparisonResult < 0))
                {
                    // Swap the indices
                    int temp = sortedIndexes[j];
                    sortedIndexes[j] = sortedIndexes[j + 1];
                    sortedIndexes[j + 1] = temp;
                }
            }
        }

        // printf(" index 1 satisfy_other: %d\n",sortedIndexes[0]);
        int i = 0;
        while (sortedIndexes[i] != -1 && trouver != 1)
        {
            if (TDemande[sortedIndexes[i]].dem1 <= TDispo[0] && TDemande[sortedIndexes[i]].dem2 <= TDispo[1] && TDemande[sortedIndexes[i]].dem3 <= TDispo[2])
            {
                printf(" ** Satisty other yes **\n");
                //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
                //printf("alloc %d %d %d\n", TAlloc[sortedIndexes[i]].alloc1,TAlloc[sortedIndexes[i]].alloc2,TAlloc[sortedIndexes[i]].alloc3);
                TDispo[0] -= TDemande[sortedIndexes[i]].dem1;
                TDispo[1] -= TDemande[sortedIndexes[i]].dem2;
                TDispo[2] -= TDemande[sortedIndexes[i]].dem3;
                TAlloc[sortedIndexes[i]].alloc1 += TDemande[sortedIndexes[i]].dem1; // Set other values as needed
                TAlloc[sortedIndexes[i]].alloc2 += TDemande[sortedIndexes[i]].dem2;
                TAlloc[sortedIndexes[i]].alloc3 += TDemande[sortedIndexes[i]].dem3;
                //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
                //printf("alloc %d %d %d\n", TAlloc[sortedIndexes[i]].alloc1,TAlloc[sortedIndexes[i]].alloc2,TAlloc[sortedIndexes[i]].alloc3);
                TInfo[sortedIndexes[i]].etat = 0;
                TInfo[sortedIndexes[i]].time_wait = 0;
                TDemande[sortedIndexes[i]].t = 2; // Set other values as needed
                TDemande[sortedIndexes[i]].dem1 = 0;
                TDemande[sortedIndexes[i]].dem2 = 0;
                TDemande[sortedIndexes[i]].dem3 = 0;
                req.i = sortedIndexes[i] + 1;
                req.t = 2;
                trouver = 1;
                send_message(TReponse, req, 1);
                printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
                break;
            }
            i++;
        }
    }
}
bool satisfy(Requette req)
{
    int sortedIndexes[5] = {-1, -1, -1, -1, -1};

    if (req.dem1 <= TDispo[0] && req.dem2 <= TDispo[1] && req.dem3 <= TDispo[2])
    {
        printf(" ** Satisfy Calcul %d from TDispo directly **\n", req.i);
        //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
        //printf("alloc %d %d %d\n", TAlloc[req.i - 1].alloc1,TAlloc[req.i - 1].alloc2,TAlloc[req.i - 1].alloc3);
        TDispo[0] -= req.dem1;
        TDispo[1] -= req.dem2;
        TDispo[2] -= req.dem3;
        TAlloc[req.i - 1].alloc1 += req.dem1; // Set other values as needed
        TAlloc[req.i - 1].alloc2 += req.dem2;
        TAlloc[req.i - 1].alloc3 += req.dem3;
        printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
        //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
        //printf("alloc %d %d %d\n", TAlloc[req.i - 1].alloc1,TAlloc[req.i - 1].alloc2,TAlloc[req.i - 1].alloc3);
        return true;
    }
    else
    {
        int avb1 = TDispo[0], avb2 = TDispo[1], avb3 = TDispo[2];
        int rem1 = req.dem1, rem2 = req.dem2, rem3 = req.dem3;
        int allc1,allc2,allc3;
        //printf("satisfy else %d\n", req.i);
        int t = 0;
        for (int i = 0; i < 5; i++)
        {
            if (TInfo[i].etat == 1)
            {
                sortedIndexes[t] = i;
                t++;
            }
        }
        if (t > 0)
        {
            const size_t size = 5; // Assuming a fixed size of 5

            int order = 1; // 1 for ascending order, 0 for descending order

            // Bubble sort
            for (int i = 0; i < t - 1; ++i)
            {
                for (int j = 0; j < t - i - 1; ++j)
                {
                    int index1 = sortedIndexes[j];
                    int index2 = sortedIndexes[j + 1];

                    int comparisonResult = compareIndexedTime(index1, index2, TInfo, order);

                    if ((order == 1 && comparisonResult > 0) || (order == 0 && comparisonResult < 0))
                    {
                        // Swap the indices
                        int temp = sortedIndexes[j];
                        sortedIndexes[j] = sortedIndexes[j + 1];
                        sortedIndexes[j + 1] = temp;
                    }
                }
            }
            //printf(" index 1 : %d\n", sortedIndexes[0]);
            int v = 0;
            // printf("check %d\n", sortedIndexes[2]);
            while (sortedIndexes[v] != -1)
            {
                avb1 += TAlloc[sortedIndexes[v]].alloc1;
                avb2 += TAlloc[sortedIndexes[v]].alloc2;
                avb3 += TAlloc[sortedIndexes[v]].alloc3;
                v++;
            }
            //printf("avb %d %d %d\n", avb1, avb2, avb3);
            if (req.dem1 <= avb1 && req.dem2 <= avb2 && req.dem3 <= avb3)
            {
                //printf("avb\n");
                rem1 -= fmin(TDispo[0], req.dem1); // 2 dispo:4 red : 7
                rem2 -= fmin(TDispo[1], req.dem2);
                rem3 -= fmin(TDispo[2], req.dem3);

                //printf("rem %d %d %d\n", rem1,rem2,rem3);
                //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);

                TDispo[0] -= fmin(TDispo[0], req.dem1);
                TDispo[1] -= fmin(TDispo[1], req.dem2);
                TDispo[2] -= fmin(TDispo[2], req.dem3);

                //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
                int c = 0;
                while (sortedIndexes[c] != -1)
                {
                    if (rem1 > 0)
                    {
                        allc1 = fmin(TAlloc[sortedIndexes[c]].alloc1, rem1);
                        TAlloc[sortedIndexes[c]].alloc1 -= allc1;
                        TDemande[sortedIndexes[c]].dem1 += allc1;
                        rem1 -= allc1;
                    }
                    if (rem2 > 0)
                    {
                        allc2 = fmin(TAlloc[sortedIndexes[c]].alloc2, rem2);
                        TAlloc[sortedIndexes[c]].alloc2 -= allc2;
                        TDemande[sortedIndexes[c]].dem2 += allc2;
                        rem2 -= allc2;
                    }
                    if (rem3 > 0)
                    {
                        allc3 = fmin(TAlloc[sortedIndexes[c]].alloc3,rem3);
                        TAlloc[sortedIndexes[c]].alloc3 -= allc3;
                        TDemande[sortedIndexes[c]].dem3 += allc3;
                        rem3 -= allc3;
                    }
                    c++;
                }
                TAlloc[req.i - 1].alloc1 += req.dem1; // Set other values as needed
                TAlloc[req.i - 1].alloc2 += req.dem2;
                TAlloc[req.i - 1].alloc3 += req.dem3;
                //printf("alloc %d %d %d\n", TAlloc[0].alloc1,TAlloc[0].alloc2,TAlloc[0].alloc3);
                //printf("alloc %d %d %d\n", TAlloc[1].alloc1,TAlloc[1].alloc2,TAlloc[1].alloc3);
                //printf("alloc %d %d %d\n", TAlloc[2].alloc1,TAlloc[2].alloc2,TAlloc[2].alloc3);
                //printf("alloc %d %d %d\n", TAlloc[3].alloc1,TAlloc[3].alloc2,TAlloc[3].alloc3);
                //printf("alloc %d %d %d\n", TAlloc[4].alloc1,TAlloc[4].alloc2,TAlloc[4].alloc3);
                printf(" ** Satisfy Calcul %d from TDispo and blocked processes **\n", req.i);
                printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
                return true;
            }
            else
                return false;
        }
        else
            return false;
    }
}
void Gerant()
{
    bool trouver, satisfied; /// find smth in Queue and tmp
    int nbproc = 5, type,started_index = 0;
    // init structs
    Requette req;
    Reponse rep;

    Init_stc();
    do
    {
        trouver = 0;
        do
        {
            sem_wait(mutex1);
            if ((*TRequette).cpt != 0)
            {
                printf(" ** Gerant found : Tampon **\n");
                sem_post(mutex1);
                sem_wait(mutex);
                req = retrieve_req();
                type = req.t;
                sem_post(mutex);
                trouver = 1;
                sem_wait(mutex1);
                (*TRequette).cpt--;
                sem_post(mutex1);
                sem_post(n_vide);
            }
            else
            {
                sem_post(mutex1);
                for (int j = 0; j < 5; ++j)
                {
                        if (msgrcv(FLibre[(started_index + j) % 5], &rep, sizeof(Reponse), 0, IPC_NOWAIT) != -1)
                        {
                            printf(" ** Received a message from FLibre%d **\n",rep.req.i);
                            type = 3;
                            trouver = 1;
                            started_index = j ;
                            break; // Exit the loop when a message is found
                        }
                }
            }
        } while (trouver == 0);
        switch (type)
        {
        case 2:
            printf(" ** Gerant Found req : 2 **\n");
            satisfied = satisfy(req);
            if (satisfied == true)
            {
                //printf(" ** Calcul %d satisfied **\n",req.i);
                send_message(TReponse, req, 1);
                printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
            }
            else
            {
                printf(" ** Calcul %d not satisfied **\n",req.i);
                send_message(TReponse, req, 0);
                TInfo[req.i - 1].etat = 1; // Set other values as needed
                TInfo[req.i - 1].time_wait = (double)time(NULL);
                TDemande[req.i - 1].t = 2; // Set other values as needed
                TDemande[req.i - 1].dem1 += req.dem1;
                TDemande[req.i - 1].dem2 += req.dem2;
                TDemande[req.i - 1].dem3 += req.dem3;
            }
            break;
        case 3:
            printf(" ** Gerant Found req : 3 **\n");
            //printf("dispo %d %d %d\n", TDispo[0],TDispo[1],TDispo[2]);
            //printf("alloc %d %d %d\n", TAlloc[rep.req.i - 1].alloc1,TAlloc[rep.req.i - 1].alloc2,TAlloc[rep.req.i - 1].alloc3);
            TAlloc[rep.req.i - 1].alloc1 -= rep.req.dem1;
            TAlloc[rep.req.i - 1].alloc2 -= rep.req.dem2;
            TAlloc[rep.req.i - 1].alloc3 -= rep.req.dem3;
            TDispo[0] += rep.req.dem1;
            TDispo[1] += rep.req.dem2;
            TDispo[2] += rep.req.dem3;
            printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
            //printf("alloc %d %d %d\n", TAlloc[rep.req.i - 1].alloc1,TAlloc[rep.req.i - 1].alloc2,TAlloc[rep.req.i - 1].alloc3);
            satisfy_other();
            break;

        case 4:
            nbproc--;
            // free res
            printf(" ** Gerant Found req : 4 **\n");
            TDispo[0] += TAlloc[req.i - 1].alloc1;
            TDispo[1] += TAlloc[req.i - 1].alloc2;
            TDispo[2] += TAlloc[req.i - 1].alloc3;
            printf(" ** TDispo :  %d %d %d **\n", TDispo[0],TDispo[1],TDispo[2]);
            printf(" ** N° Calcul left : %d **\n", nbproc);
            satisfy_other();
            break;
        }
    } while (nbproc != 0);

    printf(" ** Gerant ended **\n");
    exit(0);
}

void Calcul(int i)
{
    int id, num_req, FLibrei;
    Requette req;
    Reponse rep;
    Instruction inst;
    FILE *fp, *fp1;
    switch (i)
    {
    case 1:
        fp = fopen("file1.txt", "r");
        break;
    case 2:
        fp = fopen("file2.txt", "r");
        break;
    case 3:
        fp = fopen("file3.txt", "r");
        break;
    case 4:
        fp = fopen("file4.txt", "r");
        break;
    case 5:
        fp = fopen("file5.txt", "r");
        break;
    }
    FLibrei = FLibre[i - 1];
    if (fscanf(fp, "%d", &inst.t) != 1)
    {
        printf("files%d empty\n", i);
    }
    else
    {
        rewind(fp);
        do
        {
            inst = Read(fp);
            switch (inst.t)
            {
            case 1:
                printf(" ** Calcul %d executed inst %d ** \n", i, inst.t);
                sleep(1);
                break;
            case 2:
                printf(" ** Calcul %d executed inst %d ** \n", i, inst.t);
                req.i = i;
                req.t = 2;
                req.dem1 = inst.nb1;
                req.dem2 = inst.nb2;
                req.dem3 = inst.nb3;
                sem_wait(n_vide);
                sem_wait(mutex);
                dispose_req(req);
                printf(" ** Calcul %d inserted inst 2 into the Tampon with success **\n", i);
                sem_post(mutex);
                sem_wait(mutex1);
                (*TRequette).cpt++;
                // printf("cpt %d\n",(*TRequette).cpt);
                sem_post(mutex1);
                rep = receive_message(TReponse, i);
                if (rep.ok < 1)
                    rep = receive_message(TReponse, i);
                printf(" ** Calcul %d received ressources with success **\n", i);
                break;
            case 3:
                printf(" ** Calcul %d executed inst %d ** \n", i, inst.t);
                req.i = i;
                req.t = 3;
                req.dem1 = inst.nb1;
                req.dem2 = inst.nb2;
                req.dem3 = inst.nb3;
                send_message(FLibrei, req, 1);
                break;
            case 4:
                printf(" ** Calcul %d executed inst %d ** \n", i, inst.t);
                //sleep(1);
                req.i = i;
                req.t = 4;
                req.dem1 = inst.nb1;
                req.dem2 = inst.nb2;
                req.dem3 = inst.nb3;
                sem_wait(n_vide);
                sem_wait(mutex);
                dispose_req(req);
                printf(" ** Calcul %d inserted inst 4 into the Tampon with success **\n", i);
                sem_post(mutex);
                sem_wait(mutex1);
                (*TRequette).cpt++;
                sem_post(mutex1);
                printf(" ** End Calcul %d **\n", i);
                break;
            }

        } while (inst.t != 4);
    }
    fclose(fp);
    exit(0);
}
int main()
{
    printf("inside main\n");
    Create_Init_sem();
    Create_queues();
    Create_tmp();
    int id, i;
    id = fork();
    if (id == 0)
        Gerant();
    for (i = 1; i <= 5; i++)
    {
        id = fork();
        if (id == 0)
            Calcul(i);
    }
    for (i = 0; i < 6; i++) wait(0);

    Cleanup_queues();
    Delete_sem();
    Destroy_shm();
    return 0;
}
