package vendor.infineon.radar;

import vendor.infineon.radar.IRawDataListener;
import vendor.infineon.radar.SensorConfig;

/**
 * This is an abstraction over Radar SDK from Infineon Xensiv sensors.
 * Let us keep it slim and dumb. Let a smart (system) service provide configuration and interpret data.
 *
 * NOTE: error handling is simplistic, mostly binary.
 *       In case of errors, refer to logcat for logs from the infineon libraries.
 *       Make sure that LogRedirector is running and you are using debug libs in "prebuilt/libs".
 *       See LogRedirector.h for details.
 */
@VintfStability
interface IRadarSdk {
    /**
     * Subscribe for raw data stream.
     *
     * This method does the following in order, it:
     *   - establishes connection to first sensor found,
     *   - updates sensor configuration,
     *   - starts raw data acquisition in a new thread,
     *   - calls back to listener every time a frame is received.
     *
     * @param[in] listener Callback interface to be implemented by the caller
     * @param[in] config Sensor configuration to be set before starting data acquisition
     * @return Unique subscription id to be stored by the client to later unsubscribe
     *         or -1 if error occurs
     */
    long subscribe(in IRawDataListener listener, in SensorConfig config); // TODO add ISensorHalState listener

    /**
     * Unsubscribe for raw data stream.
     * Stops data acquisition.
     *
     * This method does the following in order, it:
     *   - removes provided listener,
     *   also, if there are no more listeners:
     *     - stops data acquisition and destroys its thread,
     *     - disconnects the sensor.
     *
     * @param subscription_id Unique subscription id returned by subscribe() call
     */
    void unsubscribe(in long subscription_id);

    /**
     * Unsubscribe all listeners, stop data acquisition, and disconnect from sensor.
     * Can be used to get rid of stale listeners or to use new config.
     * NOTE: if there are other listeners running, they will stop getting data!
     */
    void unsubscribeAll();
}
