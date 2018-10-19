#!/bin/bash
#
# Schedules a list of wordseg jobs with SLURM. Have a
# "./wordseg-slurm.sh --help" to display help.
#
# author: Mathieu Bernard <mathieu.a.bernard@inria.fr>


# import functions defined in functions.sh
. $(dirname $0)/functions.sh


# sbatch is the job launcher for SLURM
backend="sbatch"


# run a job with sbatch
function schedule_job_backend
{
    # schedule the script to be executed on the cluster
    sbatch --job-name $job_name -n$job_slots --output=$job_dir/log.txt \
           --export=ALL $job_script $backend_options | tee $job_dir/job.pid

    # save the job PID
    sed -i -r 's|Submitted batch job ||' $job_dir/job.pid
}


main "$@" || exit 1

exit 0
