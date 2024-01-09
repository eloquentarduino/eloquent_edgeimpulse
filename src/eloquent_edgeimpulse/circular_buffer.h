#ifndef ELOQUENT_COMMONS_CIRCULAR_BUFFER
#define ELOQUENT_COMMONS_CIRCULAR_BUFFER
#include "./math.h"


namespace Eloquent {
    namespace Commons {
        /**
         * Circular (endless) buffer
         * @tparam size
         */
        template<typename T, uint16_t size>
        class CircularBuffer {
        public:
            T values[size];

            /**
             * Constructor
             */
            CircularBuffer() :
                _i(0),
                _count(0),
                _batchSize(1),
                _batchCount(0) {

            }

            /**
             * Test if buffer is full
             * @return
             */
            bool isFull() {
                return _count >= size;
            }

            /**
             * Clear the buffer
             * @param deep
             */
            void clear(bool deep = false) {
                _i = 0;
                _count = 0;
                _batchCount = 0;

                if (deep)
                    for (uint16_t i = 0; i < size; i++)
                        values[i] = 0;
            }

            /**
             *
             * @param batchSize
             */
            void batch(uint16_t batchSize) {
                _batchSize = batchSize > 0 ? batchSize : 1;
            }

            /**
             *
             * @param value
             * @return
             */
            template<typename U>
            bool push(U value) {
                U values[1] = {value};

                return push(values, 1);
            }

            /**
             *
             * @param values
             * @param count
             * @return
             */
            template<typename U>
            bool push(U *values, uint16_t length) {
                // there's space for all items
                if (_count + length <= size) {
                    for (uint16_t i = 0; i < length; i++)
                        this->values[i + _count] = values[i];
                }
                // we need to shift items to the left to make room
                else {
                    uint16_t shift = _count >= size ? length : size - _count;
                    uint16_t head = size - length;

                    for (uint16_t i = 0; i < size - shift; i++)
                        this->values[i] = this->values[i + shift];

                    for (uint16_t i = 0; i < length; i++)
                        this->values[head + i] = values[i];
                }

                _batchCount += length;
                _count += length;

                return testBatch();
            }

            /**
             * Get mean value of elements
             * @return
             */
            float mean() {
                return eloquent::commons::math::arrayMean(values, size);
            }

            /**
             *
             * @tparam Printer
             * @param printer
             * @param separator
             * @param end
             */
            template<typename Printer>
            void printTo(Printer printer, char separator = ',', char end = '\n') {
                printer.print(values[0]);

                for (uint16_t i = 1; i < size; i++) {
                    printer.print(separator);
                    printer.print(values[i]);
                }

                printer.print(end);
            }

        protected:
            uint16_t _i;
            uint16_t _batchSize;
            uint16_t _batchCount;
            uint32_t _count;

            /**
             * Test if the given number of elements was pushed since last check
             * @return
             */
            bool testBatch() {
                if (_batchCount >= _batchSize) {
                    _batchCount -= _batchSize;

                    return _count >= size;
                }

                return false;
            }
        };
    }
}

#endif