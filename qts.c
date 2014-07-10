/*
 * qts.c
 *
 *  Created on: Jun 17, 2014
 *      Author: root
 */
#include <string.h>
#include <stdlib.h>

#include "qts.h"

const char* tsqsi_errorno_str(qerrno no)
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
        default:            break;
    }
    return "unknown";
}

qerrno tsqsi_create(struct tsqsi** p_ts)
{
    struct tsqsi* ts;
    struct tsqsi_dev* dev;
    char* conf_file;
    char* param;
    char buf[BUF_SIZE], alloc=0;
    int id;
    char is_fb;

    if(!( *p_ts=(struct tsqsi*)calloc(1, sizeof(struct tsqsi)) ))
    {
       return eEAlloc;
    }

    ts = *p_ts;

    for(id = 0; id < MAX_DEV_NUM; id++)
    {
        ts->fb[id].used = -1;
        ts->panel[id].used = -1;
    }

    if ( (conf_file = getenv("TSQSI_CONF")) == NULL )
    {
        if((conf_file = strdup(DEFAULT_TSQSI_CONF)) == NULL)
            return eEAlloc;

        alloc = 1;
    }

    printf("configure file: %s\n", conf_file);

    ts->fr = fopen(conf_file, "r");

    if(ts->fr == NULL)
    {
        perror("");
        return eEOpen;
    }

    while( !feof(ts->fr))
    {
        memset(buf, 0, BUF_SIZE);

        if(fgets(buf, BUF_SIZE, ts->fr) == NULL)
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

        if(id < 0 || id >= MAX_DEV_NUM)
            continue;

        if(is_fb)
            dev = &ts->fb[id];
        else
            dev = &ts->panel[id];

        if((param = strtok(NULL," ")) != NULL)
        {
            if((dev->name = strdup(param)) == NULL)
                continue;

            dev->used = id;
            dev->name_len = (unsigned char)strlen(dev->name);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->min_x = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->max_x = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->min_y = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->max_y = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->horizontal = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->vertical = (short)atoi(param);
        }

        if(param != NULL && (param = strtok(NULL," ")) != NULL)
        {
            dev->swap = (short)atoi(param);
#if 1
            printf("set %5s %d : %6s, x %4d ~ %4d, y %4d ~ %4d, h %d, v %d, s %d\n"
                    ,(is_fb) ? "fb" : "panel"
                    ,id
                    ,dev->name
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
            free(dev->name);
            dev->used = -1;

        }
    }

    if(alloc)
        free(conf_file);

    fclose(ts->fr);

    return eENoErr;
}

void tsqsi_destroy(struct tsqsi* ts)
{
    int i;

    if(ts)
    {
        for(i = 0; i < MAX_DEV_NUM; i++)
        {
            if(ts->fb[i].used != -1)
            {
                ts->fb[i].used = -1;
                free(ts->fb[i].name);
            }
            if(ts->panel[i].used != -1)
            {
                ts->panel[i].used = -1;
                free(ts->panel[i].name);
            }
        }
        free(ts);
    }
}


short tsqsi_calculate_permille(short val, short min, short max, char reverse)
{
    int permille;

   // printf("min %d max %d val %d : %d %d %d\n",min,max,val,val < min, val > max, max <= min);

    if(val < min || val > max || max <= min)
        return -1;

    permille = (((int)val - min) * MULTIPLE) / (max -min);

    if(reverse)
        permille = MULTIPLE - permille;

    return (short)permille;
}

short tsqsi_calculate_output(short permille, short min, short max)
{
    short output;

    //printf("min %d max %d per %d : %d %d\n",min,max,permille,permille <= 0, max <= min);

    if(permille < 0 || max <= min)
        return -1;

    output = min + (short)((permille * ((int)max - min)) / MULTIPLE);

    if(output > max)
        return -1;

    return output;
};

qerrno __tsqsi_transfer(short* x, short* y, struct tsqsi_dev* src, struct tsqsi_dev* dest)
{
    short per, out_x, out_y;

    per = tsqsi_calculate_permille(*x, src->min_x, src->max_x, src->horizontal != dest->horizontal);
    out_x = tsqsi_calculate_output(per, dest->min_x, dest->max_x);
    per = tsqsi_calculate_permille(*y, src->min_y, src->max_y, src->vertical != dest->vertical);
    out_y = tsqsi_calculate_output(per, dest->min_y, dest->max_y);

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

qerrno tsqsi_transfer(short* x, short* y, struct tsqsi* ts, unsigned char panel, unsigned char fb)
{
    if (panel > MAX_DEV_NUM || fb > MAX_DEV_NUM)
        return eEDevN;

    if(ts->panel[panel].used == -1 || ts->fb[fb].used == -1)
        return eEDevU;

    return __tsqsi_transfer(x, y, &ts->panel[panel], &ts->fb[fb]);
}
