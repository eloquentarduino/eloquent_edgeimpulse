#ifndef ELOQUENT_EDGEIMPULSE_IMPULSE
#define ELOQUENT_EDGEIMPULSE_IMPULSE

#include "./exception.h"
#include "./circular_buffer.h"

using Eloquent::Error::Exception;
using Eloquent::Commons::CircularBuffer;


namespace Eloquent {
    namespace EdgeImpulse {
        /**
         * Eloquent interface to the EdgeImpulse library
         */
        class Impulse {
        public:
            const size_t numInputs = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
            const size_t numOutputs = EI_CLASSIFIER_LABEL_COUNT;
            Exception exception;
            ei_impulse_result_t result;
#ifndef EI_NO_BUFFER
            CircularBuffer<float, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE> buffer;
#endif

            /**
             * Constructor
             * @param debugOn
             */
            Impulse() :
                exception("Impulse"),
                _y(0),
                _errorCode(0),
                _debug(false),
                _maxAnomaly(1) {

            }

            /**
             * Get regression output
             * @return
             */
            float y() {
                return _y;
            }

            /**
             * Get classification output
             * @return
             */
            int8_t idx() {
                return _y;
            }

            /**
             * Toggle verbose mode
             */
            void verbose(bool verbose = true) {
                _debug = verbose;
            }

            /**
             * Perform regression
             * @param features
             * @return
             */
            Exception& regression(float *features = NULL) {
                if (!_run(features).isOk())
                    return exception;

                _y = result.classification[0].value;

                return exception.clear();
            }

            /**
             * Perform classification
             * @param features
             * @return
             */
            Exception& classify(float *features = NULL) {
                if (!_run(features).isOk())
                    return exception;

                if (isAnomaly())
                    return exception.set("Detected anomaly");

                _argmax();

                return exception.clear();
            }

            /**
             * Get label of current prediction
             * @return
             */
            String label() {
                return result.classification[idx()].label;
            }

            /**
             * Get probability of current prediction
             * @return
             */
            float proba() {
                return result.classification[idx()].value;
            }

            /**
             * Set max allowed anomaly score
             */
            void setMaxAnomalyScore(float score) {
                _maxAnomaly = score;
            }

            /**
             * Check if current prediction is an anomaly
             * (if enabled)
             */
            bool isAnomaly() {
#if EI_CLASSIFIER_HAS_ANOMALY == 1
                return result.anomaly > _maxAnomaly;
#else
                return false;
#endif
            }

#ifdef EI_CLASSIFIER_OBJECT_DETECTION
            /**
             * Run object detection
             */
            Exception& detectObjects(float *features = NULL) {
                if (!_run(features).isOk())
                    return exception;

                return exception.clear();
            }

            /**
             * Check if objects were found
             */
            bool foundAnyObject() {
                return result.bounding_boxes[0].value != 0;
            }

            /**
             * Get count of (non background) bounding boxes
             */
            size_t count() {
                size_t count = 0;

                for (size_t ix = 0, i = 0; ix < result.bounding_boxes_count; ix++) {
                    if (result.bounding_boxes[ix].value > 0)
                        count++;
                }

                return count;
            }

            /**
             * Run function on each bounding box found
             */
            template<typename Callback>
            void forEach(Callback callback) {
                for (size_t ix = 0, i = 0; ix < result.bounding_boxes_count; ix++) {
                    auto bb = result.bounding_boxes[ix];

                    if (bb.value > 0)
                        callback(i++, bb);
                }
            }
#endif

            /**
             * Debug classification result to given printer
             * @tparam Printer
             * @param printer
             */
            template<class Printer>
            void debugTo(Printer &printer) {
                printer.print("EdgeImpulse classification results\n");
                printer.print("----------------------------------\n");
                printer.print(" > Outputs\n");

#ifdef EI_CLASSIFIER_OBJECT_DETECTION
                for (size_t i = 0; i < result.bounding_boxes_count; i++) {
                    auto bb = result.bounding_boxes[i];

                    if (bb.value < 0.01)
                        continue;

                    Serial.print("   > ");
                    Serial.print(bb.label);
                    Serial.print(" at (x, y) = (");
                    Serial.print(bb.x);
                    Serial.print(", ");
                    Serial.print(bb.y);
                    Serial.print("), proba = ");
                    Serial.println(bb.value, 2);
                }
#else
                for (uint8_t i = 0; i < numOutputs; i++) {
                    printer.print("   > ");
                    printer.print(result.classification[i].label);
                    printer.print(": ");
                    printer.print(result.classification[i].value);
                    printer.print("\n");
                }
#endif

#if EI_CLASSIFIER_HAS_ANOMALY == 1
                printer.print("    > Anomaly: ");
                printer.print(result.anomaly);
                printer.print("\n");
#endif

                printer.print(" > Timing\n");
                printer.print("    > DSP: ");
                printer.print(result.timing.dsp);
                printer.print(" ms\n");
                printer.print("    > Classification: ");
                printer.print(result.timing.classification);
                printer.print(" ms\n");

#if EI_CLASSIFIER_HAS_ANOMALY == 1
                printer.print("    > Anomaly: ");
                printer.print(result.timing.anomaly);
                printer.print(" ms\n");
#endif

                printer.print(" > Error: ");
                printer.print(exception.isOk() ? "OK" : exception.toString());
                printer.print(" (code ");
                printer.print(_errorCode);
                printer.print(")\n");
            }

        protected:
            float _y;
            bool _debug;
            int _errorCode;
            float _maxAnomaly;
            String _errorMessage;
            signal_t _signal;

            /**
             * Run impulse
             * @return
             */
            Exception& _run(float *features) {
                // use internal buffer, if full
                if (features == NULL) {
                    if (!buffer.isFull())
                        return exception.set("Buffer not full yet");

                    features = buffer.values;
                }

                _errorCode = numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &_signal);

                if (_errorCode)
                    return exception.set("ei::numpy::signal_from_buffer failed");

                _errorCode = run_classifier(&_signal, &result, _debug);

                if (_errorCode)
                    return exception.set("ei::run_classifier failed");

                return exception.clear();
            }

            /**
             *
             */
            void _argmax() {
                _y = 0;
                float proba = result.classification[0].value;

                for (size_t ix = 1; ix < numOutputs; ix++) {
                    float p = result.classification[ix].value;

                    if (p > proba) {
                        _y = ix;
                        proba = p;
                    }

                    if (proba >= 0.5)
                        break;
                }
            }
        };
    }
}

namespace eloq {
    namespace edgeimpulse {
        static Eloquent::EdgeImpulse::Impulse impulse;
    }
}
#endif