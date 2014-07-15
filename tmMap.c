/*
 * tmMap.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */
#include <string.h>
#include <stdlib.h>

#include "qUtils.h"
#include "tm.h"
#include "tmMap.h"
#include "tmInput.h"

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
