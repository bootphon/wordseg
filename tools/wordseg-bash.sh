#!/bin/bash
#
# Schedules a list of wordseg jobs on bash. Have a
# "./wordseg-bash.sh --help" to display help.
#
# author: Mathieu Bernard <mathieu.a.bernard@inria.fr>


# import functions defined in functions.sh
. $(dirname $0)/functions.sh


# sbatch is the job launcher for SLURM
backend="bash"


# run a job with bash TODO should be optimized to run jobs in parallel
# when possible ($job_slots)
function schedule_job_backend
{
    echo -n "running $job_name ..."
    bash $job_script
    echo " done"
}


main "$@" || exit 1

exit 0
