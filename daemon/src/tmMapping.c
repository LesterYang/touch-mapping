/*
 *  tmMapping.c
 *  Copyright © 2014  
 *  All rights reserved.
 *  
 *       Author : Lester Yang <sab7412@yahoo.com.tw>
 *  Description : Mapping algorithm
 */
 
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "tm.h"

typedef union {
    int vec[3];
    struct{
        int x;
        int y;
        int z;
    };
}tm_coord_t;

typedef struct _tm_handler
{
    q_mutex*        mutex;
    char            cfg_path[TM_PATH_LEN];
    tm_config_t     conf;
    list_head_t	    calibrate_head;
    list_head_t     native_size_head;
}tm_handler_t;

static tm_handler_t tm_handler;

tm_errno_t tm_mapping_get_cfg_path();
tm_errno_t tm_mapping_update_config(list_head_t* ap_head, list_head_t* pnl_head);
void       tm_mapping_remove_config(list_head_t* ap_head, list_head_t* pnl_head);
tm_errno_t tm_mapping_calibrate_config(void);
tm_errno_t tm_mapping_native_size_config(void);
tm_errno_t tm_mapping_fb_config(void);
tm_errno_t tm_mapping_pnl_config(list_head_t* pnl_head);
tm_errno_t tm_mapping_ap_config(list_head_t* ap_head);
tm_errno_t tm_mapping_duplicate_config(void);
tm_errno_t tm_mapping_pnl_bind_config(tm_panel_info_t* panel);
tm_errno_t tm_mapping_ap_bind_config(tm_ap_info_t* ap);

void          tm_mapping_matrix_mult(tm_trans_matrix_t *matrix, int* vector);
void          tm_mapping_point(tm_display_t* dis, int src_x, int src_y, int* dest_x, int* dest_y);
tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel);

tm_calibrate_t*         tm_mapping_get_calibrate_param(int id);
tm_native_size_param_t* tm_mapping_get_native_size_param(int id);


tm_errno_t  tm_mapping_create_handler(list_head_t* ap_head, list_head_t* pnl_head)
{
    q_assert(ap_head);
    q_assert(pnl_head);

    tm_errno_t err;

    tm_handler.mutex = q_mutex_new(true, true);

    q_init_head(&tm_handler.calibrate_head);
    q_init_head(&tm_handler.native_size_head);

    if((err=tm_mapping_update_config(ap_head, pnl_head)) != TM_ERRNO_SUCCESS)
    {
        tm_mapping_destroy_handler(ap_head, pnl_head);
        return err;
    }
		
    return TM_ERRNO_SUCCESS;
}

void tm_mapping_destroy_handler(list_head_t* ap_head, list_head_t* pnl_head)
{
    tm_mapping_remove_config(ap_head, pnl_head);
    if(tm_handler.mutex)
        q_mutex_free(tm_handler.mutex);
}

tm_errno_t tm_mapping_get_cfg_path()
{
    char proc[32]={0};
    char path[TM_PATH_LEN]={0};
    char *p;
    
    sprintf(proc, "/proc/%d/exe", getpid());

    if(readlink(proc, path, TM_PATH_LEN)<0)
        return TM_ERRNO_OTHER;

    //get lst_tm version folder
    if(!(p=strrchr(path,'/')))
        return TM_ERRNO_OTHER;
    *p=0;

    // get lst_tm folder
    if(!(p=strrchr(path,'/')))
        return TM_ERRNO_OTHER;
    *p=0;

    sprintf(tm_handler.cfg_path, "%s/lst_tm.conf", path);

    if(access(tm_handler.cfg_path, R_OK) != 0)
    {
        q_dbg(Q_INFO,"no %s or can't read this file", tm_handler.cfg_path);
        return TM_ERRNO_NO_CONF;
    }
    
    return TM_ERRNO_SUCCESS;
}

