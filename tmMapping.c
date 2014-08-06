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

typedef struct _tm_handler
{
    q_mutex*        mutex;
    tm_config_t     conf;
	list_head_t		calibrate_head;
	list_head_t     native_size_head;
}tm_handler_t;

static tm_handler_t tm_handler;

void tm_mapping_pnl_bind_conf(tm_panel_info_t* panel);
void tm_mapping_ap_bind_conf(tm_ap_info_t* ap);
void tm_mapping_point(tm_display_t* dis, int src_x, int src_y, int* dest_x, int* dest_y);

void tm_mapping_print_conf(list_head_t* ap_head, list_head_t* pnl_head)
{
    tm_calibrate_t* cal=NULL;
    tm_native_size_param_t* size = NULL;
    tm_ap_info_t* ap;
    tm_panel_info_t* panel;
    tm_display_t* dis = NULL;

    list_for_each_entry(&tm_handler.calibrate_head, cal, node)
    {
        fprintf(stderr,"cal   id : %d, %d %d %d %d %d %d %d\n",
                cal->id,
                cal->trans_matrix.element[0][0],
                cal->trans_matrix.element[0][1],
                cal->trans_matrix.element[0][2],
                cal->trans_matrix.element[1][0],
                cal->trans_matrix.element[1][1],
                cal->trans_matrix.element[1][2],
                cal->scaling);
    }
    list_for_each_entry(&tm_handler.native_size_head, size, node)
    {
        fprintf(stderr,"size  id : %d, %d %d\n",
                size->id,
                size->max_x,
                size->max_y);
    }
    list_for_each_entry(ap_head, ap, node)
    {
        fprintf(stderr,"ap    id : %d, %s, bind : size id %d\n",
                ap->id,
                ap->evt_path,
                ap->native_size->id);
    }
    list_for_each_entry(pnl_head, panel, node)
    {
        fprintf(stderr,"panel id : %d, %s, bind : size id %d, cal id : %d\n",
                panel->id,
                panel->evt_path,
                panel->native_size->id,
                panel->cal_param->id);

        list_for_each_entry(&panel->display_head, dis, node)
        {
            fprintf(stderr,"           display ap %d, (%d~%d, %d~%d) -> (%d~%d, %d~%d)\n",
            		dis->ap->id,
            		dis->from.abs_st_x,
            		dis->from.abs_end_x,
            		dis->from.abs_st_y,
            		dis->from.abs_end_y,
            		dis->to.abs_st_x,
            		dis->to.abs_end_x,
            		dis->to.abs_st_y,
            		dis->to.abs_end_y);
        }
    }
}

tm_errno_t tm_mapping_update_conf(list_head_t* ap_head, list_head_t* pnl_head)
{
    FILE *fr;
    char buf[BUF_SIZE];
    char *conf_file = NULL;
    char *default_conf = QSI_TM_CONF;
    char *param;

    if ( (conf_file = getenv("QSI_TM_CONF")) == NULL )
        conf_file = default_conf;

    q_dbg("configure file is %s", conf_file);

    fr = fopen(conf_file, "r");

    if(fr == NULL)
    {
        qerror("open configuration");
        return TM_ERRNO_NO_DEV;
    }

    tm_mapping_remove_conf();

    while( !feof(fr) )
    {
        memset(buf, 0, BUF_SIZE);
        if(fgets(buf, BUF_SIZE, fr) == NULL)
            continue;

        if(buf[0] == '#' || buf[0] == 0 || buf[BUF_SIZE - 2] != 0)
            continue;

        if((param = strtok(buf," ")) == NULL || (strlen(param) != 1))
            continue;

        switch(param[0])
        {
        	case 'c': tm_mapping_calibrate_conf();		break;
        	case 's': tm_mapping_native_size_conf();	break;
        	case 'a': tm_mapping_ap_conf(ap_head);		break;
        	case 'p': tm_mapping_pnl_conf(pnl_head);	break;
        	default:break;
        }
    }

    fclose(fr);
    return TM_ERRNO_SUCCESS;
}

void tm_mapping_remove_conf()
{
	tm_calibrate_t* cal ;
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
    	q_free(native_size);
    }
}

void tm_mapping_calibrate_conf()
{
    int id, n, row, col;
    char *param;
    int matrix_num = CAL_MATRIX_ROW * CAL_MATRIX_COL;
    tm_trans_matrix_t matrix;
    tm_calibrate_t* cal;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return;

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
            return;

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
}

void tm_mapping_native_size_conf()
{
    int id;
    tm_native_size_param_t* native_size;
    char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return;

    native_size = (tm_native_size_param_t*)q_calloc(sizeof(tm_native_size_param_t));

    if(native_size == NULL)
        return;

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

    return;

err:
    q_free(native_size);
}

void tm_mapping_pnl_conf(list_head_t* pnl_head)
{
	int id;
	tm_panel_info_t* panel;
	char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return;

    panel = (tm_panel_info_t*)q_calloc(sizeof(tm_panel_info_t));

    if(panel == NULL)
        return;

    panel->id = id;

    if((param = strtok(NULL," ")) == NULL)
        goto err;

    panel->evt_path = q_strdup((const char*)param);
    panel->mutex = q_mutex_new(q_true, q_true);

    tm_mapping_pnl_bind_conf(panel);

    q_init_head(&panel->display_head);
    q_list_add(pnl_head, &panel->node);

    return;

err:
	q_free(panel);
}

