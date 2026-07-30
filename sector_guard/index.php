<?php
include("../auth.php");
const STORAGE_FILE = "storage.txt";
const MAX_ALLOWED_SECTORS = (64) - 1;
const MAX_SECTOR_LENGTH = 24;
const FILE_ORDER = ["aux", "base", "data", "desc", "layer"];
if (!authenticate()) {
    echo "-Invalid credentials!";
    exit();
}

function deleteLineFromFile(string $filename, string $lineToDelete): bool
{
    if (!is_file($filename)) {
        return false;
    }

    $contents = file_get_contents($filename);
    if ($contents === false) {
        return false;
    }

    // Normalize line endings.
    $contents = str_replace(["\r\n", "\r"], "\n", $contents);

    $lines = explode("\n", $contents);

    // Remove trailing empty lines caused by ending newlines.
    while (!empty($lines) && end($lines) === "") {
        array_pop($lines);
    }

    $found = false;
    $newLines = [];

    foreach ($lines as $line) {
        if ($line === $lineToDelete) {
            $found = true;
            continue;
        }

        $newLines[] = $line;
    }

    if (!$found) {
        return false;
    }

    return file_put_contents($filename, implode("\n", $newLines)) !== false;
}

function delete_all_with_name_ext(string $name, array $extensions, string $directory = '.'): bool
{
    $deletedAny = false;

    foreach ($extensions as $ext) {
        $filename = rtrim($directory, DIRECTORY_SEPARATOR) .
            DIRECTORY_SEPARATOR .
            $name . '.' . $ext;

        if (is_file($filename)) {
            if (unlink($filename)) {
                $deletedAny = true;
            }
        }
    }

    return $deletedAny;
}

function touch_all_with_name_ext(string $name, array $extensions, string $directory = '.'): bool
{
    $touchedAny = false;

    foreach ($extensions as $ext) {
        $filename = rtrim($directory, DIRECTORY_SEPARATOR) .
            DIRECTORY_SEPARATOR .
            $name . '.' . $ext;
        if (touch($filename)) {
            $touchedAny = true;
        }
    }

    return $touchedAny;
}

if (!$is_administrator)
    exit("-Only administrator-level users are allowed to use the sector guard. Your account does not have administrator access!");
if ($_SERVER['REQUEST_METHOD'] == "GET") { //Get list of accepted sectors that can be modified on the server.
    $lines = file(STORAGE_FILE, FILE_IGNORE_NEW_LINES);
    echo "+" . strval(count($lines)) . "\n" . file_get_contents(STORAGE_FILE);
} else if ($_SERVER['REQUEST_METHOD'] == "POST") { //Change accepted sectors list
    $sector = file_get_contents("php://input");
    if ($sector === "") {
        exit("-No sector provided!");
    }
    $len = strlen($sector);
    if (!preg_match('/^[a-zA-Z0-9-+]+$/', $sector) || $len > MAX_SECTOR_LENGTH || $len < 2)
        exit("-Invalid sector format");
    $action = $_GET["action"];
    if (!isset($action))
        exit("-Unspecified action");
    if ($action == "add") {
        if (str_contains(file_get_contents(STORAGE_FILE), $sector)) {
            exit("-That sector is already in use!");
        }
        file_put_contents(STORAGE_FILE, "\n$sector", LOCK_EX | FILE_APPEND);
        if (!touch_all_with_name_ext($sector, FILE_ORDER, "../map/")) {
            deleteLineFromFile(STORAGE_FILE, $sector);
            exit("-Failed to touch sector '$sector'");
        }
        ;
    } else if ($action == "del") {
        if (deleteLineFromFile(STORAGE_FILE, $sector)) {
            delete_all_with_name_ext($sector, FILE_ORDER, "../map/");
            exit("+");
        }
        exit("-Sector was already not part of project scope!");
    } else {
        exit("-Invalid action");
    }
    echo "+";
}
?>
