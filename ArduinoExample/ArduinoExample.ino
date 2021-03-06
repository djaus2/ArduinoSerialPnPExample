#include <Arduino.h>
#include <Wire.h>
#include "SerialPnP.h"

//////////////////////

#include <Seeed_BMP280.h>

void ReadEnvironment( float * temp);

bool g_state = true;

BMP280 bmp280;

void GroveSetup()
{
  g_state = true;
  while (!bmp280.init());
}


void ReadEnvironment( float * temp )
{
 * temp = bmp280.getTemperature();
}

// This function handles calibration and will return success or failure as a bool.
void CbToggleState(
    int32_t     *input,
    int32_t     *output
)
{
    g_state = !g_state;
    *output = 1;
}

/////////////////////

// This global variable dictates the sample rate.
uint32_t g_SampleRate = 3000; // default = 1000ms

// Tracks time of last sample in Arduino ticks
uint32_t g_LastSample = 0;


// This function will be called by SerialPnP to both read and update the sample
// rate of the temperature. The schema of the property data is `SerialPnPSchema_Int`
// as defined on initialization, so the method signature is defined accordingly.
void CbSampleRate(
    int32_t     *input,
    int32_t     *output
)
{
    // If input variable is defined, validate and update sample rate variable
    if (input) {
        // Only allow sample rates between 100ms and 10s
        if ((*input > 100) && (*input <= 10000)) {
            g_SampleRate = *input;
        }
    }
    // Output will always be defined for a property callback
    *output = g_SampleRate;
}

// This function handles calibration and will return success or failure as a bool.
void CbCalibrate(
    int32_t     *input,
    int32_t     *output
)
{
    *output = *input << 1;
}

void setup() {
    SerialPnP_Setup("Example Thermometer");

    // This begins configuration of a new Azure PnP interface, backed by the
    // provided URI.
    SerialPnP_NewInterface("http://contoso.com/thermometer_example");

    // This provides an event, which will be periodically sent by the device to
    // provide a temperature measurement.
    SerialPnP_NewEvent("temperature",                          // short ID for the event
                       "Ambient Temperature",                  // friendly name for event
                       "A sample of the ambient temperature.", // description of event
                       SerialPnPSchema_Float,                  // data schema for temperature data
                       "celsius");                             // units of the temperature data

    // This provides a property which the gateway may use to configure the sample rate
    // for the temperature data.
    SerialPnP_NewProperty("sample_rate",                             // short ID for property
                          "Sample Rate",                             // friendly name for property
                          "Sample Rate of temperature measurements", // description of property
                          "ms",                                      // unit of sample rate is millisecond
                          SerialPnPSchema_Int,                       // data schema of property
                          false,                                     // required
                          true,                                      // writeable by gateway
                          (SerialPnPCb*) CbSampleRate);              // callback - see below

    // This provides a calibration method which allows the gateway to provide an ambient
    // temperature for calibration purposes.
    SerialPnP_NewCommand("calibrate",                                // short ID for command
                         "Calibrate Temperature",                    // friendly name for command
                         "Calibrates the thermometer",               // description of command
                         SerialPnPSchema_Int,                        // data schema of input to command
                         SerialPnPSchema_Int,                        // data schema of output to command
                         (SerialPnPCb*) CbCalibrate);                // callback - see below

        SerialPnP_NewCommand("toggle_state",                         // short ID for command
                         "Toggle State",                             // friendly name for command
                         "Enable-disbale sampling",                  // description of command
                         SerialPnPSchema_Int,                        // data schema of input to command
                         SerialPnPSchema_Int,                        // data schema of output to command
                         (SerialPnPCb*) CbToggleState);              // callback - see below

    GroveSetup();
}


void loop() {
    float temp;
    g_LastSample = millis();
    while(g_state) {

        SerialPnP_Process();
        uint32_t cticks = millis();

        if (cticks - g_LastSample < g_SampleRate) {
            continue;
        }
        ReadEnvironment( &temp ) ;

        SerialPnP_SendEventFloat("temperature", temp);


        g_LastSample = cticks;
    }
}