void tm_mapping_ap_conf(list_head_t* ap_head)
{
	int id;
	tm_ap_info_t* ap;
	char *param;

    if(((param = strtok(NULL," ")) == NULL) || ((id = atoi(param)) < 0) )
        return;

    ap = (tm_ap_info_t*)q_calloc(sizeof(tm_ap_info_t));

    if(ap == NULL)
        return;

    ap->id = id;

    if((param = strtok(NULL," ")) == NULL)
        goto err;

    ap->evt_path = q_strdup((const char*)param);
    ap->mutex = q_mutex_new(q_true, q_true);

    tm_mapping_ap_bind_conf(ap);

    q_list_add(ap_head, &ap->node);

    return;

err:
	q_free(ap);
}

void tm_mapping_pnl_bind_conf(tm_panel_info_t* panel)
{
	char *param;
	panel->cal_param = NULL;
	panel->native_size = NULL;

	while(panel->cal_param == NULL || panel->native_size == NULL)
	{
		if((param = strtok(NULL," ")) == NULL)
			break;

        switch(param[0])
        {
        	case 'c':
        		if((param = strtok(NULL," ")) == NULL)
        			return;

        		panel->cal_param = tm_mapping_get_calibrate_param(atoi(param));
        		break;

        	case 's':
        		if((param = strtok(NULL," ")) == NULL)
        			return;

        		panel->native_size = tm_mapping_get_native_size_param(atoi(param));
        		break;

        	default:
        		break;
        }
	}
}


void tm_mapping_ap_bind_conf(tm_ap_info_t* ap)
{
	char *param;
	ap->native_size = NULL;

	while(ap->native_size == NULL)
	{
		if((param = strtok(NULL," ")) == NULL)
			break;

        if(param[0] == 's')
        {
    		if((param = strtok(NULL," ")) == NULL)
    			return;

    		ap->native_size = tm_mapping_get_native_size_param(atoi(param));
        }
	}
}

tm_errno_t  tm_mapping_create_handler(list_head_t* ap_head, list_head_t* pnl_head)
{
	q_assert(ap_head);
	q_assert(pnl_head);

    tm_handler.mutex = q_mutex_new(q_true, q_true);

    q_init_head(&tm_handler.calibrate_head);
    q_init_head(&tm_handler.native_size_head);

    if(tm_mapping_update_conf(ap_head, pnl_head) != TM_ERRNO_SUCCESS)
	{
		if(tm_handler.mutex)
			q_mutex_free(tm_handler.mutex);
        return TM_ERRNO_NO_CONF;
	}
		
    return TM_ERRNO_SUCCESS;
}

void tm_mapping_destroy_handler()
{
    tm_mapping_remove_conf();
	if(tm_handler.mutex)
		q_mutex_free(tm_handler.mutex);
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

#if 0
    printf("matrix_mult:\n");
    printf("|%5d %5d %5d| |%5d|   |%5d|\n",matrix->element[0][0],matrix->element[0][1],matrix->element[0][2],vector[0],result_vec[0]);
    printf("|%5d %5d %5d| |%5d| = |%5d|\n",matrix->element[1][0],matrix->element[1][1],matrix->element[1][2],vector[1],result_vec[1]);
    printf("|    0     0     0| |%5d|   |    0|\n",vector[2]);
#endif

    vector[0] = result_vec[0];
    vector[1] = result_vec[1];
}

tm_ap_info_t* tm_mapping_transfer(int *x, int *y, tm_panel_info_t* panel)
{
    tm_calibrate_t* cal = panel->cal_param;
    tm_display_t* dis;
    union {
         int vec[3];
         struct{
             int x;
             int y;
             int z;
         };
     }coord;

     q_assert(panel);

    coord.x = *x;
    coord.y = *y;
    coord.z = 1;

    // raw touch point -> frame buffer point
    tm_mapping_matrix_mult(&cal->trans_matrix, coord.vec);

    coord.x /= cal->scaling;
    coord.y /= cal->scaling;

#if 1 //de-jitter boundary
#define dejitter_boundary(pos, max, delta) (pos < 0 && pos > -(delta)) ? 0 : (pos > max && pos < max + delta) ? max : pos
	coord.x = dejitter_boundary(coord.x, panel->native_size->max_x, JITTER_BOUNDARY);
	coord.y = dejitter_boundary(coord.y, panel->native_size->max_y, JITTER_BOUNDARY);
#endif

    if((dis = tm_match_display(coord.x, coord.y, panel)) == NULL)
        return NULL;

    tm_mapping_point(dis, coord.x, coord.y, x, y);

    return dis->ap;
}

void tm_mapping_point(tm_display_t* dis, int src_x, int src_y, int* dest_x, int* dest_y)
{  
    if(dis->to.swap)
    {
        src_x ^= src_y;
        src_y ^= src_x;
        src_x ^= src_y;
    }
    
    if(dis->to.horizontal != dis->from.horizontal)
        src_x = dis->to.abs_end_x - src_x;
    
    if(dis->to.vertical != dis->from.vertical)
        src_y = dis->to.abs_end_y - src_y;
    
    *dest_x = dis->from.abs_st_x + ((src_x - dis->to.abs_st_x) * FB_LEN_X(dis->from)) / FB_LEN_X(dis->to);
    *dest_y = dis->from.abs_st_y + ((src_y - dis->to.abs_st_y) * FB_LEN_Y(dis->from)) / FB_LEN_Y(dis->to);

    if(dis->from.swap)
    {
        *dest_x ^= *dest_y;
        *dest_y ^= *dest_x;
        *dest_x ^= *dest_y;
    }
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