tm_errno_t tm_mapping_update_config(list_head_t* ap_head, list_head_t* pnl_head)
{
    FILE *fr;
    char buf[BUF_SIZE];
    char *conf_file = NULL;
    char *default_conf = LST_TM_CFG;
    char *param, *p;
    tm_errno_t err;

    if( (conf_file = getenv("LST_TM_CFG")) == NULL )
    {
        if((tm_mapping_get_cfg_path() == TM_ERRNO_SUCCESS))
            conf_file = tm_handler.cfg_path;
        else        
            conf_file = default_conf;
    }
    
    q_dbg(Q_INFO, "configure file is %s", conf_file);

    fr = fopen(conf_file, "r");

    if(fr == NULL)
    {
        q_dbg(Q_ERR,"open configuration");
        return TM_ERRNO_NO_DEV;
    }

    tm_mapping_remove_config(ap_head, pnl_head);

    while( !feof(fr) )
    {
        memset(buf, 0, BUF_SIZE);
        if(fgets(buf, BUF_SIZE, fr) == NULL)
            continue;

        if(buf[0] == '#' || buf[0] == 0 || buf[BUF_SIZE - 2] != 0)
            continue;

        if((param = strtok(buf," ")) == NULL)
            continue;

        if((p=strchr(buf, '#')))
            p[0]=0;

        if(memcmp(param, CAL_CFG, sizeof(CAL_CFG)) == 0)
        {
            if((err = tm_mapping_calibrate_config()) != TM_ERRNO_SUCCESS)
                return err;
        }
        else if(memcmp(param, FB_CFG, sizeof(FB_CFG)) == 0)
        {
            if((err = tm_mapping_fb_config()) != TM_ERRNO_SUCCESS)
                return err;
        }
        else if(memcmp(param, AP_CFG, sizeof(AP_CFG)) == 0)
        {
            if((err = tm_mapping_ap_config(ap_head)) != TM_ERRNO_SUCCESS)
                return err;
        }
        else if(memcmp(param, PNL_CFG, sizeof(PNL_CFG)) == 0)
        {
            if((err = tm_mapping_pnl_config(pnl_head)) != TM_ERRNO_SUCCESS)
                return err;
        }
        else if(memcmp(param, COPY_CFG, sizeof(COPY_CFG)) == 0)
        {
            if((err = tm_mapping_duplicate_config()) != TM_ERRNO_SUCCESS)
                return err;
        }
    }

    fclose(fr);
    return TM_ERRNO_SUCCESS;
}

void tm_mapping_remove_config(list_head_t* ap_head, list_head_t* pnl_head)
{
    tm_ap_info_t *ap;
    tm_panel_info_t *panel;
    tm_calibrate_t* cal;
    tm_native_size_param_t* native_size;

    tm_handler.conf.calibrate_num = 0;
    tm_handler.conf.native_size_num = 0;

    while((cal = list_first_entry(&tm_handler.calibrate_head, tm_calibrate_t, node)) != NULL)
    {
    	q_list_del(&cal->node);
    	q_free(cal);
    }
        
    while((native_size = list_first_entry(&tm_handler.native_size_head, tm_native_size_param_t, node)) != NULL)
    {
    	q_list_del(&native_size->node);
    	q_free((char*)native_size->fb_path);
    	q_free(native_size);
    }

    while((ap = list_first_entry(ap_head, tm_ap_info_t, node)) != NULL)
    {
        q_list_del(&ap->node);
        q_free((char*)ap->evt_path);
        if(ap->mutex) q_mutex_free(ap->mutex);
        q_free(ap);
    }

    while((panel = list_first_entry(pnl_head, tm_panel_info_t, node)) != NULL)
    {
        q_list_del(&panel->node);
        q_free((char*)panel->evt_path);
        q_free((char*)panel->duplicate_evt_path);
        if(panel->mutex) q_mutex_free(panel->mutex);
            q_free(panel);
    }
}

tm_errno_t tm_mapping_calibrate_config()
{
    int id, n, row, col;
    char *param;
    int matrix_num = CAL_MATRIX_ROW * CAL_MATRIX_COL;
    tm_trans_matrix_t matrix;
    tm_calibrate_t* cal;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;

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
        cal = (tm_calibrate_t*)q_calloc(sizeof(tm_calibrate_t));

        if(cal == NULL)
            return TM_ERRNO_ALLOC;

        cal->id = id;
        cal->pressure.div = 1;
        cal->pressure.mult = 1;
        cal->scaling = atoi(param);
        memcpy(&cal->trans_matrix, &matrix, sizeof(tm_trans_matrix_t));

        q_mutex_lock(tm_handler.mutex);

        q_list_add(&tm_handler.calibrate_head, &cal->node);
        tm_handler.conf.calibrate_num++;

        q_mutex_unlock(tm_handler.mutex);
    }
    return TM_ERRNO_SUCCESS;
}

