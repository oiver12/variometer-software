# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS  
                    "main.c" "SoundManager.c" "BatteryManager.c" "SDCard.c" "nmea_parser.c" "QuaternionFilter.c" "DeviceContext.c" "ButtonInput.c" "AGL.c" "Util.c" "compass.c" "MainTasks.c" "ErrorHandling.c" "FlightState.c"
                    "Sensor/QMC588L.c" "Sensor/BMI088.c" "Sensor/DPS310.c" "Sensor/I2C_Manager.c"
                    "Screen/Display_Vario.c" "Screen/FIFO.c" "Screen/ScreenManager.c" "Screen/Widget_Vario.c" "Screen/Widget_Vario_Settings.c" "Screen/RenderSystem.c" "Screen/Widget_Vario_PopUp.c" "Screen/Widget_Vario_Common.c" "Screen/Widget_Vario_Screens.c" "Screen/SettingsManager.c"
                    "Screen/E_Ink/EPD_3in7.c" "Screen/E_Ink/GUI_Paint.c"
                    "Screen/E_Ink/font8.c" "Screen/E_Ink/font12.c" "Screen/E_Ink/font16.c" "Screen/E_Ink/font20.c" "Screen/E_Ink/font24.c"
                    "Kalman/cholesky.c" "Kalman/kalman.c" "Kalman/matrix.c" "Kalman/Quaternion/matrixSensorFusion.c" "Kalman/Quaternion/SensorFusion.c"
                    "TestTools/FlightLogger.c" "TestTools/performance.c"
                    "Screen/assets/test_logo.c" "Screen/assets/icons.c"
                    "AirspaceParcer/airspaceParcer.c" "AirspaceParcer/file_layer.c"
                    "IGC/IGC_Parser.c" "IGC/IGCLogger.c"
                    "Bluetooth/bluetooth.cpp"
                    "Watchdog/Watchdog.c"
                    "Flash/flashMemory.c"
                    "crc/crc.c"
                    )
file(GLOB NKOLBAN_BLUETOOTH
    "nkolban/*.c"
    "nkolban/*.cpp"    
)
list(APPEND COMPONENT_SRCS ${NKOLBAN_BLUETOOTH})
set(COMPONENT_ADD_INCLUDEDIRS ".")
register_component()
