
export IDF_PATH="$HOME/esp/esp-idf"
. ~/esp/esp-idf/export.sh
idf.py build
idf.py flash -p /dev/ttyACM0
idf.py monitor -p /dev/ttyACM0
exit