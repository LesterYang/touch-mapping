/*
 * tmMap.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */
#include <string.h>
#include <stdlib.h>

#include "tmMapping.h"
#include "qUtils.h"
#include "tm.h"
#include "tmInput.h"

#if 1

typedef struct _tm_handler
{
    q_mutex*        mutex;
    q_queue*        queue;
    tm_config_t*    cal;
    tm_fb_param_t*  fb_param;
}tm_handler_t;

tm_handler_t tm_handler;

tm_fb_param_t g_fb_param[] = {
    {TM_PANEL_FRONT,    800, 480, 0, 0, 0},
    {TM_PANEL_LEFT,    1000, 600, 0, 0, 0},
    {TM_PANEL_RIGHT,   1000, 600, 0, 0, 0},
    {TM_PANEL_NONE,       0,   0, 0, 0, 0}
};

tm_config_t g_cal[] = {
    {TM_PANEL_FRONT, {{{1, 0, 0}, {0, 1, 0}}}, 1, 0, {0, 1, 1}},
    {TM_PANEL_LEFT,  {{{1, 0, 0}, {0, 1, 0}}}, 1, 0, {0, 1, 1}},
    {TM_PANEL_RIGHT, {{{1, 0, 0}, {0, 1, 0}}}, 1, 0, {0, 1, 1}},
    {TM_PANEL_NONE,  {{{0, 0, 0}, {0, 0, 0}}}, 0, 0, {0, 0, 0}}
};


tm_errno_t  tm_mapping_create_handler()
{
    tm_handler.fb_param = g_fb_param;
    tm_handler.cal = g_cal;

    //tm_mapping_set_default_param();
    tm_mapping_update_conf();

    tm_handler.mutex = q_mutex_new(q_true, q_true);
    tm_handler.queue = q_create_queue(MAX_QUEUE);

    return TM_ERRNO_SUCCESS;
}

void tm_mapping_destroy_handler()
{
    q_mutex_free(tm_handler.mutex);
    q_destroy_queue(tm_handler.queue);
}

tm_errno_t tm_mapping_update_conf()
{
    int n, idx = -1, row, col;
    tm_trans_matrix_t matrix;
    tm_config_t* cal;
    FILE *fr;
    char buf[BUF_SIZE];
    char *conf_file = NULL;
    char *default_conf = QSI_TM_CONF;
    char *param;
    int matrix_num = CAL_MATRIX_ROW * CAL_MATRIX_COL;

    if ( (conf_file = getenv("QSI_TM_CONF")) == NULL )
        conf_file = default_conf;

    q_dbg("configure file is %s", conf_file);

    fr = fopen(conf_file, "r");

    if(fr == NULL)
    {
        qerror("open configuration");
        return TM_ERRNO_NO_DEV;
    }

    while( !feof(fr) )
    {
        memset(buf, 0, BUF_SIZE);
        if(fgets(buf, BUF_SIZE, fr) == NULL)
            continue;

        if(buf[0] == '#' || buf[0] == 0 || buf[BUF_SIZE - 2] != 0)
            continue;

        if((param = strtok(buf," ")) == NULL || (strlen(param) != 1))
            continue;

        idx = param[0] - '0';

        if (idx < 0 || idx >= TM_PANEL_NUM)
            continue;

        cal = &tm_handler.cal[idx];

        for(n=0; n<matrix_num; n++)
        {
            if((param = strtok(NULL," ")) == NULL)
                break;

            row = n/CAL_MATRIX_COL;
            col = n%CAL_MATRIX_COL;
            matrix.element[row][col] = atoi(param);
        }

        if((n == matrix_num) && ((param = strtok(NULL," ")) != NULL))
        {
            cal->scaling = atoi(param);
            memcpy(&cal->trans_matrix, &matrix, sizeof(tm_trans_matrix_t));
        }

#if 0 //test
        fprintf(stderr, "b %d : %2d %2d %2d %2d %2d %2d %2d\n",
                idx,
                tm_handler.cal[idx].trans_matrix.element[0][0],
                tm_handler.cal[idx].trans_matrix.element[0][1],
                tm_handler.cal[idx].trans_matrix.element[0][2],
                tm_handler.cal[idx].trans_matrix.element[0][3],
                tm_handler.cal[idx].trans_matrix.element[0][4],
                tm_handler.cal[idx].trans_matrix.element[0][5],
                tm_handler.cal[idx].scaling);

        fprintf(stderr, "f %d : %2d %2d %2d %2d %2d\n",
                idx,
                tm_handler.fb_param[idx].x,
                tm_handler.fb_param[idx].y,
                tm_handler.fb_param[idx].horizontal,
                tm_handler.fb_param[idx].vertical,
                tm_handler.fb_param[idx].swap);


        fprintf(stderr,"=====\n");
#endif

    }

    fclose(fr);
    return TM_ERRNO_SUCCESS;
}


