#!/bin/bash

set -e

bzcat ~/Downloads/db.dat.bz2 | grep '^[DCK]' | iconv -f latin2 -t utf-8  | sed 's/^.//' | bash run.sh ./extractor
