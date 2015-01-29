#!/bin/bash

###################
# search pattern
###################
ver_pattern="qsi_tm_*_v*"
proj_pattern="qsi_tm.conf*"

###################
# system parameter
###################
program_name="qsi_tm"
_func_="main()"
redirect="/dev/console"
arglist="$@"
dbg="0"

###################
# global variable
###################
location=""
output="/home/qsi_tm"
input=""
version=""
project=""
new=""

###################
# color code
###################
c_null="\e[0m"
c_Red="\e[31m"
c_Green="\e[32m"
c_Yellow="\e[33m"
c_Blue="\e[34m"
c_Purple="\e[35m"
c_Cyan="\e[36m"
c_LRed="\e[1;31m"
c_LGreen="\e[1;32m"
c_LYellow="\e[1;33m"
c_LBlue="\e[1;34m"
c_LPurple="\e[1;35m"
c_LCyan="\e[1;36m"

#################
# Function
#################
usage()
{
    echo " Usage: $0 OPTION [PARAMTERS]"                                        1>&2
    echo " OPTION"                                                              1>&2
    echo "     -p, --prefix             set target prefix"                      1>&2
    echo "     -s, --source             set source path"                        1>&2
    echo "                              [defaul : current working directory]"   1>&2
    echo "     -d, --debug              more information"                       1>&2
    echo "         --silence            print nothing "                         1>&2
    echo "     -h, --help               help"                                   1>&2
    exit 0
}

parse_options()
{
    local SHORT_OPTS="hds:p:"
    local LONG_OPTS="help,debug,prefix:,source:,silence"
    local opt=`getopt -l "$LONG_OPTS" -o "$SHORT_OPTS" -- "$@"`

    eval set -- "$opt"

    while true
    do
        case "$1" in
            -h|--help)     
                usage
                ;;
            -d|--debug)
                dbg="1"
                ;;
            --silence)
                dbg="0"
                redirect="/dev/null"
                ;;
            -s|--source)
                if [ -d "$2" ];then
                    location=$2
                else
                    log_notify "[line $LINENO] no $2 directory, using default path"
                fi
                shift
                ;;
            -p|--prefix)
                output=$2$output
                shift
                ;;
            --)     
                break
                ;;
            *)      
                log_exit "[line $LINENO] option error"
                ;;
        esac
        shift
    done
}

log_exit()
{
    local pattern="${c_LRed}-- ${program_name} -- : $1 ${c_null}"
    echo -e "$pattern" > $redirect
    echo -e "${c_LRed}-- ${program_name} -- : Exit ${c_null}" 1>&2
    exit 0
}

log_err()
{
    local pattern="${c_LRed}-- ${program_name} -- : $1 ${c_null}"
    echo -e "$pattern" > $redirect
}

log_notify()
{
    local pattern="-- ${program_name} -- : $1"
    local color="$2"
    
    if [ -n "$color" ];then
        pattern="${color}${pattern}${c_null}"
    fi
    
    echo -e "$pattern" > $redirect
}

log_info()
{
    if [ "$dbg" == "0" ];then
        return
    fi
    
    local pattern="-- ${program_name} -- : $1"
    local color="$2"
    
    if [ -n "$color" ];then
        pattern="${color}${pattern}${c_null}"
    fi
    
    echo -e "$pattern" > $redirect
}

set_ver_param()
{
    local cand="$1"

    if [ -z "$cand" ];then
        log_exit "[line $LINENO] no candidate version"
    fi

    for i in `seq 0 1 $ver_num`
    do
        if [ "$cand" == "${ver_candidate[$i]}" ];then
            input=${ver_path[$i]}
            break;
        fi
    done
    
    if [ -z "$input" ];then
        log_exit "[line $LINENO] no matched ($cand)"
    fi
}

choose_version()
{
    local res
    local -a ver_path
    local -a ver_candidate
    local -i ver_num
    local -i i

    res=`find $location -type d -name "$ver_pattern"`

    if [ -z "$res" ];then
        log_exit "[line $LINENO] no qsi_tm source"
    fi

    res=$(echo "$res" | sed 'a,' | tr -d '\n')
    ver_num=$((`echo "$res"|awk -F ',' '{print NF}'`-1))

    for i in `seq 0 1 $((ver_num-1))`
    do
        ver_path[$i]=`echo $res | awk -F ',' '{print $'$((i+1))' }'`
        ver_candidate[$i]=`echo "${ver_path[$i]}" | awk -F '/' '{print $NF}'`
    done
    ver_candidate[$((++i))]="quit"

    
    if [ "$ver_num" == "1" ];then
        input=${ver_path[0]}
        version=${ver_candidate[0]}
        return 0
    fi
    
    #:<<'MENU'

    PS3='Please choose version: '

    select opt in "${ver_candidate[@]}"
    do
        if [ "$opt" == "quit" ];then
			log_notify "Exit" "$c_LCyan"
            exit 0
        fi
        
        if [ -z "$opt" ];then
            log_info "invalid input"
        else
            set_ver_param $opt
            version=$opt
            break
        fi

    done

    #MENU
}

choose_project()
{
    local res
    local -a proj_candidate
    local -i proj_num
    local -i i
   
    if [ -z "$input" ];then
        log_exit "[line $LINENO] set correct version before choose project"
    fi

#: << 'PROJ'
    local res=`find $input -type f -name "$proj_pattern"`

    if [ -z "$res" ];then
        log_exit "[line $LINENO] no project source"
    fi

    res=$(echo "$res" | sed 'a,' | tr -d '\n')
    proj_num=$((`echo "$res"|awk -F ',' '{print NF}'`-1))

    for i in `seq 0 1 $((proj_num-1))`
    do
        proj_candidate[$i]=`echo $res | awk -F ',' '{print $'$((i+1))' }' | awk -F '/' '{print $NF}'`
    done
    proj_candidate[$((++i))]="quit"

    #:<<'MENU'

    PS3='Please choose project: '

    select opt in "${proj_candidate[@]}"
    do
        if [ "$opt" == "quit" ];then
			log_notify "Exit" "$c_LCyan"
            exit 0
        fi
        
        if [ -z "$opt" ];then
            log_info "invalid input"
        else
            project=$opt
            break
        fi

    done

    #MENU
#PROJ
}

show_param()
{
    log_info "output  : $output"
    log_info "version : $version"
    log_info "project : $project"

    new="`echo "$version" | awk -F '_' '{print $NF}'` : `echo "$project" | sed 's/vs.//g'`"
}

update_program()
{
    log_info "update program ... "

    test -d "$output"  || log_exit "[line $LINENO] no $output directory"
    test -z "$input" && log_exit "[line $LINENO] no set version"

    cp -r $input $output/ || log_exit "[line $LINENO] copy error"
    sync
}

update_link()
{
    log_info "update link ... "
  
    local link_ver="tm"
    local link_proj="qsi_tm.conf"

    test -z "$project"   && log_exit "[line $LINENO] no set project"
    test -L "$output/$link_ver"  && rm "$output/$link_ver"
    test -L "$output/$link_proj" && rm "$output/$link_proj"

    cd $output
   
    ln -s $version $link_ver || log_exit "[line $LINENO] make link error"
    ln -s $link_ver/$project $link_proj
    sync
}


#################
# Main
#################

cd `dirname $0`
location="$PWD"

parse_options $arglist
choose_version
choose_project
show_param
update_program
update_link

log_notify "Finish qsi_tm [$new]" "$c_LGreen"

