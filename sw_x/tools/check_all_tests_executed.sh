result=`grep TC_COMPLETE $TEST_WORKDIR/../$TEST_NAME.output| sed -e 's/^.*TC_COMPLETE *= *//g' | awk '{print $1}' | sort | wc -l`
echo $result
postrun="_PostRun"
if [ $result -lt $1 ]; then
        echo "TC_RESULT=FAIL ;;; TC_NAME=$SUITE_NAME${postrun}"
else
        echo ""
fi

