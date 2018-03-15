#!/bin/sh

TEST_SCRIPT=$1
MODULE=
PARAM=
CASENUM=1
TOTALCASENUM=0
CUR_PATH=`pwd`

clean_env()
{
	rm ${CUR_PATH}/*.yuv  2>/dev/null
        rm ${CUR_PATH}/*.h264 2>/dev/null
        rm ${CUR_PATH}/*.h265 2>/dev/null
        rm ${CUR_PATH}/*.jpeg 2>/dev/null
}

case_print()
{
	echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
	echo "Module: $1, Run Case: $2, Total: $3"
	echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
}

clean_env

###Script start here
if [ "$CUR_PATH" != "/mnt" ] && [ "$CUR_PATH" != "/mnt/" ];
then 
	echo "The Current Dir is not /mnt"
	exit 1
fi

if [ "$#" == 1 ];then
	echo "Auto run test"
elif [ "$#" == 2 ];then
	echo "Single case test"
else
	echo "Input error"
	echo "For Auto run test, USAGE: $0 <FileName>"
	echo "For Single case test, USAGE: $0 <FileName> <Casenum>"
	echo  " "
        exit 1;
fi

if [ ! -f "$TEST_SCRIPT" ];then
	echo "File $TEST_SCRIPT not exits!"
	exit 1;
fi

DSTPATH="${CUR_PATH}/output/"
if [ ! -d "$DSTPATH" ];then
	mkdir -m 777 -p $DSTPATH
fi

if [ ! -w "$DSTPATH" ];then
	chmod 777 $DSTPAT -R
fi

###check MODULE
while read line
do
	args=`echo $line | awk -F= '{ print $1; }'`
	if [ "$args" = "MODULE" ];then
		MODULE=`echo $line | awk -F= '{ print $2; }'`
		break
	fi	
done < $TEST_SCRIPT 

if [ -z "$MODULE" ];then
	echo "Not find MODULE, check your script please!"
	exit 1
fi

DSTPATH="${CUR_PATH}/output/${MODULE}"
if [ "$#" == 1 ];then
	if [ -d "$DSTPATH" ];then
        	rm $DSTPATH -rf
	fi
	mkdir -m 777 -p $DSTPATH
elif [ "$#" == 2 ];then
	if [ ! -d "$DSTPATH" ];then
                mkdir -m 777 -p $DSTPATH
        else
		chmod 777 $DSTPATH -R
	fi	
fi

###check total case num
while read line
do
        args= `echo $line | grep "^#"` || `echo $line | grep "^//"`
        if [ ! -z "$args" ];then
                continue
        fi

        args=`echo $line | awk -F= '{ print $1; }'`
        if [ "$args" = "CASE" ];then
                let "TOTALCASENUM=TOTALCASENUM+1"
        fi
done < $TEST_SCRIPT 2>/dev/null

if [ "$#" == 2 ];then
	if [ "$TOTALCASENUM" -lt "$2" ] || [ "$2" -le 0 ];then
		echo "Input err, total case num is "$TOTALCASENUM""
		echo "USAGE: $0 <FileName> <1~${TOTALCASENUM}>"
      		exit 1;
	fi
fi

##run case
if [ "$#" == 1 ];then 
	while read line
	do
		args= `echo $line | grep "^#"` || `echo $line | grep "^//"`
		if [ ! -z "$args" ];then
			continue	
		fi

		args=`echo $line | awk -F= '{ print $1; }'`
		if [ "$args" = "CASE" ];then
			PARAM=`echo $line | awk -F= '{ print $2; }'`
			case_print $MODULE $CASENUM $TOTALCASENUM 
			echo -e "Param: $PARAM \n"
			sleep 3		
			echo $MODULE $PARAM > /proc/hal/uttest

			DSTPATH="${CUR_PATH}/output/${MODULE}/${MODULE}_case_${CASENUM}"
                	if [ ! -d "$DSTPATH" ];then
                        	mkdir -m 777 -p $DSTPATH
                	fi

			mv ./*.yuv  $DSTPATH 2>/dev/null
                	mv ./*.h264 $DSTPATH 2>/dev/null
                	mv ./*.h265 $DSTPATH 2>/dev/null
                	mv ./*.jpeg $DSTPATH 2>/dev/null

			let "CASENUM=CASENUM+1"
			sleep 2
		fi
	done < $TEST_SCRIPT 2>/dev/null
elif [ "$#" == 2 ];then
	while read line
        do
                args= `echo $line | grep "^#"` || `echo $line | grep "^//"`
                if [ ! -z "$args" ];then
                        continue
                fi

                args=`echo $line | awk -F= '{ print $1; }'`
                if [ "$args" = "CASE" ];then
			if [ "$CASENUM" != "$2" ];then
				let "CASENUM=CASENUM+1" 
				continue
			fi

                        PARAM=`echo $line | awk -F= '{ print $2; }'`
                        case_print $MODULE $CASENUM $TOTALCASENUM
                        echo -e "Param: $PARAM \n"
                        sleep 2
                        echo $MODULE $PARAM > /proc/hal/uttest

                        DSTPATH="${CUR_PATH}/output/${MODULE}/${MODULE}_case_${CASENUM}"
                        if [ ! -d "$DSTPATH" ];then
                                mkdir -m 777 -p $DSTPATH
			else
				rm ${DSTPATH}/* -rf
                        fi

                        mv ./*.yuv  $DSTPATH 2>/dev/null
                        mv ./*.h264 $DSTPATH 2>/dev/null
                        mv ./*.h265 $DSTPATH 2>/dev/null
                        mv ./*.jpeg $DSTPATH 2>/dev/null

			break
                fi
        done < $TEST_SCRIPT 2>/dev/null
fi


echo "Module: $MODULE, Run Case Completed!"







