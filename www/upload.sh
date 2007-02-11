#!/bin/bash

export IMAGES="*.png *.jpeg"
export TEXT="*.html *.css ChangeLog"
export DOWNLOADS="downloads"

rsync -L -r --rsh="ssh" ${IMAGES} ${TEXT} ${DOWNLOADS} icculus.org:/webspace/homepages/jcspray/referencer/
