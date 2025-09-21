package vendor.infineon.radar;

import vendor.infineon.radar.FrameData;

@VintfStability
interface IRawDataListener {
    oneway void onFrameReceived(in FrameData data);
}