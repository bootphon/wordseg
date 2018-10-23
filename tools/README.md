wordseg-qsub.sh
===============

Little scripts to launch **wordseg** segmentation jobs on SGE and
SLURM job management systems.


Usage
-----

    ./wordseg-sge.sh <jobs-file> <output-directory> [<qsub-options>]

    ./wordseg-slurm.sh <jobs-file> <output-directory> [<sbatch-options>]

    ./wordseg-bash.sh <jobs-file> <output-directory>


Format of `<jobs-file>`
-----------------------

`<jobs-file>` must contain one job definition per line. A job
definition is made of the following fields:

    <job-name> <tags-file> <unit> <separator> <wordseg-command>

* The `<job-name>` must not contain spaces. Name of the job on the
  scheduler (SGE or SLURM) and name of the result directory in
  `<output-directory>`.

* `<tags-file>` must be an absolute path, or a path expressed from the
  directory containing the `wordseg-qsub.sh` script. It contains the
  input text in a phonologized form (as a suite of phonemes,
  syllables and word boundaries), one utterance per line.

* `<unit>` is either `phone` or `syllable`, this is the representation
  level at which you want the text to be segmented.

* `<separator>` is a string defining the token separation for phones,
  syllables and words in the tags file. If it includes spaces it must
  be surrounded by quotes. For exemple `"-p' ' -s';esyll' -w';eword'`.

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

  * `tags.txt`: Copy of the input text file
  * `gold.txt`: Gold text for evaluation, generated from tags
  * `input.txt`: Prepared text for segmentation, generated from tags

* Output:

  * `stats.json`: Statistics on the input text, in JSON format
  * `output.txt`: The segmented text
  * `eval.txt`: Evaluation of the segmented text against the gold
  * `log.txt`: Data logged during script execution

* Job:

  * `job.pid`: The PID of the job as executed by the scheduler
  * `job.sh`: The executed job script (not available for
    `wordseg-bash.sh`)


Exemple
-------

Have a look to the exemple provided, which executes six flavors of TP
on the same input:

    ./wordseg-qsub.sh exemple/jobs.txt ./results

This tool is part of wordseg <https://github.com/bootphon/wordseg>.
