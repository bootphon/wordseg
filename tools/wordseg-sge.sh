#!/bin/bash
#
# Schedules a list of wordseg jobs with SGE. Have a
# "./wordseg-sge.sh --help" to display help.
#
# author: Mathieu Bernard <mathieu.a.bernard@inria.fr>


# import functions defined in functions.sh
. $(dirname $0)/functions.sh


# qsub is the job launcher for SGE
backend="qsub"


# run a job with qsub
function schedule_job_backend
{
    # schedule the script to be executed on the cluster
    qsub -pe openmpi $job_slots $backend_options -j y -V -cwd -S /bin/bash \
         -o $job_dir/log.txt -N $job_name $job_script | tee $job_dir/job.pid

    # save the job PID
    sed -i -r 's|^Your job ([0-9]+).*$|\1|' $job_dir/job.pid
}


main "$@" || exit 1

exit 0
