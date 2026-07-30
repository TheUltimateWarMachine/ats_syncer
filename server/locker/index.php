<?php
include("../auth.php");
const STORAGE_FILE = "storage.json";
const SECTOR_GUARD_STORAGE = "../sector_guard/storage.txt";
const MAX_ALLOWED_LOCKED_SECTORS = 16;
$ACCEPTED_SECTORS = file(SECTOR_GUARD_STORAGE, FILE_IGNORE_NEW_LINES);
if (!authenticate()) {
    echo "-Invalid credentials!";
    exit();
}

$locks = json_decode(file_get_contents(STORAGE_FILE));
//-1 = not found, 0+ = index found at
function is_sector_locked(string $sector): int
{
    global $locks;
    $c = 0;
    foreach ($locks as $lock_info) {
        if (!strcmp($lock_info->sector, $sector))
            return $c;
        $c++;
    }
    return -1;
}

function get_lock_n(int $idx)
{
    global $locks;
    return $locks[$idx];
}

if ($_SERVER['REQUEST_METHOD'] == "GET") { //Return all sector locks
    echo "+" . count($locks) . "\n";
    $c = 0;
    foreach ($locks as $lock) {
        echo $lock->sector . "/" . $lock->author . "/" . $lock->date . "/" . $lock->reason . "/";
        if ($c++ != count($locks) - 1)
            echo PHP_EOL;
    }
} else if ($_SERVER['REQUEST_METHOD'] == "POST") {
    $action = $_GET["action"];
    if (!isset($action)) {
        echo "-Unspecified action!";
        exit();
    }
    $sector = $_GET["sector"];
    if (!isset($sector)) {
        echo "-Unspecified sector!";
        exit();
    }
    $sector = str_replace(' ', '+', $sector);
    if (!in_array($sector, $ACCEPTED_SECTORS)) {
        echo "-Specified sector $sector is not in project scope!";
        exit();
    }
    $lines = file(STORAGE_FILE, FILE_IGNORE_NEW_LINES);
    switch ($action) {
        case 'lock':
            $reason = file_get_contents("php://input");
            $len_reason = strlen($reason);
            if (!isset($reason) || $len_reason > 48 || $len_reason < 2) {
                echo "-Lock reason '$reason' must be 2-48 characters long!";
                exit();
            }
            if (!preg_match('/^[a-zA-Z0-9 .]+$/', $reason)) {
                echo "-Lock reason must be alphanumeric! (spaces, periods allowed)";
                exit();
            }
            if (count($locks) > MAX_ALLOWED_LOCKED_SECTORS) {
                echo "-Locked sectors limit of " . MAX_ALLOWED_LOCKED_SECTORS . " has been reached! Contact the developer if you must surpass this.";
                exit();
            }
            if (is_sector_locked($sector) != -1) {
                echo "-Specified sector is already locked!";
                exit();
            }
            $new_lock = (object) [
                "sector" => $sector,
                "author" => $_GET["usr"],
                "date" => formatted_now_date(),
                "reason" => $reason
            ];
            $locks[] = $new_lock;
            file_put_contents(STORAGE_FILE, json_encode($locks), LOCK_EX);
            echo "+";
            break;
        case 'unlock':
            if (($pos = is_sector_locked($sector)) == -1) {
                exit("-Specified sector is not currently locked!");
            }
            $lock_pos = get_lock_n($pos);
            if (!$is_administrator) {
                if (strcmp($_GET["usr"], $lock_pos->author))
                    exit("-Specified sector is already locked by {$lock_pos->author}!");
            }
            array_splice($locks, $pos, 1);
            file_put_contents(STORAGE_FILE, json_encode($locks), LOCK_EX);
            echo "+";
            break;
        default:
            echo "-Unknown action";
            break;
    }
}
?>