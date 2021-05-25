#include <Adafruit_MCP9808.h>
Adafruit_MCP9808 mcp;

bool setupTemp() {
    if (mcp.begin(MCP9808_ADDR, &Wire)) {
        sensor_state |= (1 << 1);
        mcp.setResolution(3);
        mcp.shutdown_wake(0);

        return true;
    }

#if defined(DEBUG)

    if (sensor_state & 0b10) {
        Serial.println("[SENSOR] MCP9808 did not respond. Please check wiring.");
    }

#endif

    sensor_state &= ~(1 << 1);

    return false;
}

void readTemp(sensorData *data) {
    data->temp[1] = mcp.readTempC();

    if (!isnan(data->temp[1])) {
        data->temp[1] = round2(temp_filter.add(data->temp[1]));

    } else {
        sensor_state &= ~(1 << 1);
        temp_filter.reset();

#if defined(DEBUG)
        Serial.println("[SENSOR] MCP9808 stopped responding.");
#endif
    }
}