tm_errno_t tm_mapping_native_size_config()
{
    int id;
    tm_native_size_param_t* native_size;
    char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;

    native_size = (tm_native_size_param_t*)q_calloc(sizeof(tm_native_size_param_t));

    if(native_size == NULL)
        return TM_ERRNO_ALLOC;

    native_size->id = id;

    if((param = strtok(NULL," ")) == NULL)
        goto err;

    native_size->max_x = atoi(param);

    if((param = strtok(NULL," ")) == NULL)
         goto err;

    native_size->max_y = atoi(param);

    q_mutex_lock(tm_handler.mutex);

    q_list_add(&tm_handler.native_size_head, &native_size->node);
    tm_handler.conf.native_size_num++;

    q_mutex_unlock(tm_handler.mutex);

    return TM_ERRNO_SUCCESS;

err:
    q_free(native_size);
    return TM_ERRNO_PARAM;
}

tm_errno_t tm_mapping_fb_config()
{
    int id;
    struct fb_var_screeninfo fb_var;
    tm_native_size_param_t* native_size;
    char *param;
    char path[64]={0};

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;

    native_size = (tm_native_size_param_t*)q_calloc(sizeof(tm_native_size_param_t));

    if(native_size == NULL)
        return TM_ERRNO_ALLOC;

    native_size->id = id;

    if((param = strtok(NULL," ")) == NULL)
        goto err;

    memcpy(path, param, strlen(param)-1);

    native_size->fb_path = q_strdup((const char*)param);

    if(memcmp(native_size->fb_path, FB_CFG_AUTO_DEV, sizeof(FB_CFG_AUTO_DEV)-1) == 0)
    {
        native_size->max_x = FB_CFG_DEFAULT_X;
        native_size->max_y = FB_CFG_DEFAULT_Y; 
    }
    else
    {
        int fd;
        if((fd = open(native_size->fb_path, O_RDWR)) == -1)
        {
            q_dbg(Q_ERR,"open %s error", native_size->fb_path);
            goto err_param;
        }
     
        if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0)
        {
            q_dbg(Q_ERR,"get fb size error");
            q_close(fd);
            goto err_param;
        }
        
        native_size->max_x = fb_var.xres;
        native_size->max_y = fb_var.yres;

        q_close(fd);
    }
    
    q_mutex_lock(tm_handler.mutex);

    q_list_add(&tm_handler.native_size_head, &native_size->node);
    tm_handler.conf.native_size_num++;

    q_mutex_unlock(tm_handler.mutex);

    return TM_ERRNO_SUCCESS;

err_param:
    q_free((char*)native_size->fb_path);
err:
    q_free(native_size);
    return TM_ERRNO_PARAM;
   
}

tm_errno_t tm_mapping_pnl_config(list_head_t* pnl_head)
{
    int id;
    tm_panel_info_t* panel;
    char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;

    panel = (tm_panel_info_t*)q_calloc(sizeof(tm_panel_info_t));

    if(panel == NULL)
        return TM_ERRNO_ALLOC;

    panel->id = id;

    if((param = strtok(NULL," ")) == NULL)
        goto err;

    panel->evt_path = q_strdup((const char*)param);
    panel->mutex = q_mutex_new(true, true);
    panel->duplicate = false;


    if(tm_mapping_pnl_bind_config(panel) == TM_ERRNO_NO_DEV)
        goto err_bind;
    
    q_init_head(&panel->display_head);
    q_list_add(pnl_head, &panel->node);

    return TM_ERRNO_SUCCESS;

err_bind:
    q_free((char*)panel->evt_path);
    q_mutex_free(panel->mutex);
err:
    q_free(panel);
    return TM_ERRNO_NO_CONF;
}

