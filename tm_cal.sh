#! /bin/bash

defualt_file="/etc/qst_mt.conf"

cal_tmp_file="/tmp/cal"
tmp_conf="/tmp/qst_mt.conf"
conf_file=""
last_file=""

ts_conf="/usr/etc/ts.conf"
ts_module="/usr/lib/ts"
ts_type="INPUT"

declare -a ts_dev
declare -a fb_dev
declare -a cal_id

declare -i max_num=0


usage()
{
    echo "Usage: $0 [option]" 1>&2
    echo "[option] :    " 1>&2
    echo "      undo    recover last configuration" 1>&2
    echo "      -h      help" 1>&2
}

set_conf_file()
{
    conf_file=`echo $QSI_TM_CONF`

    if [ -z "$conf_file" ];then
        conf_file=$defualt_file
        if [ -z "$conf_file" ];then
            echo "tm_cal : no qsi_tm.conf"
            exit 1
        fi
    fi

    last_file="${conf_file}_last"
}

check_tmp_file()
{
    if [ -e $cal_tmp_file ];then
        rm $cal_tmp_file
    fi

    if [ -e $tmp_conf ];then
        rm $tmp_conf
    fi
}

set_param()
{
    while IFS=' ' read -r -a line
    do
        if [ "${line[0]}" == "pnl_info" ]; then
            ts_dev[$max_num]=${line[2]}
            cal_id[$max_num]=${line[4]}
            fb_dev[$max_num]=${line[8]}
            max_num=$((max_num+1))
        fi
    done  < $conf_file
}


do_calibrate()
{
    echo "tm_cal : start to calibrate (count = $max_num)"

	export  TSLIB_CONFFILE="$ts_conf"
	export  TSLIB_PLUGINDIR="$ts_module"
	export  TSLIB_TSEVENTTYPE="$ts_type"

    for (( i=0 ; i<$max_num ; i++))
    do
        echo "tm_cal : do $i ..."
        export TSLIB_TSDEVICE="${ts_dev[$i]}"
        export TSLIB_FBDEVICE="${fb_dev[$i]}"
        export TSLIB_CALIBFILE="$cal_tmp_file$i"
        #

    done
}

save_conf()
{ 
    while read line
    do
        if [ "`echo "$line" | awk '{print $1}'`" == "cal_conf" ]
        then
            id="`echo "$line" | awk '{print $2}'`"
            for (( i=0 ; i<$max_num ; i++))
            do
                if [ "$id" == "${cal_id[$i]}" ]
                then
                    echo "cal_conf ${cal_id[$i]} `cat $cal_tmp_file$i`" >> $tmp_conf
                fi                    
            done
        else
            echo $line >> $tmp_conf 
        fi
    done  < $conf_file

    cp $conf_file $last_file && sync
    cp $tmp_conf $conf_file || echo "tm_cal : error. readonly ?"
    sync
}

clear_tmp_file()
{
    rm $tmp_conf
    for (( i=0 ; i<$max_num ; i++))
    do
        rm $cal_tmp_file$i            
    done
}

undo_conf()
{
    if [ -e "$last_file" ]; then
        cp $last_file $conf_file || echo "tm_cal : error. readonly ?"
        sync
    else
        echo "tm_cal : no $last_file"
    fi
}

########################################
# Main()
########################################


case "$1" in
    "undo")
        set_conf_file
        undo_conf
        ;;
    "-h")
        usage
        ;;
    *)
        check_tmp_file
        set_conf_file
        set_param
        do_calibrate
        save_conf
        clear_tmp_file
        ;;
esac

exit 0

