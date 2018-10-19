wordseg-qsub.sh
===============

Little scripts to launch **wordseg** segmentation jobs on SGE and
SLURM job management systems.


Usage
-----

    ./wordseg-sge.sh <jobs-file> <output-directory> [<qsub-options>]

    ./wordseg-slurm.sh <jobs-file> <output-directory> [<sbatch-options>]


Format of `<jobs-file>`
-----------------------

`<jobs-file>` must contain one job definition per line. A job
definition is made of the following fields:

    <job-name> <prepared-file> <gold-file> <wordseg-command>

* The `<job-name>` must not contain spaces. Name of the job on qsub and
  name of the result directory in `<output-directory>`.

* `<prepared-file>` and `<gold-file>` are files, path expressed from the
  directory containing the `wordseg-qsub.sh` script.

    * `<prepared-file>` is the input text file to be segmented. It is
      a suite of space separeted phonemes or syllables, one utterance
      per line.

    * `<gold-file>` is the gold file to evaluate the segmented output,
      spaces at word boundaries only.

* The `<wordseg-command>` is a usual call to a wordseg algorithm but
  with no options `<input-file>` and `<output-file>` specified.

* `wordseg-dibs` is a special case because it requires a training
  file. That training file must be provided as the **last argument**
  of the command in the `<wordseg-command>` field of the job
  definition.

See `exemple/jobs.txt` for a valid exemple.


Output folder
-------------

The result and intermediate files of each job defined in `jobs.txt`
are stored in distinct folders in
`<output-directory>/<job-name>`. Each folder contains:

* Input:

  * `gold.txt`: Copy of the gold text for evaluation
  * `input.txt`: Copy of the input text for segmentation

* Output:

  * `output.txt`: The segmented text
  * `eval.txt`: Evaluation of the segmented text against the gold
  * `log.txt`: Data logged during script execution

* Job:

  * `job.pid`: The PID of the job (to be used with `qstat`)
  * `job.sh`: The executed job script


Exemple
-------

Have a look to the exemple provided, which executes six flavors of TP
on the same input:

    ./wordseg-qsub.sh exemple/jobs.txt ./results

This tool is part of wordseg <https://github.com/bootphon/wordseg>.
