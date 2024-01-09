# Eloquent Edge Impulse

An *beginner-friendly* Arduino library to run [Edge Impulse](https://edgeimpulse.com) models
with ease.

## How to install

Install latest version from the Arduino Library Manager.

## How to use

```cpp
/**
 * Run Edge Impulse model by pasting
 * features into Serial Monitor
 */
#include <your_ei_model_inferencing.h>
#include <eloquent_edgeimpulse.h>

using eloq::edgeimpulse::impulse;


void setup() {
    delay(3000);
    Serial.begin(115200);
    Serial.println("__EDGE IMPULSE SERIAL__");
    Serial.print("Paste your feature vector in the Serial Monitor");
    Serial.print(" (expecting ");
    Serial.print(impulse.numInputs);
    Serial.println(" comma-separated values)");
}


void loop() {
    if (!Serial.available())
        return;

    // fill impulse buffer
    for (int i = 0; i < impulse.numInputs; i++)
        impulse.buffer.push(Serial.readStringUntil(',').toFloat());

    // run regression model (only choose one)
    if (!impulse.regression().isOk()) {
        Serial.println(impulse.exception.toString());
        return;
    }

    // run classification model (only choose one)
    if (!impulse.classify().isOk()) {
        Serial.println(impulse.exception.toString());
        return;
    }

    // run object detection model (only choose one)
    if (!impulse.detectObjects().isOk()) {
        Serial.println(impulse.exception.toString());
        return;
    }

    // if regression
    Serial.print("Predicted value: ");
    Serial.println(impulse.y());

    // if classification
    Serial.print("Predicted class: ");
    Serial.print(impulse.idx());
    Serial.print(", label: ");
    Serial.println(impulse.label());

    // if object detection
    Serial.print("Objects found: ");
    Serial.println(impulse.count());

    impulse.forEach([](int i, ei_impulse_result_bounding_box_t bbox) {

    });

    // you can access the inner result object for more control
    // impulse.result

    // debug detailed info
    impulse.debugTo(Serial);
    impulse.buffer.clear();
    Serial.println();
}
```