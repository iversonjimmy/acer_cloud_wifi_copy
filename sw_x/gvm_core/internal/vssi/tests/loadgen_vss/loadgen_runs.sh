#! /bin/bash

# Run loadgen for 1, 10, 100, and 1000 users for the following data sets:
# * 1000 10k files, 10 1M files, 1 10M file.
# Record the results for each run.

wikitable_header="{| class=\"wikitable sortable\"\n|-\n! Data Profile !! Users !! Files/User !! File Size !! Read (MiB/s) !! Read (op/s) !! Write (MiB/s) !! Write (op/s) !! Commit (op/s) !! Merge Time (us)"
wikitable_footer="|}"

start_id=0
users_list="1 10 100 1000"
files_list="10 100 1024 10240"
photos_list="small_test.jpg med_test.jpg large_test.jpg"
max_data=$((1024*10)) #10MiB
logdir=test
username=loadgen%d@igware.com
lab=www-c300.pc.igware.net

do_all=true
do_generic=false
do_docs=false
do_pix=false

function USAGE() {
    echo -e "USAGE:\n$0 [-l LAB] [-u USERNAME] [-U MAXUSERS] [-s ID] [-m SIZE] [-d DIR]"
    echo -e "\t-l LAB\t lab to use [${lab}]"
    echo -e "\t-u USERNAME\t base username or users file [${username}]"
    echo -e "\t-n #USERS\t Number of users to test (may be a quoted list) [${users_list}]"
    echo -e "\t-i ID\t starting ID for \"%s\" in USERNAME [${start_id}]"
    echo -e "\t-f SIZE(S)\t File size(s) in KiB to test (may be a quoted list). Does not apply for PicStream tests, [${files_list}]"
    echo -e "\t-p FILE(S)\t Photo files to use in PicStream tests (may be a quoted list). [${photos_list}]"
    echo -e "\t-m MAX-DATA\t Maximum amount of data (in Kib) to send per user per test (minimum one file/test). [${max_data}]"
    echo -e "\t-d DIR\t directory for test results [${logdir}]"
    echo -e "To limit data profiles, use the following options (all profiles tested by default.)"
    echo -e "\t-A \t Do all data profile tests (default)"
    echo -e "\t-P \t Do PicStream testing. Disables -A option"
    echo -e "\t-D \t Do CloudDocs testing. Disables -A option"
    echo -e "\t-G \t Do generic testing. Disables -A option"
    exit $E_OPTERR
}
 
function run_tests() {
    # Remove target dataset contents.
    ./loadgen_vss -s ${lab} -a -u ${username} -n $users -i $start_id $profile -R 2>&1 > ${log_base}RESET.log
    # Write pass
    ./loadgen_vss -s ${lab} -a -u ${username} -n $users -i $start_id $profile -w 1 2>&1 > ${log_base}w1.log
    # Read pass (to verify data)
    ./loadgen_vss -s ${lab} -a -u ${username} -n $users -i $start_id $profile -r 1 -V 2>&1 > ${log_base}V_r1.log
    # Read pass (for time)
    ./loadgen_vss -s ${lab} -a -u ${username} -n $users -i $start_id $profile -r 1 2>&1 > ${log_base}r1.log
}
   
function report() {
    read_bw=`grep ^+++ ${log_base}r1.log | grep -v dir | grep MiB/s | awk '{ print $4 }'`
    read_op=`grep ^+++ ${log_base}r1.log | grep -v dir | grep op/s | awk '{ print $4 }'`
    # Collect write rates from the w1 log
    write_bw=`grep ^+++ ${log_base}w1.log | grep -v commit | grep MiB/s | awk '{ print $4 }'`
    write_op=`grep ^+++ ${log_base}w1.log | grep -v commit | grep op/s | awk '{ print $4 }'`
    commit_op=`grep ^+++ ${log_base}w1.log | grep commit | grep op/s | awk '{ print $4 }'`
    merge_time=`grep ^+++ ${log_base}w1.log | grep merge | grep us | awk '{ print $4 }'`
    
    echo -e "|-\n| ${profile_name} || ${users} || ${count} || ${size_disp} || ${read_bw} || ${read_op} || ${write_bw} || ${write_op} || ${commit_op} || ${merge_time}"
}

while getopts ":l:u:n:i:d:f:p:m:?APGD" optname
do
    case "$optname" in
        "l")
            lab=$OPTARG
            ;;
        "u")
            username=$OPTARG
            ;;
        "n")
            users_list=$OPTARG
            ;;
        "i")
            start_id=$OPTARG
            ;;
        "d")
            logdir=$OPTARG
            ;;
        "m")
            max_data=$OPTARG
            ;;
        "f")
            files_list=$OPTARG
            ;;
        "p")
            photos_list=$OPTARG
            ;;
        "A")
            do_all=true
            ;;
        "P")
            do_pix=true
            do_all=false
            ;;
        "D")
            do_docs=true
            do_all=false
            ;;
        "G")
            do_generic=true
            do_all=false
            ;;
        "?")
            USAGE
            exit 0
            ;;
        "*")
            echo "Unsupported option:${optname}"
            USAGE
            ;;
    esac
done

mkdir -p ${logdir}

echo -e $wikitable_header

for users in $users_list ; do
    for size_k in $files_list ; do
        count=$(( $max_data / $size_k ))
        if [[ $count -eq 0 ]] ; then
            count=1 
        fi
        
        if [[ $size_k -ge 1024 ]] ; then
            size_disp=$((${size_k} / 1024))M
        else
            size_disp=${size_k}k
        fi
        
        if [[ $do_all == true || $do_generic == true ]] ; then
            profile_name="General-use"
            profile="-f $size_k -c $count"
            log_base=./${logdir}/loadgen_generic_n${users}_f${size_k}_c${count}_
            run_tests
            report
        fi
        
        if [[ $do_all == true || $do_docs == true ]] ; then
            profile_name="CloudDocs"
            profile="-P documents -f $size_k -c $count"
            log_base=./${logdir}/loadgen_docs_n${users}_f${size_k}_c${count}_
            run_tests
            report
        fi
    done
    
    if [[ $do_all == true || $do_pix == true ]] ; then
        for photo in $photos_list ; do
            file_size=`stat -c %s ${photo}`
            
            size_disp=$file_size
            
            count=$(( ($max_data * 1024) / $file_size ))
            if [[ $count -eq 0 ]] ; then
                count=1 
            fi
            profile_name="PicStream"
            profile="-P photos,file=${photo},read=photos -c $count"
            log_base=./${logdir}/loadgen_photo_n${users}_${photo}_c${count}_
            run_tests
            report
        done       
    fi
done

echo -e $wikitable_footer
