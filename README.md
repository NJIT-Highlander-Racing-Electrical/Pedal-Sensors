# Pedal-Sensors

A subsystem that measures the angle of the accelerator and brake pedal. These values are sent via the CAN-Bus to other subsystems on the vehicle and the DAS. 
 
## 2024-2025 Design Goals

* Use [PSC360G2-F2AA-C0002-ERA360-05K](https://www.newark.com/amphenol-piher-sensorscontrols/psc360g2-f2aa-c0002-era360-05k/mag-rotary-angle-position-sensor/dp/71AC5626) from Newark Electronics (roughly $15)
   * This specific model has the following parameters:
      * Redundant Output
          * Analogic Output
          * One goes 90% to 10% voltage output from 0 degrees to 360 degrees
          * Other output goes 10% to 90% voltage output from 0 degrees to 360 degrees
      * 360 Degrees sensing range
      * 5V Opearting Voltage


## 2025-2026

Perhaps it would be worth looking into using [these linear hall sensors](https://www.mouser.com/ProductDetail/Amphenol-Piher/PS2P-LIN-CE-M002-1A0-L0000-ELS120-05?qs=vLWxofP3U2wU68Sblifdzw%3D%3D&srsltid=AfmBOoohN6dBwwOQZAoPYRWV-dxtn3KejJmPO92qOBfPrXmIcMOC2kIM). Since there is no contact, and we are free to position the magnetic piece wherever, we could eliminate the need for complex mechanisms to convert linear motion to angular. Originally, we used that setup because there was a rotating bolt at the axis of rotation so there was no linear travel. However, now that the bolt is fixed, we cannot get an angular reading at the axis of rotation