void tm_mapping_matrix_mult(tm_trans_matrix_t *matrix, int* vector)
{
    int i, j;
    int result_vec[2] = {0};

    for(i=0; i<CAL_MATRIX_ROW; i++)
    {
        for(j=0; j<CAL_MATRIX_COL; j++)
        {
            result_vec[i] +=  matrix->element[i][j] * vector[j];
        }
    }

#if 1
    printf("matrix_mult:\n");
    printf("|%5d %5d %5d| |%5d|   |%5d|\n",matrix->element[0][0],matrix->element[0][1],matrix->element[0][2],vector[0],result_vec[0]);
    printf("|%5d %5d %5d| |%5d| = |%5d|\n",matrix->element[1][0],matrix->element[1][1],matrix->element[1][2],vector[1],result_vec[1]);
    printf("|    0     0     0| |%5d|   |    0|\n",vector[2]);
#endif

    vector[0] = result_vec[0];
    vector[1] = result_vec[1];
}

tm_errno_t tm_mapping_transfer(int *x, int *y, tm_config_t* config, tm_fb_param_t*  src_fb, tm_fb_param_t*  dest_fb)
{
    static int last_x[TM_PANEL_NUM];
    static int last_y[TM_PANEL_NUM];
    int idx = config->name;
    union {
         int vec[3];
         struct{
             int x;
             int y;
             int z;
         };
     }coord;

     q_assert(config);
     q_assert(src_fb);
     q_assert(dest_fb);

     if (idx >= TM_PANEL_NUM || idx < 0)
        return TM_ERRNO_DEV_NUM;

    if(*x != 0)
        coord.x = last_x[idx] = *x;
    else if (last_x[idx] != 0)
        coord.x = last_x[idx];
    else
        return TM_ERRNO_POINT;

    if(*y != 0)
        coord.y = last_y[idx] = *y;
    else if (last_y[idx] != 0)
        coord.y = last_y[idx];
    else
        return TM_ERRNO_POINT;

    coord.z = 1;

    // raw touch point -> frame buffer point
    tm_mapping_matrix_mult(&config->trans_matrix, coord.vec);

    if (src_fb == dest_fb)
    {
        *x = coord.x / config->scaling;
        *y = coord.y / config->scaling;
        return TM_ERRNO_SUCCESS;
    }


    // frame buffer -> frame buffer

    coord.x /= config->scaling;
    coord.y /= config->scaling;

    if(src_fb->swap)
    {
        coord.z = coord.x;
        coord.x = coord.y;
        coord.y = coord.z;
        coord.z = 1;
    }

    if(src_fb->horizontal != dest_fb->horizontal)
        coord.x = src_fb->max_x - coord.x;

    if(src_fb->vertical != dest_fb->vertical)
        coord.y = src_fb->max_y - coord.y;

    coord.x = (dest_fb->max_x * coord.x) / src_fb->max_x;
    coord.y = (dest_fb->max_y * coord.y) / src_fb->max_y;

    if(dest_fb->swap)
    {
        coord.z = coord.x;
        coord.x = coord.y;
        coord.y = coord.z;
        coord.z = 1;
    }

    *x = coord.x;
    *y = coord.y;
    return TM_ERRNO_SUCCESS;
}


#if 0 //check frame buffer mapping
#define test_src_x 800
#define test_src_y 480
#define test_dest_x 1000
#define test_dest_y 600
tm_fb_param_t test_src_param[] = {
    {0, test_src_x, test_src_y, 0, 0, 0}, //lt
    {1, test_src_x, test_src_y, 0, 0, 1},
    {2, test_src_x, test_src_y, 1, 0, 0}, //rt
    {3, test_src_x, test_src_y, 1, 0, 1},
    {4, test_src_x, test_src_y, 1, 1, 0}, //rb
    {5, test_src_x, test_src_y, 1, 1, 1},
    {6, test_src_x, test_src_y, 0, 1, 0}, //lb
    {7, test_src_x, test_src_y, 0, 1, 1}
};

tm_fb_param_t test_dest_param[] = {
    {0, test_dest_x, test_dest_y, 0, 0, 0},
    {0, test_dest_x, test_dest_y, 0, 0, 1},
    {0, test_dest_x, test_dest_y, 1, 0, 0},
    {0, test_dest_x, test_dest_y, 1, 0, 1},
    {0, test_dest_x, test_dest_y, 1, 1, 0},
    {0, test_dest_x, test_dest_y, 1, 1, 1},
    {0, test_dest_x, test_dest_y, 0, 1, 0},
    {0, test_dest_x, test_dest_y, 0, 1, 1},
};

