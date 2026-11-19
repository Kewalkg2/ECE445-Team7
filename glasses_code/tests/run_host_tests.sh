#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  tests/test_dsp.cpp src/processing/signal_conditioner.cpp \
  src/processing/fft.cpp src/processing/flicker_detector.cpp \
  src/control/attenuation_controller.cpp \
  src/control/feedback_controller.cpp \
  -o /tmp/ece445_dsp_tests
/tmp/ece445_dsp_tests

g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  tests/test_cooperative.cpp src/processing/fft.cpp \
  src/processing/flicker_detector.cpp src/control/attenuation_controller.cpp \
  -o /tmp/ece445_cooperative_tests
/tmp/ece445_cooperative_tests


g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
  tests/test_end_to_end.cpp src/processing/fft.cpp \
  src/processing/flicker_detector.cpp src/control/attenuation_controller.cpp \
  -o /tmp/ece445_end_to_end_tests
/tmp/ece445_end_to_end_tests
