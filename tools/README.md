wordseg-qsub.sh
===============

A little script to launch
[wordseg](https://github.com/bootphon/wordseg) segmentation jobs on
qsub.


Usage
-----

    ./wordseg-qsub.sh <jobs-file> <output-directory> <prepared-file> <gold-file> [<qsub-options>]

Get a detailed usage message with:

    ./wordseg-qsub.sh --help


Exemple
-------

Have a look to the exemple provided, which executes six flavors of TP
on the same input:

    ./wordseg-qsub.sh exemple/jobs.txt ./results exemple/input.txt exemple/gold.txt


Format of `jobs.txt`
--------------------

`<jobs-file>` must contain one job definition per line. A job
definition is made of the following fields:

    <job-name> <wordseg-command>

The `<job-name>` must not contain spaces. The <wordseg-command> is a
usual call to a wordseg algorithm but with no options <input-file> and
<output-file> specified. See `exemple/jobs.json` for a valid exemple.


Output folder
-------------

The result and intermediate files of each job defined in `jobs.txt`
are stored in separate folders. Each folder contains those files:

* Input:

  * `gold.txt`: Copy of the gold text for evaluation
  * `input.txt`: Copy of the input text for segmentation

* Output:

  * `output.txt`: The segmented text
  * `eval.txt`: Evaluation of the segmented text aginst the gold
  * `log.txt`: Data logged during script execution

* Job:

  * `job.pid`: The PID of the job (to be used with `qstat`)
  * `job.sh`: The executed job script
  * `qstat.txt`: Output of the `qstat` command executed at the very
    end of the script. To see how many memory the job occupied (this
    was an issue with `wordseg-ag`), have a:

        cat qstat.txt | grep usage | sed -r 's|.*maxvmem=||'
