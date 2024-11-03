#!/bin/bash -eux
# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

# Script for running a full OSS-Fuzz-gen cycle:
# 1) Generate OSS-Fuzz projects from scratch with a single harness.
# 2) Extract Fuzz Introspector report to provide program analysis data.
# 3) Run OSS-Fuzz-gen to create a larger set of harnesses.

BENCHMARK_HEURISTICS="${VARIABLE:-jvm-public-candidates}"
OSS_FUZZ_GEN_MODEL=${MODEL}
TARGETS_FUZZ="${TARGETS}"
VAR_HARNESSES_PER_PROJECT="${HARNESS_PER_PROJECT:-5}"

# Make sure OFG does not erase any bits in the target OSS-Fuzz project
export OFG_CLEAN_UP_OSS_FUZZ=0

# Define base directories.
BASE_DIR=$PWD
OSS_FUZZ_GEN_DIR=${BASE_DIR}/../../
WORKDIR=$PWD/workdir

# Set environment if it's not already set up.
if [ -d "${WORKDIR}" ]
then
  echo "Workdir exists, reusing set up."
else
  echo "Creating workdir."
  mkdir -p $WORKDIR
  cd $WORKDIR
  # set up virtualenv
  python3.11 -m pip install virtualenv
  python3.11 -m virtualenv .venv
  . .venv/bin/activate

  # set up fuzz introspector
  git clone https://github.com/ossf/fuzz-introspector
  cd fuzz-introspector
  python3 -m pip install -r ./requirements.txt
  cd tools/web-fuzzing-introspection/
  python3 -m pip install -r ./requirements.txt

  # set up oss-fuzz
  cd $WORKDIR
  git clone https://github.com/google/oss-fuzz

  cd ${OSS_FUZZ_GEN_DIR}
  python3 -m pip install -r ./requirements.txt

  # exit virtualenv
  deactivate
fi

# Define Fuzz Introspector, OSS-Fuzz and OFG directories.
ROOT_FI=${WORKDIR}/fuzz-introspector
OSS_FUZZ_DIR=${WORKDIR}/oss-fuzz
OFG_DIR=${BASE_DIR}/../../

cd $WORKDIR
. .venv/bin/activate

# Generate OSS-Fuzz projects from scratch
cd ${BASE_DIR}
python3 ./generate_projects.py -o ${OSS_FUZZ_DIR} -u ${TARGETS_FUZZ} -w $WORKDIR
comma_separated_project_name=`cat $WORKDIR/project-name`

# Generate fresh introspector reports that OFG can use as seed for auto
# generation.
echo "Creating introspector reports"
cd ${OSS_FUZZ_DIR}
for project in `echo $comma_separated_project_name | sed 's/,/ /g'`
do
  python3 $ROOT_FI/oss_fuzz_integration/runner.py \
      introspector $project 10 --disable-webserver
  # Reset is necessary because some project exeuction
  # could break the display encoding which affect
  # the later oss-fuzz-gen execution.
  reset
done

# Shut down the existing webapp if it's running
curl --silent http://localhost:8080/api/shutdown || true

# Create Fuzz Introspector's webserver DB
echo "[+] Creating the webapp DB"
cd $ROOT_FI/tools/web-fuzzing-introspection/app/static/assets/db/
python3 ./web_db_creator_from_summary.py \
    --local-oss-fuzz ${OSS_FUZZ_DIR}

# Start webserver
echo "Shutting down server in case it's running"
curl --silent http://localhost:8080/api/shutdown || true

echo "[+] Launching FI webapp"
cd $ROOT_FI/tools/web-fuzzing-introspection/app/
FUZZ_INTROSPECTOR_LOCAL_OSS_FUZZ=${OSS_FUZZ_DIR} \
  python3 ./main.py >> /dev/null &

SECONDS=5
while true
do
  # Checking if exists
  MSG=$(curl -v --silent 127.0.0.1:8080 2>&1 | grep "Fuzzing" | wc -l)
  if [[ $MSG > 0 ]]; then
    echo "Found it"
    break
  fi
  echo "- Waiting for webapp to load. Sleeping ${SECONDS} seconds."
  sleep ${SECONDS}
done

# Run OSS-Fuzz-gen on the projects
echo "[+] Running OSS-Fuzz-gen experiment"
cd ${OSS_FUZZ_GEN_DIR}
LLM_NUM_EVA=1 LLM_NUM_EXP=1 ./run_all_experiments.py \
    --model=$OSS_FUZZ_GEN_MODEL \
    -g ${BENCHMARK_HEURISTICS} \
    -gp ${comma_separated_project_name} \
    -gm ${VAR_HARNESSES_PER_PROJECT} \
    -of ${OSS_FUZZ_DIR} \
    -e http://127.0.0.1:8080/api

echo "Shutting down started webserver"
curl --silent http://localhost:8080/api/shutdown || true

echo "Merging the generated projects"
cd $BASE_DIR
python3 ./result_merger.py \
    -r ../../results/ \
    -d ${WORKDIR}/auto-generated-projects/ \
    -p ${comma_separated_project_name} \
    -o ${OSS_FUZZ_DIR}

echo "Auto-generated OSS-Fuzz projects in ${WORKDIR}/auto-generated-projects/"