tm_errno_t tm_mapping_ap_config(list_head_t* ap_head)
{
    int id, fd;
    tm_errno_t err = TM_ERRNO_SUCCESS;
    tm_ap_info_t* ap;
    char *param;
    long absbits[NUM_LONGS(ABS_CNT)]={0};
    
    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;

    ap = (tm_ap_info_t*)q_calloc(sizeof(tm_ap_info_t));

    if(ap == NULL)
        return TM_ERRNO_ALLOC;

    ap->id = id;

    if((param = strtok(NULL," ")) == NULL)
    {
        err = TM_ERRNO_NO_CONF;
        goto err;
    }
    
    ap->evt_path = q_strdup((const char*)param);

    if((fd=open(ap->evt_path, O_RDWR))<0)
    {
        q_dbg(Q_ERR,"open %s error", ap->evt_path);
        err = TM_ERRNO_NO_FD;
        goto err_param; 
    }

    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0)
        ap->touch_type = testBit(ABS_MT_POSITION_X, absbits)?TM_INPUT_TYPE_MT_B:TM_INPUT_TYPE_SINGLE;
    else
    {
        err = TM_ERRNO_IOCTL;
        goto err_fd;
    }
    
    q_close(fd);

    ap->threshold = true;
    ap->mutex = q_mutex_new(true, true);

    if(tm_mapping_ap_bind_config(ap) == TM_ERRNO_NO_DEV)
    {
        err = TM_ERRNO_NO_CONF;
        goto err_bind;
    }
    
    q_list_add(ap_head, &ap->node);

    return TM_ERRNO_SUCCESS;


err_bind:
    q_mutex_free(ap->mutex);
err_fd:
    q_close(fd);
err_param:
    q_free((char*)ap->evt_path);
err:
    q_free(ap);
    return err;
}

tm_errno_t tm_mapping_duplicate_config()
{
    int id;
    tm_panel_info_t* panel;
    char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return TM_ERRNO_PARAM;
    
    panel = tm_get_panel_info(id);

    if(panel == NULL)
        return TM_ERRNO_NO_DEV;

    if((param = strtok(NULL," ")) == NULL)
        return TM_ERRNO_PARAM;
    
    panel->duplicate_evt_path = q_strdup((const char*)param);
    panel->duplicate = true;
   
    return TM_ERRNO_SUCCESS;
}


tm_errno_t tm_mapping_pnl_bind_config(tm_panel_info_t* panel)
{
    char *param;
    panel->cal_param = NULL;
    panel->native_size = NULL;

    while((param = strtok(NULL," ")) != NULL)
    {
        if(memcmp(param, CAL_CFG, sizeof(CAL_CFG)) == 0)
        {
            if((param = strtok(NULL," ")) == NULL)
                return TM_ERRNO_PARAM;
            panel->cal_param = tm_mapping_get_calibrate_param(atoi(param));
        }
        else if(memcmp(param, FB_CFG, sizeof(FB_CFG)) == 0)
        {
            if((param = strtok(NULL," ")) == NULL)
                return TM_ERRNO_PARAM;
            panel->native_size = tm_mapping_get_native_size_param(atoi(param));
            
            if(panel->native_size == NULL)
                return TM_ERRNO_NO_DEV;
        }
        else if(memcmp(param, AP_CFG, sizeof(AP_CFG)) == 0)
        {
            if((param = strtok(NULL," ")) == NULL)
                return TM_ERRNO_PARAM;

            tm_display_t* dis =  (tm_display_t*)q_calloc(sizeof(tm_display_t));
            
            if(dis == NULL)
                return TM_ERRNO_ALLOC;
            
            dis->ap = tm_get_ap_info(atoi(param));

            if(dis->ap == NULL)
            {
                q_free(dis);
                return TM_ERRNO_NO_DEV;
            }

            dis->from_ap.begin_x = 0;
            dis->from_ap.begin_x = 0;
            dis->from_ap.w = 100;
            dis->from_ap.h = 100;
            dis->to_pnl.begin_x = 0;
            dis->to_pnl.begin_x = 0;
            dis->to_pnl.w = 100;
            dis->to_pnl.h = 100;

            tm_fillup_fb_param(&dis->from_ap, dis->ap->native_size);
            tm_fillup_fb_param(&dis->to_pnl, panel->native_size);

            q_list_add(&panel->display_head, &dis->node);
        }
    }
    return TM_ERRNO_SUCCESS;
}


tm_errno_t tm_mapping_ap_bind_config(tm_ap_info_t* ap)
{
    char *param;
    ap->native_size = NULL;

    while(ap->native_size == NULL)
    {
        if((param = strtok(NULL," ")) == NULL)
            return TM_ERRNO_PARAM;

        if(memcmp(param, FB_CFG, sizeof(FB_CFG)) == 0)
        {
            if((param = strtok(NULL," ")) == NULL)
                return TM_ERRNO_PARAM;

            ap->native_size = tm_mapping_get_native_size_param(atoi(param));

            if(ap->native_size == NULL)
                return TM_ERRNO_NO_DEV;
        }
    }
    return TM_ERRNO_SUCCESS;
}

