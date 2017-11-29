#!/bin/sh

# Inspired by https://gist.github.com/vidavidorra/548ffbcdae99d752da02
#
# This script is executed by Travis after tests success.
#
# Configure the project to make the configuration buildable on
# readthedocs. Push the configured project to the readthedocs-pages
# branch. The push then trigger the documentation build on
# readthedocs.org
#
# Required installed packages: cmake sphinx
#
# Required global variables:
# - TRAVIS_BUILD_NUMBER : The number of the current build.
# - TRAVIS_COMMIT       : The commit that the current build is testing.
# - GH_REPO_NAME        : The name of the repository.
# - GH_REPO_REF         : The GitHub reference to the repository.
# - GH_REPO_TOKEN       : Secure token to the github repository.

set -e

SOURCE=$TRAVIS_BUILD_DIR
TARGET=$SOURCE/readthedocs-pages

echo "Setting up the script in $SOURCE"

# Set the push default to simple i.e. push only the current branch.
git config --global push.default simple

# Pretend to be an user called Travis CI.
git config user.name "Travis CI"
git config user.email "travis@travis-ci.org"

# get the readthedocs-pages branch in a clean working directory for
# this script
git clone -b readthedocs-pages https://git@$GH_REPO_REF $TARGET

# Remove everything currently in the gh-pages branch.  GitHub is smart
# enough to know which files have changed and which files have stayed
# the same and will only update the changed files. So the gh-pages
# branch can be safely cleaned, and it is sure that everything pushed
# later is the new documentation.
rm -rf $TARGET/*

echo "Configure the project in $TARGET"
# copy all targeted files
cp -a $SOURCE/doc $TARGET
cp -a $SOURCE/wordseg $TARGET
cp -a $SOURCE/CHANGELOG.rst $TARGET
cp -a $SOURCE/build/setup.py $TARGET
cp -a $SOURCE/build/html/conf.py $TARGET/doc

# remove *.in, __pycache__ and *.pyc files
rm -f $TARGET/doc/conf.py.in
find $TARGET -type f -name *.pyc -delete
# find $TARGET -type d -name __pycache__ -exec rm -rf {} \;

echo "Uploading the project to github readthedocs-pages branch"
# add all the configured files
cd $TARGET
git add --all
git status

# Commit the added files with a title and description containing the
# Travis CI build number and the GitHub commit reference that issued
# this build.
git commit \
    -m "Deploy to github readthedocs-pages Travis build: ${TRAVIS_BUILD_NUMBER}" \
    -m "Commit: ${TRAVIS_COMMIT}"

# Force push to the remote gh-pages branch.
# The ouput is redirected to /dev/null to hide any sensitive credential data
# that might otherwise be exposed.
echo "Pushing to ${GH_REPO_REF}"
git push --force "https://${GH_REPO_TOKEN}@${GH_REPO_REF}" > /dev/null 2>&1

echo "Done"
