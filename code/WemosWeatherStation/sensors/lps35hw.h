#include <LPS35HW.h>
LPS35HW lps;

bool setupBaro() {
    if (lps.begin(&Wire)) {
        lps.setLowPassFilter(LPS35HW::LowPassFilter_ODR9);
        sensor_state |= (1 << 0);

        return true;
    }

#if defined(DEBUG)

    if (sensor_state & 0b1) {
        Serial.println("[SENSOR] LPS35HW did not respond. Please check wiring.");
    }

#endif

    sensor_state &= ~(1 << 0);

    return false;
}

void readBaro(sensorData *data) {
    data->pressure[1] = lps.readPressure(); // abs
    data->temp[0] = lps.readTemp();

    if (!isnan(data->temp[0]) && !isnan(data->pressure[1])) {
        data->pressure[0] = round2(baro_filter[0].add(
                                       meteoFunctions.relativePressure_c(
                                           data->pressure[1], height_above_sea, data->temp[0])));
        data->pressure[1] = round2(baro_filter[1].add(data->pressure[1]));
        data->temp[0] = round2(baro_temp_filter.add(data->temp[0]));

    } else {
        sensor_state &= ~(1 << 0);
        baro_filter[0].reset();  // rel
        baro_filter[1].reset();  // abs
        baro_temp_filter.reset();

#if defined(DEBUG)
        Serial.println("[SENSOR] LPS35HW stopped responding.");
#endif
    }
}
