# i know this shouldn't be managed like this, but too lazy to implement the esp-idf way of using cmake =w=
idf_component_register(SRCS "main.c" "rgb.c" "motor.c" "madgwick.c" "imu.c" "pid.c" "ble.c"
                    INCLUDE_DIRS ".")
