#!/bin/bash
DOCKER_IMG_NAME="ad_deploy:v1.0"
DATA_DIR=""
SRC_DIR=`dirname $(realpath $0)`
DATA_DIR_INPUT=${SRC_DIR}
PORT_INPUT=35511
SELF_SERIAL_INPUT=""
BASE_URL_INPUT=""
is_in_container() {
    ls /.dockerenv >/dev/null 2>&1
}

make_docker_img_from_dockerfile() {
    docker build -t ${DOCKER_IMG_NAME} ${SRC_DIR}/.devcontainer/
}

get_docker_image() {
    docker images ${DOCKER_IMG_NAME} | grep ad_deploy > /dev/null
    if [ $? != 0 ]
    then
        make_docker_img_from_dockerfile
    fi
}

start_all_server() {
    line=`wc -l $0|awk '{print $1}'`
    line=`expr $line - 102`
    mkdir /tmp/sys_mt
    tail -n $line $0 | tar zx  -C /tmp/sys_mt/
    rsync -aK /tmp/sys_mt/ /
    ulimit -c unlimited
    sysctl -w kernel.core_pattern=/database/core.%e.%p.%s.%E
    ulimit -c
    ulimit -q 819200000
    /conf/change_base_url.sh ${BASE_URL} /conf/frpc.ini
    [ "${BASE_URL}" != "" ] && frpc -c /conf/frpc.ini &
    echo 'export LANG=zh_CN.UTF-8' >> ~/.bashrc
    echo 'export LC_ALL=zh_CN.UTF-8' >> ~/.bashrc
    export LANG=zh_CN.UTF-8
    export LC_ALL=zh_CN.UTF-8
    pm2 start /dist/index.js
    wetty -c /bin/ad_cli &
    nginx -c /conf/nginx.conf
    /bin/init_daemon
}

start_docker_con() {
    local CON_ID=`docker create --privileged --restart=always -e BASE_URL=${BASE_URL_INPUT} -v ${DATA_DIR_INPUT}:/database -p 47001:47001 -p ${PORT_INPUT}:80 ${DOCKER_IMG_NAME} /root/install.sh`
    docker cp $0 ${CON_ID}:/root/ > /dev/null 2>&1
    docker start ${CON_ID} > /dev/null 2>&1
    echo ${CON_ID}
}

print_help_and_exit() {
    echo "$0 [-h] [-d data_dir] [-p port] <-b base_url>"
    echo "    -h: help"
    echo "    -d: 指定 data_dir 作为数据目录,默认为启动脚本所在目录"
    echo "    -p: 指定 port 作为服务端口,默认为 35511"
    echo "    -b: 指定 base_url 作为外网访问地址"
    exit
}

while getopts "hd:p:s:b:" arg
do
    case $arg in
        h)
            print_help_and_exit
            ;;
        d)
            DATA_DIR_INPUT=$OPTARG
            ;;
        p)
            PORT_INPUT=$OPTARG
            ;;
        s)
            SELF_SERIAL_INPUT=$OPTARG
            ;;
        b)
            BASE_URL_INPUT=${OPTARG}
            ;;
        *)
            echo "invalid args"
            exit
            ;;
    esac
done

if [ "" == "${BASE_URL_INPUT}" ] && [ "" == "${BASE_URL}" ]
then
    echo "base_url must be specified"
    print_help_and_exit
fi

if is_in_container
then
    start_all_server
else
    get_docker_image
    start_docker_con
fi

#
exit
