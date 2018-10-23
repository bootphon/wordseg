#!/bin/bash
#
# Functions common to wordseg-sge.sh and wordseg-slurm.sh
#
# author: Mathieu Bernard <mathieu.a.bernard@inria.fr>



# A usage string ($backend must be defined)
function usage
{
    echo "Usage: $0 <jobs-file> <output-directory> [<$backend-options>]"
    echo
    echo "Each line of the <jobs-file> must be in the format:"
    echo "  <job-name> <tags-file> <separator> <unit> <wordseg-command>"
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

        # we need at least 5 arguments
        [ "$(echo $line | wc -w)" -lt 5 ] && error "line $n: job definition invalid: $"

        # check tags file exists
        tags_file=$(echo $line | cut -d' ' -f2)
        [ -f $tags_file ] || error "line $n: tags file not found $tags_file"

        # check <unit> is either phone or syllable
        unit=$(echo $line | cut -d' ' -f3)
        ! [ "$unit" == "phone" -o "$unit" == "syllable" ] \
            && error "line $n: unit must be 'phone' or 'syllable', it is $unit"

        # check wordseg command
        [ -z "$(echo $line | grep wordseg-)" ] \
            && error "line $n: wordseg command not found"
        binary=wordseg-$(echo $line | sed -r 's|^.* wordseg-([a-z]+) .*$|\1|')
        [ -z $(which $binary 2> /dev/null) ] && error "line $n: binary not found $binary"
    done < $jobs
}


# determine the number of slots (CPU cores) to be used by a
# <wordseg-command>. Looks for -j or --njobs options in the command.
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


# extract the separator defined for a job
function parse_separator
{
    echo $(echo $1 | cut -d' ' -f4- | sed -r 's|^(.*) wordseg.*$|\1|')
}

# extract the wordseg command defined for a job
function parse_command
{
    echo wordseg-$(echo $1 | sed -r 's|^.* wordseg-(.*)$|\1|')
}


function schedule_job
{
    # parse arguments
    job_name=$(echo $1 | cut -d' ' -f1)
    tags_file=$(echo $1 | cut -d' ' -f2)
    unit=$(echo $1 | cut -d' ' -f3)
    job_cmd=$(parse_command "$1")
    job_slots=$(parse_nslots "$job_cmd")
    separator=$(parse_separator "$1")

    # create the output directory
    job_dir=$output_dir/$job_name
    mkdir -p $job_dir
    job_dir=$(readlink -f $job_dir)

    # copy input data in the output directory
    cp $tags_file $job_dir/tags.txt
    touch $job_dir/log.txt

    # special case for wordseg-dibs, extract the train file as the
    # last argument of $job_cmd
    training_file=
    if [[ $job_cmd == "wordseg-dibs"* ]];
    then
        training_file=${job_cmd##* }  # last word of job_cmd
        job_cmd=${job_cmd% *}  # all but last word of job_cmd
        cp $training_file $job_dir/train.txt
        training_file=train.txt
    fi

    # write the job script that will be scheduled on qsub
    job_script=$job_dir/job.sh
    cat <<EOF > $job_script
#!/bin/bash

tstart=\$(date +%s.%N)

cd $job_dir

echo "Extract statistics" >> log.txt
wordseg-stats -v --json $separator -o stats.json tags.txt 2>> log.txt

echo "Generate input and gold from tags" >> log.txt
wordseg-prep -v -u $unit $separator -o input.txt -g gold.txt tags.txt 2>> log.txt

echo "Start segmentation, command is: $job_cmd" >> log.txt
$job_cmd -o output.txt input.txt $training_file 2>> log.txt

if [ \$? -eq 0 ]
then
    echo "Segmentation done!" >> log.txt

    echo "Start evaluation" >> log.txt
    wordseg-eval -v -o eval.txt output.txt gold.txt 2>> log.txt
    echo "Evaluation done!" >> log.txt
else
    echo "Segmentation failed"
fi

tend=\$(date +%s.%N)
echo "Total time (s): \$(echo "\$tend - \$tstart" | bc)" >> log.txt

EOF

    # run the job on the backend (bash, SLURM or SGE). Read variables
    # from environment
    schedule_job_backend
}


function main
{
    # make sure the backend is installed on the machine
    [ -z $(which $backend 2> /dev/null) ] && error "$backend not found"


    # display usage message when required (bad arguments or --help)
    [ $# -lt 2 -o $# -gt 3 ] && usage
    [ "$1" == "-h" -o "$1" == "-help" -o "$1" == "--help" ] && usage


    # parse input arguments
    jobs_file=$1
    ! [ -f $jobs_file ] && error "file not found: $jobs_file file"

    output_dir=$2
    [ -e $output_dir ] && error "directory already exists: $output_dir"

    backend_options=$3  # may be empty


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
}
