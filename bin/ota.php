<?PHP

const debug = false;

// Dateiname: Vnn-nn-nn.<variante3>.<typ7>.bin
$db = array(
// MTQQ
    "ino.nodemcu" => "V01-04-04.ino.nodemcu.bin",
    "ino.d1_mini" => "V01-04-04.ino.d1_mini.bin",
// Relais-Webschalter mit IIC
    "iic.nodemcu" => "V00-04-02.iic.nodemcu.bin",
    "iic.d1_mini" => "V00-04-02.iic.d1_mini.bin",
// Relais-Webschalter ohne IIC, minimale Hardwarenutzung
    "min.nodemcu" => "V00-04-02.min.nodemcu.bin",
    "min.d1_mini" => "V00-04-02.min.d1_mini.bin",
);

$timestamp = "\n" . date("d.m.Y - H:i");
file_put_contents("update.log", $timestamp . " Start update, client " . $_SERVER['REMOTE_ADDR']
        . ", User-Agent " . $_SERVER['HTTP_USER_AGENT'] 
        . ", ESP-MAC " . $_SERVER['HTTP_X_ESP8266_STA_MAC'] 
        . ", AP-MAC " . $_SERVER['HTTP_X_ESP8266_AP_MAC']
        . ", Version " . $_SERVER['HTTP_X_ESP8266_VERSION']
        . ", free space " . $_SERVER['HTTP_X_ESP8266_FREE_SPACE']
        . ", sketch size " . $_SERVER['HTTP_X_ESP8266_SKETCH_SIZE']
        . ", chip size " . $_SERVER['HTTP_X_ESP8266_CHIP_SIZE']
        . ", sdk version " . $_SERVER['HTTP_X_ESP8266_SDK_VERSION']
        , FILE_APPEND);

if (debug) {
    error_reporting(E_ALL); // Oder so: error_reporting(E_ALL & ~ E_NOTICE);
    ini_set('display_errors', 'On');
}

header('Content-type: text/plain; charset=utf8', true);

function check_header($name, $value = false) {
    if (!isset($_SERVER[$name])) {
        return false;
    }
    if ($value && $_SERVER[$name] != $value) {
        return false;
    }
    return true;
}

function sendFile($path) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 200 OK', true, 200);
    header('Content-Type: application/octet-stream', true);
    header('Content-Disposition: attachment; filename=' . basename($path));
    header('Content-Length: ' . filesize($path), true);
    header('x-MD5: ' . md5_file($path), true);
    readfile($path);
}

if (!check_header('HTTP_USER_AGENT', 'ESP8266-http-Update')) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater!\n";
    file_put_contents("update.log", $timestamp . " Abort update, only for ESP8266", FILE_APPEND);
    if (!debug) {
        exit();
    }
}

if (
        !check_header('HTTP_X_ESP8266_STA_MAC') ||
        !check_header('HTTP_X_ESP8266_AP_MAC') ||
        !check_header('HTTP_X_ESP8266_FREE_SPACE') ||
        !check_header('HTTP_X_ESP8266_SKETCH_SIZE') ||
        !check_header('HTTP_X_ESP8266_CHIP_SIZE') ||
        !check_header('HTTP_X_ESP8266_SDK_VERSION') ||
        !check_header('HTTP_X_ESP8266_VERSION')
) {
    header($_SERVER["SERVER_PROTOCOL"] . ' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater! (header)\n";
    file_put_contents("update.log", $timestamp . " Abort update, missing ESP8266-header from updater", FILE_APPEND);
    if (!debug) {
        exit();
    }
}

$esp_firmware = "";
$esp_key = "";
$esp_version = $_SERVER['HTTP_X_ESP8266_VERSION'];
if (isset($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']])) {
    $esp_key = $_SERVER['HTTP_X_ESP8266_STA_MAC'];
    $esp_firmware = $db[$esp_key];
} else if (isset($db[substr($esp_version, 10, 11)])) {
    $esp_key = substr($esp_version, 10, 11);
    $esp_firmware = $db[$esp_key];
}

if ($esp_firmware != "") {
    if ($esp_firmware > $esp_version) {
        if (file_exists($esp_firmware)) {
            sendFile($esp_firmware);
            file_put_contents("update.log", $timestamp . " Update " . $esp_firmware . " ok", FILE_APPEND);
        } else {
            file_put_contents("update.log", $timestamp . " Abort update, file " . $esp_firmware . " not found", FILE_APPEND);
            header($_SERVER["SERVER_PROTOCOL"] . ' 500 no file for ESP MAC', true, 500);
        }
    } elseif ($esp_firmware < $esp_version) {
        file_put_contents("update.log", $timestamp . " Abort update, version on chip (" . $esp_version . ") is newer as (" . $esp_firmware . ")", FILE_APPEND);
        header($_SERVER["SERVER_PROTOCOL"] . ' 304 version on chip is newer', true, 304);
    } else {
        file_put_contents("update.log", $timestamp . " Abort update " . $esp_firmware . " not modified", FILE_APPEND);
        header($_SERVER["SERVER_PROTOCOL"] . ' 304 Not Modified', true, 304);
    }
} else {
    file_put_contents("update.log", $timestamp . " Abort update, no version for ESP (" . $esp_key . ") in table", FILE_APPEND);
    header($_SERVER["SERVER_PROTOCOL"] . ' 500 no version for ESP ' . $esp_version, true, 500);
}
