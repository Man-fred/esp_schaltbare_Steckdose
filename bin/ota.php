<?PHP
const debug = false;
// MTQQ
const version_nr = "V01-04-04";
// Relais-Webschalter
const relais_nr = "E00-03-03";

$d1_mini = version_nr.".ino.d1_mini";
$nodemcu = version_nr.".ino.nodemcu";

$db = array(
    "18:FE:34:D6:12:C8" => version_nr.".ino.nodemcu",
    "5C:CF:7F:15:0E:A2" => version_nr.".ino.d1_mini",
    "5C:CF:7F:14:6E:F9" => version_nr.".ino.d1_mini",
	"5C:CF:7F:0B:17:10" => version_nr.".ino.nodemcu",
	"5C:CF:7F:02:84:CF" => relais_nr .".ino.nodemcu",
	);

$timestamp = "\n".date("d.m.Y - H:i");
file_put_contents ( "update.log" , $timestamp." Start update, client ".$_SERVER['REMOTE_ADDR'].", User-Agent ".
	$_SERVER['HTTP_USER_AGENT'].", ESP-MAC ".
	$_SERVER['HTTP_X_ESP8266_STA_MAC'].", Version ".$_SERVER['HTTP_X_ESP8266_VERSION'] ,FILE_APPEND);
if (debug) {
	error_reporting (E_ALL); // Oder so: error_reporting(E_ALL & ~ E_NOTICE);
	ini_set ('display_errors', 'On');
}

header('Content-type: text/plain; charset=utf8', true);

function check_header($name, $value = false) {
    if(!isset($_SERVER[$name])) {
        return false;
    }
    if($value && $_SERVER[$name] != $value) {
        return false;
    }
    return true;
}

function sendFile($path) {
    header($_SERVER["SERVER_PROTOCOL"].' 200 OK', true, 200);
    header('Content-Type: application/octet-stream', true);
    header('Content-Disposition: attachment; filename='.basename($path));
    header('Content-Length: '.filesize($path), true);
    header('x-MD5: '.md5_file($path), true);
    readfile($path);
}

if(!check_header('HTTP_USER_AGENT', 'ESP8266-http-Update')) {
    header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater!\n";
    file_put_contents ( "update.log" , $timestamp." Abort update, only for ESP8266" ,FILE_APPEND);
    if (!debug) {
		exit();
	}
}

if(
    !check_header('HTTP_X_ESP8266_STA_MAC') ||
    !check_header('HTTP_X_ESP8266_AP_MAC') ||
    !check_header('HTTP_X_ESP8266_FREE_SPACE') ||
    !check_header('HTTP_X_ESP8266_SKETCH_SIZE') ||
    !check_header('HTTP_X_ESP8266_CHIP_SIZE') ||
    !check_header('HTTP_X_ESP8266_SDK_VERSION') ||
    !check_header('HTTP_X_ESP8266_VERSION')
) {
    header($_SERVER["SERVER_PROTOCOL"].' 403 Forbidden', true, 403);
    echo "only for ESP8266 updater! (header)\n";
    file_put_contents ( "update.log" , $timestamp." Abort update, missing ESP8266-header from updater" ,FILE_APPEND);
    if (!debug) {
		exit();
	}
}

if(isset($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']])) {
    if($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']] > $_SERVER['HTTP_X_ESP8266_VERSION']) {
        //sendFile("/esp8266/".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']].".bin");
		if (file_exists($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']].".bin")) {
			sendFile($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']].".bin");
			file_put_contents ( "update.log" , $timestamp." Update ".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']]." ok" ,FILE_APPEND);
		} else {
			file_put_contents ( "update.log" , $timestamp." Abort update, file ".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']].".bin not found" ,FILE_APPEND);
			header($_SERVER["SERVER_PROTOCOL"].' 500 no file for ESP MAC', true, 500);
		}
    } elseif($db[$_SERVER['HTTP_X_ESP8266_STA_MAC']] < $_SERVER['HTTP_X_ESP8266_VERSION']) {
		file_put_contents ( "update.log" , $timestamp." Abort update, version on chip newer as ".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']] ,FILE_APPEND);
        header($_SERVER["SERVER_PROTOCOL"].' 500 version on chip is newer', true, 304);
    } else {
		file_put_contents ( "update.log" , $timestamp." Update ".$db[$_SERVER['HTTP_X_ESP8266_STA_MAC']]." not modified" ,FILE_APPEND);
        header($_SERVER["SERVER_PROTOCOL"].' 304 Not Modified', true, 304);
    }
} else { 
	file_put_contents ( "update.log" , $timestamp." Abort update, no version for ESP MAC in table" ,FILE_APPEND);
    header($_SERVER["SERVER_PROTOCOL"].' 500 no version for ESP MAC', true, 500);
}
