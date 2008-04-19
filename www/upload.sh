#!/bin/bash

export IMAGES="*.png *.ico"
export TEXT="*.html *.css ChangeLog *.pdf"

rsync -v -L -r --rsh="ssh" ${IMAGES} ${TEXT} icculus.org:/webspace/projects/referencer/
