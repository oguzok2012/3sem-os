#ifndef sh
#define sh

#define SHM_NAME "/shm"
#define REQ_SEM_NAME "/req_sem"
#define RES_SEM_NAME "/res_sem"

typedef struct {
    int number;
    int stop_flag;
} shared_data_t;

#endif