void tm_mapping_matrix_mult(tm_trans_matrix_t *matrix, int* vector)
{
    int i, j;
    int reslut[2] = {0};

    for(i=0; i<CAL_MATRIX_ROW; i++)
    {
        for(j=0; j<CAL_MATRIX_COL; j++)
        {
            reslut[i] +=  matrix->element[i][j] * vector[j];
        }
    }

#if 0
    printf("matrix_mult:\n");
    printf("|%5d %5d %5d| |%5d|   |%5d|\n",matrix->element[0][0],matrix->element[0][1],matrix->element[0][2],vector[0],reslut[0]);
    printf("|%5d %5d %5d| |%5d| = |%5d|\n",matrix->element[1][0],matrix->element[1][1],matrix->element[1][2],vector[1],reslut[1]);
    printf("|    0     0     0| |%5d|   |    0|\n",vector[2]);
#endif

    vector[0] = reslut[0];
    vector[1] = reslut[1];
}

void tm_mapping_point(tm_display_t* dis, int src_x, int src_y, int* dest_x, int* dest_y)
{  
    /*                             [ap]      [panel]
        *   [framebuffer]   :    from         to
        *   [touch event]  :    dest         src
        */

    if(dis->to_pnl.swap)
    {
        src_x ^= src_y;
        src_y ^= src_x;
        src_x ^= src_y;
    }
    
    if(dis->to_pnl.horizontal != dis->from_ap.horizontal)
        src_x = dis->to_pnl.abs_end_x - src_x;
    
    if(dis->to_pnl.vertical != dis->from_ap.vertical)
        src_y = dis->to_pnl.abs_end_y - src_y;
    
    *dest_x = FB_MAP_X(src_x, dis);
    *dest_y = FB_MAP_Y(src_y, dis);

    if(dis->from_ap.swap)
    {
        *dest_x ^= *dest_y;
        *dest_y ^= *dest_x;
        *dest_x ^= *dest_y;
    }
}

tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    tm_calibrate_t* cal = panel->cal_param;
    tm_display_t* dis;
    tm_coord_t coord;

    q_assert(panel);
    
    coord.x = *x;
    coord.y = *y;
    coord.z = 1;
    q_dbg(Q_DBG_POINT," -> x y : %d, %d",coord.x, coord.y);
   
    // raw touch point -> frame buffer point
    tm_mapping_matrix_mult(&cal->trans_matrix, coord.vec);

    coord.x /= cal->scaling;
    coord.y /= cal->scaling;
    q_dbg(Q_DBG_POINT," <- x y : %d, %d",coord.x, coord.y);

    //de-jitter boundary
    coord.x = dejitter_boundary(coord.x, panel->native_size->max_x, JITTER_BOUNDARY);
    coord.y = dejitter_boundary(coord.y, panel->native_size->max_y, JITTER_BOUNDARY);

    if((dis = tm_match_display(coord.x, coord.y, panel)) == NULL)
    {
        return NULL;
    }

#if Q_DBG_MAP == Q_DBG_ENABLE
    int per_x,per_y;
    per_x = ((coord.x-dis->to_pnl.abs_begin_x)*100)/(dis->to_pnl.abs_end_x - dis->to_pnl.abs_begin_x);
    per_y = ((coord.y-dis->to_pnl.abs_begin_y)*100)/(dis->to_pnl.abs_end_y - dis->to_pnl.abs_begin_y);
    q_dbg(Q_DBG_MAP,"pnl : %d%% %d%% (%d,%d)",per_x,per_y, coord.x, coord.y);
#endif

    tm_mapping_point(dis, coord.x, coord.y, x, y);

#if Q_DBG_MAP == Q_DBG_ENABLE
    per_x = ((*x-dis->from_ap.abs_begin_x)*100)/(dis->from_ap.abs_end_x - dis->from_ap.abs_begin_x);
    per_y = ((*y-dis->from_ap.abs_begin_y)*100)/(dis->from_ap.abs_end_y - dis->from_ap.abs_begin_y);
    q_dbg(Q_DBG_MAP,"out : %d%% %d%% (%d,%d)",per_x,per_y, *x, *y);