tm_fb_param_t src_point[] = {
    {0, 300, 120, 0, 0, 0},
    {0, 120, 300, 0, 0, 0},
    {0, 500, 120, 0, 0, 0},
    {0, 120, 500, 0, 0, 0},
    {0, 500, 360, 0, 0, 0},
    {0, 360, 500, 0, 0, 0},
    {0, 300, 360, 0, 0, 0},
    {0, 360, 300, 0, 0, 0},
};

tm_fb_param_t ans[] = {
    {0, 375, 150, 0, 0, 0},
    {0, 150, 375, 0, 0, 0},
    {0, 625, 150, 0, 0, 0},
    {0, 150, 625, 0, 0, 0},
    {0, 625, 450, 0, 0, 0},
    {0, 450, 625, 0, 0, 0},
    {0, 375, 450, 0, 0, 0},
    {0, 450, 375, 0, 0, 0},
};

void tm_mapping_test()
{
    int i,j;
    tm_config_t conf;
    tm_config_t* config = &conf;
    tm_fb_param_t* src_fb;
    tm_fb_param_t* dest_fb;
    static union {
        int vec[3];
        struct{
            int x;
            int y;
            int z;
        };
    }coord;

    conf.scaling = 1;

    for(i=0;i<8;i++)
    {
        coord.x = src_point[i].max_x;
        coord.y = src_point[i].max_y;
        coord.z = 1;

        printf("%d : %d %d\n",i, coord.x,coord.y);
        src_fb = &test_src_param[i];

        for(j=0;j<8;j++)
        {
            coord.x = src_point[i].max_x;
            coord.y = src_point[i].max_y;
            coord.z = 1;

            dest_fb = &test_dest_param[j];

            if (src_fb == dest_fb)
            {
                continue;
            }

            if(src_fb->swap)
            {
                coord.z = coord.x;
                coord.x = coord.y;
                coord.y = coord.z;
                coord.z = 1;
            }

            if(src_fb->horizontal != dest_fb->horizontal)
                coord.x = src_fb->max_x - coord.x;

            if(src_fb->vertical != dest_fb->vertical)
                coord.y = src_fb->max_y - coord.y;

            coord.x = (dest_fb->max_x * coord.x) / (src_fb->max_x * config->scaling);
            coord.y = (dest_fb->max_y * coord.y) / (src_fb->max_y * config->scaling);

            if(dest_fb->swap)
            {
                coord.z = coord.x;
                coord.x = coord.y;
                coord.y = coord.z;
                coord.z = 1;
            }

            if(coord.x == ans[j].max_x && coord.y == ans[j].max_y)
                printf("ok   -> %d : %d %d\n",j, coord.x,coord.y);
            else
                printf("fail -> %d : %d %d (ans : %d %d)\n",j, coord.x,coord.y,ans[j].max_x,ans[j].max_y);
        }
    }
}
#endif

tm_config_t* tm_mapping_get_config()
{
    return tm_handler.cal;
}

tm_fb_param_t* tm_mapping_get_fb_param()
{
    return tm_handler.fb_param;
}

#else



const char* tm_errorno_str(qerrno no)
{
    switch(no)
    {
        case eENoErr:       return "No error";
        case eENoDev:       return "No such device";
        case eEDevP:        return "Parameter of device error";
        case eEDevU:        return "Device isn't used";
        case eEDevN:        return "Bad device number";
        case eEAlloc:       return "allocate error";
        case eEOpen:        return "Open file error";
        case eEPoint:       return "Points are out of rage";
        case eEParam:       return "Parameter error";
        case eESwap:        return "Need to swap x,y";
        default:            break;
    }
    return "unknown";
}




qerrno tm_update_conf(struct sTmData* tm)
{
    struct sTmDevParam* dev;
    char* conf_file;
    char* param;
    char buf[BUF_SIZE], alloc=0;
    int id;
    char is_fb;

    for(id = 0; id < eTmDevNum; id++)
    {
        tm->fb[id].used = -1;
        tm->panel[id].used = -1;
    }

    if ( (conf_file = getenv("QSI_TM_CONF")) == NULL )
    {
        if((conf_file = strdup(QSI_TM_CONF)) == NULL)
            return eEAlloc;

        alloc = 1;
    }

    printf("configure file: %s\n", conf_file);

    tm->fr = fopen(conf_file, "r");

    if(tm->fr == NULL)
    {
        perror("");
        return eEOpen;
    }

    while( !feof(tm->fr))
    {
        memset(buf, 0, BUF_SIZE);

        if(fgets(buf, BUF_SIZE, tm->fr) == NULL)
            continue;

        if(buf[0] == '#' || buf[0] == 0 || buf[BUF_SIZE - 2] != 0)
            continue;

        if((param = strtok(buf," ")) == NULL || (strlen(param) != 1))
            continue;

        if(param[0] == 'f')
            is_fb = 1;
        else if (param[0] == 'p')
            is_fb = 0;
        else
            continue;

        if((param = strtok(NULL," ")) == NULL || (strlen(param) != 1))
            continue;

        id = param[0] - '0';

        if(id < 0 || id >= eTmDevNum)
            continue;

        if(is_fb)
            dev = &tm->fb[id];
        else
            dev = &tm->panel[id];

        dev->used = id;

        if((param = strtok(NULL," ")) != NULL)
        {
            dev->min_x = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->max_x = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->min_y = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->max_y = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->horizontal = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->vertical = (int16_t)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->swap = (int16_t)atoi(param);
#if 1
            printf("set %5s %d : x %4d ~ %4d, y %4d ~ %4d, h %d, v %d, s %d\n"
                    ,(is_fb) ? "fb" : "panel"
                    ,id
                    ,dev->min_x
                    ,dev->max_x
                    ,dev->min_y
                    ,dev->max_y
                    ,dev->horizontal
                    ,dev->vertical
                    ,dev->swap);
#endif
        }
        else
        {
            dev->used = -1;
        }
    }

    if(alloc)
        q_free(conf_file);

    fclose(tm->fr);

    return eENoErr;
}

