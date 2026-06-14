# SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
#
# SPDX-License-Identifier: MIT
# boot.py
import esp32

NETWORK_TIMEOUT = 60

if __name__ == "__main__":
    from startup import startup
    from m5sync import sync
    import os

    nvs = esp32.NVS("uiflow")
    try:
        boot_option = nvs.get_u8("boot_option")
    except:
        boot_option = 1

    # Force boot_option=0 to skip UiFlow2 UI and run main.py directly
    try:
        nvs.put_u8("boot_option", 0)
    except:
        pass

    startup(0, NETWORK_TIMEOUT)

    try:
        s = open("/flash/main_ota_temp.py", "rb")
        f = open("/flash/main.py", "wb")
        f.write(s.read())
        s.close()
        f.close()
        os.remove("/flash/main_ota_temp.py")
    except:
        pass
