#include <Arduino.h>
#include "app/glasses_app.h"

void setup() { GlassesApp::begin(); }
void loop() { GlassesApp::update(); }
