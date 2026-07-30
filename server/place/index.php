<?php
include("../auth.php");
include("history.php");
if (!authenticate()) {
    exit("-Invalid credentials!");

}
const MAP_DIR = "../map";
const FILE_ORDER = ["aux", "base", "data", "desc", "layer"];
const SEPARATOR = "*";
const MAX_REQ_BODY_LEN = 32 * 1000 * 1000; //in MB
const MAX_REASON_LEN = 48;
$ACCEPTED_SECTORS = file("../sector_guard/storage.txt", FILE_IGNORE_NEW_LINES);
$n_accepted_files = count(FILE_ORDER);
$locks = json_decode(file_get_contents("../locker/storage.json"));
//-1 = sector is unlocked; string = the lock author.
function is_sector_locked2(string $sector)
{
    global $locks;
    $c = 0;
    foreach ($locks as $lock_info) {
        if (!strcmp($lock_info->sector, $sector))
            return $lock_info->author;
        $c++;
    }
    return -1;
}
function is_valid_b64(string $string): bool
{
    return base64_decode($string, true) !== false;
}



if ($_SERVER['REQUEST_METHOD'] == "POST") {
    $sector = $_GET["sector"];
    $reason = $_GET["reason"];
    if (!isset($sector)) {
        exit("-Sector unspecified!");
    }

    if (!isset($reason)) {
        $reason = "No reason provided";
    }
    if (strlen($reason) > MAX_REASON_LEN) {
        exit("-Placement reason is too long!");
    }

    if (!preg_match('/^[a-zA-Z0-9 .]+$/', $reason)) {
        echo "-Placement reason must be alphanumeric! (spaces, periods allowed)";
        exit();
    }

    $sector = str_replace(' ', '+', $sector);
    if (!in_array($sector, $ACCEPTED_SECTORS)) {
        exit("-Specified sector is outside of project bounds!");
    }
    $lock_author = is_sector_locked2($sector);
    if ($lock_author != -1 && strcmp($lock_author, $_GET["usr"]) != 0) {
        exit("-Specified sector is locked by another user ($lock_author)!");

    }
    $body = file_get_contents("php://input");
    if ($body === "") {
        exit("-No attached files!");
    }
    if (strlen($body) > MAX_REQ_BODY_LEN) {
        exit("-A max of " . strval(MAX_REQ_BODY_LEN) . "B of data may be sent at a time! Contact the developer if you must surpass this.");

    }
    $files = explode(SEPARATOR, $body);
    if (count($files) != $n_accepted_files) {
        exit("-Improper amount of attached files (" . count($files) . "/$n_accepted_files)!");
    }
    for ($i = 0; $i < $n_accepted_files; $i++) {
        if (!is_valid_b64($files[$i])) {
            exit("-File {$i} is not properly base64-encoded!");
        }
    }
    for ($i = 0; $i < $n_accepted_files; $i++) {
        file_put_contents(MAP_DIR . DIRECTORY_SEPARATOR . "$sector." . FILE_ORDER[$i], base64_decode($files[$i]), LOCK_EX);
    }
    if (
        historyAppend((object) [
            'time' => formatted_now_date(),
            'time_n' => time(),
            'reason' => $reason,
            'sectors' => $sector,
            'author' => $_GET["usr"]
        ])
    ) {
        echo "+";
    } else
        exit("- Failed to append to change history.");
} else if ($_SERVER['REQUEST_METHOD'] == "GET") {
    $action = $_GET["action"];
    if (!isset($action)) {
        exit("-Action unspecified!");
    }
    $max_hist = historyLatestNumber();
    switch ($action) {
        case 'enumerate':
            echo "+" . strval($max_hist);
            break;
        case 'retrieve':
            $file = $_GET["file"];
            if (isset($file) && ctype_digit($file)) {
                $file = (int) $file;
                $hist = historyGet($file);
                if ($file > $max_hist || !isset($hist)) {
                    exit("-Requested history file does not exist!");
                }
                echo "+" . count($hist) . PHP_EOL;
                $c = 0;
                foreach ($hist as $entry) {
                    echo $entry->sectors . "/" . $entry->author . "/" . $entry->reason . "/" . $entry->time . "/";
                    if ($c++ != count($hist) - 1)
                        echo PHP_EOL;
                }
            } else exit("-Requested file is not numeric!");
            break;
        default:
            exit("-Unknown action!");
    }
}
?>