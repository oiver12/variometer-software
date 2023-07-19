
export IDF_PATH="$HOME/esp/esp-idf"
. ~/esp/esp-idf/export.sh
idf.py build
idf.py flash monitor
exit