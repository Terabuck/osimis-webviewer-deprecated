#!/bin/bash
# this script builds the osimis/orthanc-webviewer-plugin docker image and
# pushes it to DockerHub.  You must be logged in DockerHub before it runs.
#
# arguments: $1 = branchName (optional; if not provided, it will get it 
#                             from a git command.  In a jenkins build context,
#                             the branch name is not available through git so it must be
#                             passed as an argument)
set -e
set -x

#start from the right place
cd "${REPOSITORY_PATH:-$(git rev-parse --show-toplevel)}"/

source scripts/setBuildVariables.sh

imageName='osimis/orthanc-webviewer-plugin'

#build the image
docker build -t $imageName:$releaseCommitId .