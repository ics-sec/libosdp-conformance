#!/bin/bash

# (C)Copyright 2015-2016 Smithee,Spelvin,Agnew & Plinge, Inc.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#   http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

echo "Content-type: text/html"
echo ""

echo "<HTML><HEAD><TITLE>Last 400 lines of Monitor log</TITLE>"
echo "<BODY>"
echo "<BR>"
echo "<PRE>"
date
sudo -n tail -400 /opt/osdp-conformance/run/MON/open-osdp.log
echo "</BODY></HTML>"