#endif

    return dis->ap;
}

void tm_mapping_duplicate_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    tm_calibrate_t* cal = panel->cal_param;
    tm_coord_t coord;

    q_assert(panel);
    
    coord.x = *x;
    coord.y = *y;
    coord.z = 1;
   
    // raw touch point -> frame buffer point
    tm_mapping_matrix_mult(&cal->trans_matrix, coord.vec);

    coord.x /= cal->scaling;
    coord.y /= cal->scaling;

    //de-jitter boundary
    *x = dejitter_boundary(coord.x, panel->native_size->max_x, JITTER_BOUNDARY);
    *y = dejitter_boundary(coord.y, panel->native_size->max_y, JITTER_BOUNDARY);

    return;
}


tm_calibrate_t* tm_mapping_get_calibrate_param(int id)
{
    tm_calibrate_t* cal = NULL;

    list_for_each_entry(&tm_handler.calibrate_head, cal, node)
    {
    	 if(cal->id == id) break;
    }

    return cal;
}

tm_native_size_param_t* tm_mapping_get_native_size_param(int id)
{
    tm_native_size_param_t* size = NULL;

    list_for_each_entry(&tm_handler.native_size_head, size, node)
    {
        if(size->id == id) break;
    }

    return size;
}

bool tm_mapping_native_size_is_const(tm_native_size_param_t* size)
{
    return (memcmp(size->fb_path, FB_CFG_DEV, sizeof(FB_CFG_DEV)-1)) ? false : true;  
}

void tm_mapping_print_config(list_head_t* ap_head, list_head_t* pnl_head)
{
    tm_mapping_print_cal_info();
    tm_mapping_print_size_info();
    tm_mapping_print_ap_info(ap_head);
    tm_mapping_print_panel_info(pnl_head);
}


void tm_mapping_print_cal_info()
{
    tm_calibrate_t* cal=NULL;
    list_for_each_entry(&tm_handler.calibrate_head, cal, node)
    {
        q_dbg(Q_INFO,"  cal id %2d -> %d %d %d %d %d %d %d\n",
                cal->id,
                cal->trans_matrix.element[0][0],
                cal->trans_matrix.element[0][1],
                cal->trans_matrix.element[0][2],
                cal->trans_matrix.element[1][0],
                cal->trans_matrix.element[1][1],
                cal->trans_matrix.element[1][2],
                cal->scaling);
    }
}

void tm_mapping_print_size_info()
{
    tm_native_size_param_t* size=NULL;
    list_for_each_entry(&tm_handler.native_size_head, size, node)
    {
        q_dbg(Q_INFO," size id %2d -> %d %d\n",
                size->id,
                size->max_x,
                size->max_y);
    }
}

void tm_mapping_print_ap_info(list_head_t* ap_head)
{
    tm_ap_info_t* ap;
    list_for_each_entry(ap_head, ap, node)
    {
        q_dbg(Q_INFO,"   ap id %2d -> %s, bind : size id %d\n",
                ap->id,
                ap->evt_path,
                ap->native_size->id);
    }
}

void tm_mapping_print_panel_info(list_head_t* pnl_head)
{
    tm_panel_info_t* panel;
    tm_display_t* dis = NULL;
    list_for_each_entry(pnl_head, panel, node)
    {
        q_dbg(Q_INFO,"panel id %2d -> %s, bind : size id %d, cal id : %d\n",
                panel->id,
                panel->evt_path,
                panel->native_size->id,
                panel->cal_param->id);

        list_for_each_entry(&panel->display_head, dis, node)
        {
            q_dbg(Q_INFO,"%14s display ap %d, (%d~%d, %d~%d) -> (%d~%d, %d~%d)\n",
                    "",
            		dis->ap->id,
            		dis->from_ap.abs_begin_x,
            		dis->from_ap.abs_end_x,
            		dis->from_ap.abs_begin_y,
            		dis->from_ap.abs_end_y,
            		dis->to_pnl.abs_begin_x,
            		dis->to_pnl.abs_end_x,
            		dis->to_pnl.abs_begin_y,
            		dis->to_pnl.abs_end_y);
        }
    }
}


