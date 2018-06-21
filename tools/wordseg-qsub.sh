#!/bin/bash
#
# Schedules a list of wordseg jobs with qsub. Have a
# "./wordseg-qsub.sh --help" to display help.
#
# author: Mathieu Bernard <mathieu.a.bernard@inria.fr>


######################### Functions definitions ################################


# A usage string
function usage
{
    echo "Usage: $0 <jobs-file> <output-directory> [<qsub-options>]"
    echo
    echo "Each line of the <jobs-file> must be in the format:"
    echo "  <job-name> <prepared-file> <gold-file> <wordseg-command>"
    echo
    echo "See $(dirname $0)/README.md for more details"
    exit 1
}


# display an error message and exit
function error
{
    if [ -z "$1" ]
    then
        message="fatal error"
    else
        message="fatal error: $1"
    fi

    echo $message
    exit 1
}


# ensure all entries in <jobs-file> are valid
function check_jobs
{
    jobs=$1

    # ensure all names (1st column) are different
    nall=$(cat $jobs | wc -l)
    nuniques=$(cat $jobs | cut -f 1 -d ' ' | sort | uniq | wc -l)
    ! [ "$nuniques" -eq "$nall" ] && error "job names are not all unique"

    n=0
    while read line
    do
        n=$(( n + 1 ))

        # we need at least 4 arguments
        [ "$(echo $line | wc -w)" -lt 4 ] && error "line $n: job definition invalid: $"

        # check prepared file exists
        prepared_file=$(echo $1 | cut -d' ' -f2)
        [ -f $prepared_file ] || error "prepared file not found: $prepared_file"

        # check gold file exists
        gold_file=$(echo $1 | cut -d' ' -f3)
        [ -f $gold_file ] || error "gold file not found: $gold_file"

        # check prepared and gold have the same number of lines
        np=$(cat $prepared_file | wc -l)
        ng=$(cat $gold_file | wc -l)
        ! [ "$np" -eq "$ng" ] \
            && error "prepared and gold files have different number of lines ($np != $ng)"

        # check wordseg command
        binary=$(echo $line | cut -f4 -d' ')
        [ -z $(which $binary 2> /dev/null) ] && error "line $n: binary not found: $binary"
    done < $jobs
}


# determine the number of slots (CPU cores) to be used by a <wordseg-command>
function parse_nslots
{
    cmd=$1

    # look for -j
    [[ "$*" == *" -j"* ]] && \
        nslots=$(echo $cmd | sed -r 's|^.* -j *([0-9]+).*$|\1|')

    # if fail, look for --njobs
    [ -z $nslots ] && [[ "$*" == *" --njobs"* ]] && \
        nslots=$(echo $cmd | sed -r 's|^.* --njobs *([0-9]+).*$|\1|')

    # by default, use 1 slot
    [ -z $nslots ] && nslots=1

    echo $nslots
}


function schedule_job
{
    # parse arguments
    job_name=$(echo $1 | cut -d' ' -f1)
    job_cmd=$(echo $1 | cut -d' ' -f4-)
    job_slots=$(parse_nslots "$job_cmd")
    prepared_file=$(echo $1 | cut -d' ' -f2)
    gold_file=$(echo $1 | cut -d' ' -f3)

    # create the output directory
    job_dir=$output_dir/$job_name
    mkdir -p $job_dir
    job_dir=$(readlink -f $job_dir)

    # copy input data in the output directory
    cp $prepared_file $job_dir/input.txt
    cp $gold_file $job_dir/gold.txt
    touch $job_dir/log.txt

    # write the job script that will be scheduled on qsub
    job_script=$job_dir/job.sh
    cat <<EOF > $job_script
#!/bin/bash

tstart=\$(date +%s.%N)

cd $job_dir

echo "Start segmentation, command is: $job_cmd" >> log.txt
$job_cmd -o output.txt input.txt 2>> log.txt

if [ \$? -eq 0 ]
then
    echo "Segmentation done!" >> log.txt

    echo "Start evaluation" >> log.txt
    wordseg-eval -v -o eval.txt output.txt gold.txt 2>> log.txt
    echo "Evaluation done!" >> log.txt
else
    echo "Segmentation failed"
fi

qstat -j \$(cat job.pid) > qstat.txt
tend=\$(date +%s.%N)
echo "Total time (s): \$(echo "\$tend - \$tstart" | bc)" >> log.txt

EOF

    # schedule the script to be executed on the cluster
    qsub -pe openmpi $job_slots $qsub_options -j y -V -cwd -S /bin/bash \
         -o $job_dir/log.txt -N $job_name $job_script | tee $job_dir/job.pid

    # save the job PID
    sed -i -r 's|^Your job ([0-9]+).*$|\1|' $job_dir/job.pid
}



######################### Script starts here ###################################


# make sure qsub is installed on the machine
[ -z $(which qsub 2> /dev/null) ] && error "qsub not found"


# display usage message when required (bad arguments or --help)
[ $# -lt 2 -o $# -gt 3 ] && usage
[ "$1" == "-h" -o "$1" == "-help" -o "$1" == "--help" ] && usage


# parse input arguments
jobs_file=$1
! [ -f $jobs_file ] && error "file not found: $jobs file"

output_dir=$2
[ -e $output_dir ] && error "directory already exists: $output_dir"

qsub_options=$3  # may be empty


# remove any comments in the jobs file
tmp_file=$(mktemp)
trap "rm -f $tmp_file" EXIT
cat $jobs_file | sed "/^\s*#/d;s/\s*#[^\"']*$//" > $tmp_file
jobs_file=$tmp_file


# check the job definitions are correct
check_jobs $jobs_file

echo "submitting $(cat $jobs_file | wc -l) jobs, writing to $output_dir"


# schedule all the defined jobs
while read job
do
    schedule_job "$job"
done < $jobs_file


exit 0