qerrno tm_create(struct sTmData** p_tm)
{
    struct sTmData* tm;

    if(!( *p_tm=(struct sTmData*)q_calloc(sizeof(struct sTmData)) ))
    {
       return eEAlloc;
    }

    tm = *p_tm;

    tm_update_conf(tm);

    tm->mutex = q_mutex_new(q_true, q_true);

    tm->queue = q_create_queue(MAX_QUEUE);

    return eENoErr;
}

void tm_destroy(struct sTmData* tm)
{
    q_assert(tm);

    if(tm->mutex)
        q_mutex_free(tm->mutex);

    q_destroy_queue(tm->queue);

    q_free(tm);
    tm=NULL;
}


int16_t tm_calculate_permille(int16_t val, int16_t min, int16_t max, char reverse)
{
    int permille;

   // printf("min %d max %d val %d : %d %d %d\n",min,max,val,val < min, val > max, max <= min);

    if(val < min || val > max || max <= min)
        return -1;

    permille = (((int)val - min) * MULTIPLE) / (max -min);

    if(reverse)
        permille = MULTIPLE - permille;

    return (int16_t)permille;
}

int16_t tm_calculate_output(int16_t permille, int16_t min, int16_t max)
{
    int16_t output;

    //printf("min %d max %d per %d : %d %d\n",min,max,permille,permille <= 0, max <= min);

    if(permille < 0 || max <= min)
        return -1;

    output = min + (int16_t)((permille * ((int)max - min)) / MULTIPLE);

    if(output > max)
        return -1;

    return output;
};

qerrno __tm_transfer(int16_t* x, int16_t* y, struct sTmDevParam* src, struct sTmDevParam* dest)
{
    int16_t per, out_x, out_y;

    per = tm_calculate_permille(*x, src->min_x, src->max_x, src->horizontal != dest->horizontal);
    out_x = tm_calculate_output(per, dest->min_x, dest->max_x);
    per = tm_calculate_permille(*y, src->min_y, src->max_y, src->vertical != dest->vertical);
    out_y = tm_calculate_output(per, dest->min_y, dest->max_y);

    if(out_x == -1 || out_y == -1)
        return eEPoint;

    if(src->swap != dest->swap)
    {
        *x = out_y;
        *y = out_x;
    }
    else
    {
        *x = out_x;
        *y = out_y;
    }

    return eENoErr;
}

qerrno tm_transfer(int16_t* x, int16_t* y, struct sTmData* tm, unsigned char panel, unsigned char fb)
{
    if (panel > eTmDevNum || fb > eTmDevNum)
        return eEDevN;

    if(tm->panel[panel].used == -1 || tm->fb[fb].used == -1)
        return eEDevU;

    return __tm_transfer(x, y, &tm->panel[panel], &tm->fb[fb]);
}

qerrno tm_transfer_value(int16_t* val, tm_event_code code, struct sTmDevParam* src, struct sTmDevParam* dest)
{
    int16_t per, out_x, out_y;

    if(code == eTmEventX)
    {
        per = tm_calculate_permille(*val, src->min_x, src->max_x, src->horizontal != dest->horizontal);
        if((out_x = tm_calculate_output(per, dest->min_x, dest->max_x)) == -1)
            return eEPoint;
    }
    else if(code == eTmEventY)
    {
        per = tm_calculate_permille(*val, src->min_y, src->max_y, src->vertical != dest->vertical);
        if((out_y = tm_calculate_output(per, dest->min_y, dest->max_y)) == -1)
            return eEPoint;;
    }
    else
        return eEParam;

    if(src->swap != dest->swap)
    {
        return eESwap;
    }

    return eENoErr;
}
#endif
